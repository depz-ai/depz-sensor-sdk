package ai.depz.sensor.android

import ai.depz.sensor.devices.DepzDevice
import android.content.Context
import android.hardware.usb.UsbDevice
import java.io.OutputStream
import java.time.Duration
import java.util.concurrent.Executor
import kotlinx.coroutines.flow.Flow

/**
 * The DEPZ sensor boards attached to this Android device over USB host: find them, get the
 * user's permission, open them.
 *
 * ```kotlin
 * val board = DepzAndroid.discover(context).first()
 * val sensor = DepzAndroid.open(context, board)   // asks for the USB permission when needed
 * try {
 *     if (sensor is Sr04Sensor) {
 *         sensor.start()
 *         sensor.measurements.collect { measurement -> … }
 *     }
 * } finally {
 *     sensor.close()
 * }
 * ```
 *
 * From Java, without coroutines — every `suspend` function has a variant that answers a
 * [DepzCallback] on an [Executor] (the main thread unless one is named) and returns a
 * [Cancellable]; [discover], [find], [device] and [hasPermission] answer at once and are called
 * as they are:
 *
 * ```java
 * DepzUsbDevice board = DepzAndroid.discover(context).get(0);
 * Cancellable opening = DepzAndroid.open(context, board, new DepzCallback<DepzSensor>() {
 *     @Override public void onResult(DepzSensor sensor) { … }   // main thread; close it when done
 *     @Override public void onError(Exception error) { … }      // UsbPermissionDeniedException, DepzException…
 * });
 * ```
 *
 * No function here blocks its thread: the suspending ones and their callback variants wait for
 * the user or work on [kotlinx.coroutines.Dispatchers.IO], the others only read Android's device
 * list. Every `context` may be any [Context]; the SDK keeps the application's.
 */
object DepzAndroid {
    /**
     * The attached DEPZ boards — the USB devices with a DEPZ id, by the Java SDK's table
     * ([ai.depz.sensor.usb.UsbIds.isKnownDepzUsb], the filter desktop discovery uses); any other
     * USB device is ignored. Nothing is opened and no permission is asked.
     *
     * Ordered as on desktop: by USB serial, the boards without a readable serial last. Several
     * boards of one model differ by [DepzUsbDevice.serial] — which is `null` until the app holds
     * the permission for the board, so call this again after [requestPermission] to read it.
     */
    @JvmStatic
    fun discover(context: Context): List<DepzUsbDevice> = usb(context).discover()

    /**
     * The attached DEPZ boards as they come and go (hot-plug): the list [discover] returns, at once
     * when the collection starts and then again every time a DEPZ board is attached or detached —
     * a board plugged in while the app runs shows up, an unplugged one is gone from the next list.
     * Nothing is opened and no permission is asked.
     *
     * A cold flow that never completes: each collection listens to Android's
     * `USB_DEVICE_ATTACHED` / `USB_DEVICE_DETACHED` broadcasts with a receiver of its own (on the
     * application's context; no manifest entry needed), released when the collection is
     * cancelled. A collector slower than the changes gets the latest list, not every one. Each
     * list is read in the collector's context, as [discover] is. A granted permission is not a
     * USB change: the serial it makes readable is in the next list, or in a [discover] call.
     *
     * A re-plugged board is a new device to Android — a new [DepzUsbDevice.deviceName], the same
     * [DepzUsbDevice.serial] — and nothing reconnects by itself: [open] it again.
     */
    @JvmSynthetic
    fun devices(context: Context): Flow<List<DepzUsbDevice>> = usb(context).devices()

    /**
     * The attached DEPZ board whose USB serial is [serial], or `null`: none is attached, or the
     * app holds no permission for it yet and so cannot read its serial.
     */
    @JvmStatic
    fun find(context: Context, serial: String): DepzUsbDevice? = usb(context).find(serial)

    /**
     * [usbDevice] — one from [android.hardware.usb.UsbManager] or from a
     * `USB_DEVICE_ATTACHED` intent — as a [DepzUsbDevice], to [open] exactly that device. Its USB
     * id is not checked: as a serial port opened by its path on desktop, the board answers for
     * itself when it is opened.
     */
    @JvmStatic
    fun device(context: Context, usbDevice: UsbDevice): DepzUsbDevice = usb(context).device(usbDevice)

    /** Whether the app holds the USB permission for [device]: [open] will show no dialog. */
    @JvmStatic
    fun hasPermission(context: Context, device: DepzUsbDevice): Boolean = usb(context).hasPermission(device)

    /**
     * Asks the user for the USB permission for [device] with the system's dialog and suspends
     * until the answer. Returns at once with `true` when the app already holds it.
     *
     * Android shows one such dialog at a time, so concurrent requests take turns. Cancelling the
     * coroutine stops the wait and releases the broadcast receiver that listens for the answer;
     * the dialog itself stays until the user dismisses it. A board that is not attached is not
     * asked for, and one unplugged while its dialog is open ends the wait: both are `false`.
     *
     * @return `true` when the permission is granted; `false` when the user refused, or the board
     *   is not attached (any more)
     */
    @JvmSynthetic
    suspend fun requestPermission(context: Context, device: DepzUsbDevice): Boolean =
        usb(context).requestPermission(device)

