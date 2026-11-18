package ai.depz.sensor.android

import ai.depz.sensor.devices.DeviceLostException
import ai.depz.sensor.devices.Link
import java.time.Duration
import java.util.concurrent.atomic.AtomicBoolean

/**
 * The link of a board opened through [DepzAndroid], told by Android when its USB device is
 * detached: from then on [read] and [write] throw a [DeviceLostException] that names the device,
 * which the Java SDK's device takes as the loss — its streams end with it and it closes the link.
 *
 * The link under it reports a detach of its own on its next transfer ([AndroidUsbLink] asks
 * Android whether the device is still attached); Android's broadcast makes the loss independent
 * of what a transfer on a vanished device returns, and both say it in the same words.
 */
internal class DetachWatchLink private constructor(
    private val link: Link,
    private val watch: AutoCloseable,
    private val detached: AtomicBoolean,
    private val deviceName: String,
) : Link {
    private val watching = AtomicBoolean(true)

    override fun read(timeout: Duration): ByteArray {
        checkAttached()
        val data = link.read(timeout)
        // Detached during the read: the loss is reported by this read, not by the next.
        checkAttached()
        return data
    }

    override fun write(data: ByteArray) {
        checkAttached()
        link.write(data)
    }

    /** Stops listening for the detach, then closes the link under this one. Idempotent, as the link's. */
    override fun close() {
        try {
            if (watching.compareAndSet(true, false)) watch.close()
        } finally {
            link.close()
        }
    }

    override fun isClosed(): Boolean = link.isClosed

    override fun name(): String = link.name()

    private fun checkAttached() {
        if (detached.get()) throw DeviceLostException(detachedMessage(deviceName))
    }

    companion object {
        /** What a link says of its detached device, by Android's name of it. */
        fun detachedMessage(deviceName: String): String = "$deviceName: device detached"

        /**
         * Opens the link of [device] and watches for its detach until the link is closed. The
         * watch starts first: a device that goes away while it is being opened is not missed.
         */
        fun open(host: UsbHost, device: DepzUsbDevice): DetachWatchLink {
            val detached = AtomicBoolean(false)
            val watch = host.watch { deviceName, attached ->
                if (!attached && deviceName == device.deviceName) detached.set(true)
            }
            try {
                return DetachWatchLink(host.openLink(device.usbDevice), watch, detached, device.deviceName)
            } catch (e: Throwable) {
                watch.close()
                throw e
            }
        }
    }
}
