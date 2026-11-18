package ai.depz.sensor.android

import ai.depz.sensor.devices.DepzDevice
import ai.depz.sensor.devices.DepzDevices
import ai.depz.sensor.devices.DeviceLostException
import ai.depz.sensor.devices.Link
import ai.depz.sensor.devices.RecordingLink
import ai.depz.sensor.usb.UsbIds
import android.hardware.usb.UsbDevice
import java.io.IOException
import java.io.OutputStream
import java.time.Duration
import java.util.concurrent.Executor
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.flow.conflate
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext

/** Discovery, permission and opening over a [UsbHost]: what [DepzAndroid] does, without Android. */
internal class DepzUsb(
    private val host: UsbHost,
    private val dialog: Mutex = DIALOG,
    private val io: CoroutineDispatcher = Dispatchers.IO,
    private val calls: CoroutineScope = CALLS,
    private val permissionPollMs: Long = PERMISSION_POLL_MS,
) {
    fun discover(): List<DepzUsbDevice> {
        val boards = host.attached().map(host::describe)
            .filter { UsbIds.isKnownDepzUsb(it.vendorId, it.productId) }
            .associateBy { it.deviceName }
        // The desktop order (contract 02 §4): by USB serial, boards without one last.
        return UsbIds.orderBySerial(boards.values.map { UsbIds.PortRef.of(it.deviceName, it.serial.orEmpty()) })
            .map { boards.getValue(it.port()) }
    }

    /**
     * [discover] now, then again after every USB device attached or detached. A change only wakes
     * the collector, which reads the list itself: the lists arrive in the order they were true.
     */
    fun devices(): Flow<List<DepzUsbDevice>> = callbackFlow {
        val watch = host.watch { _, _ -> trySend(Unit) }
        trySend(Unit)
        awaitClose { watch.close() }
    }.conflate().map { discover() }
        // an unrelated USB device (a mouse, a hub) changes nothing here
        .distinctUntilChanged { a, b -> a.map { it.deviceName to it.serial } == b.map { it.deviceName to it.serial } }

    fun find(serial: String): DepzUsbDevice? = discover().firstOrNull { it.serial == serial }

    fun device(usbDevice: UsbDevice): DepzUsbDevice = host.describe(usbDevice)

    fun hasPermission(device: DepzUsbDevice): Boolean = host.hasPermission(device.usbDevice)

    suspend fun requestPermission(device: DepzUsbDevice): Boolean {
        if (hasPermission(device)) return true
        // Android shows one permission dialog at a time: the requests wait their turn.
        return dialog.withLock {
            if (hasPermission(device)) return true
            var request: AutoCloseable? = null
            var watch: AutoCloseable? = null
            try {
                coroutineScope {
                    val answer = CompletableDeferred<Boolean>()
                    // An unplugged board has no permission to give, and its dialog may never be
                    // answered: the wait ends with the board, or every later request would wait
                    // behind it for ever. Watched first, so a detach during the request is seen.
                    watch = host.watch { deviceName, attached ->
                        if (!attached && deviceName == device.deviceName) answer.complete(false)
                    }
                    if (host.isAttached(device.usbDevice)) {
                        request = host.requestPermission(device.usbDevice) { answer.complete(it) }
                    } else {
                        answer.complete(false)
                    }
                    // A grant whose answer never comes: the dialog on the screen was asked for by
                    // an earlier process of the app (a Samsung phone stops a new app two seconds
                    // after its first start, Android 16, 2026-11-11), so the user's "OK" went to a
                    // process that is gone. The system holds the grant all the same: read it.
                    val poll = launch {
                        while (true) {
                            delay(permissionPollMs)
                            if (hasPermission(device)) answer.complete(true)
                        }
                    }
                    try {
                        answer.await()
                    } finally {
                        poll.cancel()
                    }
                }
            } finally {
                // answered, cancelled or failed: nothing listens any more
                try {
                    request?.close()
                } finally {
                    watch?.close()
                }
            }
        }
    }

    /**
     * Opens [device]; with [capture], every byte of the link is also journaled into it as a
     * `.depzrec` (the Java SDK's `RecordingLink`). [capture] is closed with the link, and here on
     * every path that never gets to a link.
     */
    suspend fun open(device: DepzUsbDevice, timeout: Duration, capture: OutputStream? = null): DepzDevice {
        var opened: DepzDevice? = null
        var linked = false // the recording link owns the capture and closes it with itself
        try {
            if (!requestPermission(device)) {
                // No permission because the board is gone is a loss, not the user's refusal.
                if (!host.isAttached(device.usbDevice)) {
                    throw DeviceLostException(DetachWatchLink.detachedMessage(device.deviceName))
                }
                throw UsbPermissionDeniedException(device)
            }
            withContext(io) {
                var link: Link = DetachWatchLink.open(host, device)
                if (capture != null) {
                    link = try {
                        RecordingLink(link, capture, mapOf("port" to device.deviceName))
                    } catch (e: Throwable) {
                        link.close()
                        throw e
                    }
                    linked = true
                }
                // openOnLink owns the link from here: it closes it when the probe fails.
                opened = DepzDevices.openOnLink(link, device.vendorId, device.productId, timeout)
            }
        } catch (e: Throwable) {
            withContext(NonCancellable + io) {
                // Cancelled while the board was being opened: nobody gets the device, so close it.
                if (e is CancellationException) opened?.close()
                if (capture != null && !linked) {
                    try {
                        capture.close()
                    } catch (closeFailure: IOException) {
                        e.addSuppressed(closeFailure)
                    }
                }
            }
            throw e
        }
        return checkNotNull(opened)
    }

    fun requestPermission(device: DepzUsbDevice, callback: DepzCallback<Boolean>, executor: Executor): Cancellable =
        calls.answer(callback, executor) { requestPermission(device) }

    @JvmOverloads
    fun open(
        device: DepzUsbDevice, timeout: Duration, callback: DepzCallback<DepzSensor>, executor: Executor,
        capture: OutputStream? = null,
    ): Cancellable =
        // A sensor nobody will get — cancelled when it was already open — is closed.
        calls.answer(callback, executor, discard = { it.close() }) { DepzSensor.of(open(device, timeout, capture), io) }

    private companion object {
        /** One dialog at a time for the whole process. */
        val DIALOG = Mutex()

        /** How often a request that waits for its answer reads whether the board is granted. */
        const val PERMISSION_POLL_MS = 500L
    }
}