    /**
     * [requestPermission] for a program without coroutines: returns at once, and [callback] gets
     * the answer on [executor] — `onResult(true)` when the permission is granted (also when the
     * app already holds it), `onResult(false)` when the user refused or the board is not attached
     * (any more), `onError` when the dialog could not be shown.
     *
     * @param executor where [callback] is called; the main thread when omitted
     * @return the handle that stops the wait ([Cancellable.cancel]): the receiver that listens
     *   for the answer is released and [callback] is not called
     */
    @JvmStatic
    @JvmOverloads
    fun requestPermission(
        context: Context,
        device: DepzUsbDevice,
        callback: DepzCallback<Boolean>,
        executor: Executor = MainExecutor,
    ): Cancellable = usb(context).requestPermission(device, callback, executor)

    /**
     * Opens [device] as the [DepzSensor] of its family. The permission is asked first when the app
     * does not hold it ([requestPermission]); then the board is probed and its class chosen by the
     * Java SDK exactly as on desktop (`DepzDevices.openOnLink` with the board's USB ids), so the
     * result is an [Sr04Sensor], a [Vl53l8Sensor], a [Bno086Sensor]… — or a [GenericSensor] for a
     * board the SDK has no class for; [DepzSensor.device] is the Java SDK's device under it. The
     * caller closes it.
     *
     * On any failure nothing stays open: the USB link is closed. The same when the coroutine is
     * cancelled, though the probe under way is not interrupted — it ends within its timeouts.
     *
     * **Unplugged while open.** Android's `USB_DEVICE_DETACHED` for the board ends it as a loss:
     * its streams fail with `DeviceLostException` (a listener's `onError`), whose `cause` names
     * the device — `/dev/bus/usb/001/005: device detached`, the [DepzUsbDevice.deviceName] — the
     * exception is then in [DepzSensor.lost], every later call fails with it, and the USB
     * connection is released. The other open boards are not touched. The lost sensor still has
     * to be closed; nothing reconnects — see [devices] for the board coming back.
     *
     * **Capturing the link.** With [capture], every byte exchanged with the board from the first
     * probe on is also journaled into it: a `.depzrec` capture (contract 08; the Java SDK's
     * `RecordingLink`, with Android's device name as the header's `port`), which every DEPZ SDK
     * replays as if the board were attached (`ReplayLink`) — what to send with a bug report.
     * The stream is the SDK's from this call on: closed when the sensor is closed or lost, and
     * when the board could not be opened. Seconds of a capture are kilobytes to megabytes; for
     * the measurements of a long run see [DepzSensor.record].
     *
     * @param timeout how long the device waits for a reply to a request, as in the Java SDK
     * @param capture where the `.depzrec` capture of the link goes; `null` for none
     * @throws UsbPermissionDeniedException the user refused the permission
     * @throws ai.depz.sensor.devices.DeviceLostException the USB device cannot be opened — it is
     *   gone (also while its permission dialog was open: `…: device detached`), or it is not a
     *   CDC-ACM serial device
     * @throws ai.depz.sensor.devices.NoDepzDeviceException nothing DEPZ-shaped answered the probe
     * @throws ai.depz.sensor.devices.BootloaderModeException the board runs its bootloader
     */
    @JvmSynthetic
    suspend fun open(
        context: Context,
        device: DepzUsbDevice,
        timeout: Duration = DepzDevice.DEFAULT_TIMEOUT,
        capture: OutputStream? = null,
    ): DepzSensor = DepzSensor.of(usb(context).open(device, timeout, capture))

    /**
     * [open] for a program without coroutines: returns at once — the board is probed on a
     * background thread — and [callback] gets the outcome on [executor]: `onResult` with the open
     * [DepzSensor], which the program then closes, or `onError` with what the `suspend` variant
     * throws ([UsbPermissionDeniedException] after a refusal, the Java SDK's `DepzException`s).
     *
     * @param executor where [callback] is called; the main thread when omitted. One that refuses
     *   the task (shut down) gets no call, and the sensor is closed
     * @param timeout how long the device waits for a reply to a request, as in the Java SDK
     * @param capture where the `.depzrec` capture of the link goes, as in the `suspend` variant;
     *   `null` for none
     * @return the handle that gives the board up ([Cancellable.cancel]): [callback] is not called
     *   and nothing stays open — a board already open by then is closed
     */
    @JvmStatic
    @JvmOverloads
    fun open(
        context: Context,
        device: DepzUsbDevice,
        callback: DepzCallback<DepzSensor>,
        executor: Executor = MainExecutor,
        timeout: Duration = DepzDevice.DEFAULT_TIMEOUT,
        capture: OutputStream? = null,
    ): Cancellable = usb(context).open(device, timeout, callback, executor, capture)

    private fun usb(context: Context) = DepzUsb(AndroidUsbHost(context))
}
