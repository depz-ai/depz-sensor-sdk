package ai.depz.sensor.android

import ai.depz.sensor.devices.DeviceLostException
import ai.depz.sensor.devices.Link
import ai.depz.sensor.devices.LinkClosedException
import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbInterface
import android.hardware.usb.UsbManager
import java.time.Duration
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.locks.LockSupport
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

/**
 * The Java SDK's [Link] over Android USB host, for a CDC-ACM board.
 *
 * The device core reads on its own thread in short slices, writes on the caller's thread and may
 * close while a read is blocked, and closes a second time after a loss. A transfer holds a lock for
 * its duration, so [close] releases the interfaces only once no transfer uses them.
 *
 * A detached device: its transfers fail, [read] then finds the device missing from Android's
 * list and throws [DeviceLostException] — by the end of the read under way, whose timeout is the
 * caller's short slice — and [close] still releases the connection. A board opened with
 * [DepzAndroid.open] is also told of the detach by Android's broadcast.
 *
 * The app must already hold the USB permission for [device].
 */
class AndroidUsbLink private constructor(
    private val usbManager: UsbManager,
    private val device: UsbDevice,
    private val connection: UsbDeviceConnection,
    private val control: UsbInterface,
    private val data: UsbInterface,
    private val bulkIn: UsbEndpoint,
    private val bulkOut: UsbEndpoint,
) : Link {
    private val closed = AtomicBoolean(false)
    private val readLock = ReentrantLock()
    private val writeLock = ReentrantLock()

    // One packet per transfer: a transfer that times out part-way through a larger buffer loses
    // the packets it had already received.
    private val packet = ByteArray(bulkIn.maxPacketSize)

    override fun name(): String = device.deviceName

    override fun isClosed(): Boolean = closed.get()

    override fun read(timeout: Duration): ByteArray = readLock.withLock {
        checkOpen()
        val ms = timeout.toMillis().coerceIn(1, Int.MAX_VALUE.toLong()).toInt()
        val started = System.nanoTime()
        val n = connection.bulkTransfer(bulkIn, packet, packet.size, ms)
        when {
            n > 0 -> packet.copyOf(n)
            // UsbDeviceConnection.bulkTransfer returns -1 for every failure, with no reason (API 31
            // sources: the ioctl's -1, errno dropped): a timeout and a device that is gone look
            // the same. Android's device list tells the two apart.
            n < 0 && !attached() -> throw DeviceLostException(DetachWatchLink.detachedMessage(name()))
            n < 0 -> {
                // A timeout took the whole of `ms`; a transfer that failed at once (the device
                // just unplugged and still listed, a USB error) waits the rest of it here, or
                // the reader, which calls back to back, would spin on the failure.
                val restNs = ms * 1_000_000L - (System.nanoTime() - started)
                if (restNs > 0) LockSupport.parkNanos(restNs) // throws nothing when interrupted
                EMPTY
            }
            else -> EMPTY
        }
    }

    override fun write(data: ByteArray): Unit = writeLock.withLock {
        checkOpen()
        var done = 0
        while (done < data.size) {
            val n = connection.bulkTransfer(bulkOut, data, done, data.size - done, WRITE_TIMEOUT_MS)
            if (n <= 0) {
                throw DeviceLostException(
                    if (attached()) "${name()}: write failed" else DetachWatchLink.detachedMessage(name()),
                )
            }
            done += n
        }
    }

    override fun close() {
        if (!closed.compareAndSet(false, true)) return
        readLock.withLock {
            writeLock.withLock {
                setControlLines(connection, control, 0)
                connection.releaseInterface(data)
                connection.releaseInterface(control)
                connection.close()
            }
        }
    }

    private fun checkOpen() {
        if (closed.get()) throw LinkClosedException("${name()} closed")
    }

    private fun attached(): Boolean = usbManager.deviceList.containsKey(device.deviceName)

    companion object {
        private val EMPTY = ByteArray(0)
        private const val WRITE_TIMEOUT_MS = 1000
        private const val CONTROL_TIMEOUT_MS = 1000
        private const val USB_RT_ACM = 0x21
        private const val SET_LINE_CODING = 0x20
        private const val SET_CONTROL_LINE_STATE = 0x22
        private const val DTR_AND_RTS = 0x03
        private const val DTR_LOW_MS = 20L

        /** 115200 baud, one stop bit, no parity, eight data bits. */
        private val LINE_CODING = byteArrayOf(0x00, 0xC2.toByte(), 0x01, 0x00, 0, 0, 8)

        private fun setControlLines(connection: UsbDeviceConnection, control: UsbInterface, state: Int) {
            connection.controlTransfer(
                USB_RT_ACM, SET_CONTROL_LINE_STATE, state, control.id, null, 0, CONTROL_TIMEOUT_MS,
            )
        }

        /**
         * Opens [device]: claims its CDC interfaces and raises DTR and RTS (a CDC device may hold
         * its output until the host asserts DTR). Blocks for the USB requests and a 20 ms DTR
         * pulse, as [read], [write] and [close] block for theirs: not for the main thread
         * ([DepzAndroid.open] calls it on a background one).
         *
         * @throws DeviceLostException the device cannot be opened
         */
        fun open(usbManager: UsbManager, device: UsbDevice): AndroidUsbLink {
            val interfaces = (0 until device.interfaceCount).map(device::getInterface)
            val control = interfaces.firstOrNull { it.interfaceClass == UsbConstants.USB_CLASS_COMM }
                ?: throw DeviceLostException("${device.deviceName}: no CDC control interface")
            val data = interfaces.firstOrNull { it.interfaceClass == UsbConstants.USB_CLASS_CDC_DATA }
                ?: throw DeviceLostException("${device.deviceName}: no CDC data interface")
            val endpoints = (0 until data.endpointCount).map(data::getEndpoint)
                .filter { it.type == UsbConstants.USB_ENDPOINT_XFER_BULK }
            val bulkIn = endpoints.firstOrNull { it.direction == UsbConstants.USB_DIR_IN }
                ?: throw DeviceLostException("${device.deviceName}: no bulk IN endpoint")
            val bulkOut = endpoints.firstOrNull { it.direction == UsbConstants.USB_DIR_OUT }
                ?: throw DeviceLostException("${device.deviceName}: no bulk OUT endpoint")

            val connection = usbManager.openDevice(device)
                ?: throw DeviceLostException("${device.deviceName}: cannot open (no USB permission?)")
            if (!connection.claimInterface(control, true) || !connection.claimInterface(data, true)) {
                connection.close()
                throw DeviceLostException("${device.deviceName}: cannot claim the CDC interfaces")
            }
            try {
                connection.controlTransfer(
                    USB_RT_ACM, SET_LINE_CODING, 0, control.id, LINE_CODING, LINE_CODING.size, CONTROL_TIMEOUT_MS,
                )
                // A board disarms its streams when DTR drops. A process that died left DTR up and
                // the sensor running, so start from a drop.
                setControlLines(connection, control, 0)
                Thread.sleep(DTR_LOW_MS)
                setControlLines(connection, control, DTR_AND_RTS)
                return AndroidUsbLink(usbManager, device, connection, control, data, bulkIn, bulkOut)
            } catch (e: Throwable) {
                // An interrupted thread, most likely: nobody gets the link, so nothing stays claimed.
                connection.close()
                throw e
            }
        }
    }
}
