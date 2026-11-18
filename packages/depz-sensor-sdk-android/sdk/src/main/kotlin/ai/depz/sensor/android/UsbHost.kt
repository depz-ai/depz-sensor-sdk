package ai.depz.sensor.android

import ai.depz.sensor.devices.Link
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Build
import java.util.UUID
import java.util.concurrent.atomic.AtomicBoolean

/**
 * What the SDK needs from Android's USB host: the seam the unit tests replace. A [UsbDevice] is
 * only a handle here — everything read from it goes through [describe].
 */
internal interface UsbHost {
    /** Every attached USB device, DEPZ or not. */
    fun attached(): List<UsbDevice>

    /** Whether [device] is still attached: a handle outlives its device. */
    fun isAttached(device: UsbDevice): Boolean

    /** The ids and strings of [device]; the serial only when the app may read it. */
    fun describe(device: UsbDevice): DepzUsbDevice

    fun hasPermission(device: UsbDevice): Boolean

    /**
     * Shows the system's permission dialog for [device]; [onAnswer] gets the user's answer, on any
     * thread, possibly before this returns. The caller closes the result on every path — after
     * the answer or instead of it: that releases what listens for the answer.
     */
    fun requestPermission(device: UsbDevice, onAnswer: (granted: Boolean) -> Unit): AutoCloseable

    /** Opens the CDC-ACM link to [device]; the app holds the permission. */
    fun openLink(device: UsbDevice): Link

    /**
     * Calls [onChange] after every USB device attached or detached, DEPZ or not, with Android's
     * name of it — on any thread, and [attached] already tells the change — until the result is
     * closed.
     */
    fun watch(onChange: (deviceName: String, attached: Boolean) -> Unit): AutoCloseable
}

/** [UsbHost] over [UsbManager]. */
internal class AndroidUsbHost(context: Context) : UsbHost {
    // The application's context: a receiver must not keep an activity alive.
    private val context: Context = context.applicationContext
    // Null on a device without the USB service (no USB host and no accessory feature): nothing
    // is attached there, so no other function is reached with a device.
    private val service: UsbManager? = this.context.getSystemService(UsbManager::class.java)
    private val manager: UsbManager get() = checkNotNull(service) { "this device has no USB service" }

    override fun attached(): List<UsbDevice> = service?.deviceList?.values?.toList().orEmpty()

    override fun isAttached(device: UsbDevice): Boolean =
        service?.deviceList?.containsKey(device.deviceName) == true

    override fun describe(device: UsbDevice): DepzUsbDevice {
        // Android 10+ throws instead of answering while the app holds no permission.
        val serial = try {
            device.serialNumber
        } catch (e: SecurityException) {
            null
        }
        return DepzUsbDevice(
            device, device.deviceName, device.vendorId, device.productId,
            serial?.takeIf { it.isNotEmpty() }, device.productName,
        )
    }

    override fun hasPermission(device: UsbDevice): Boolean = manager.hasPermission(device)

    override fun requestPermission(device: UsbDevice, onAnswer: (granted: Boolean) -> Unit): AutoCloseable {
        // An action of its own per request, and one nobody can know: an answer can only be this
        // request's — not the answer to a dialog an earlier process of the app left on the screen
        // (a counter starts again with the process), not another app's broadcast.
        val action = "${context.packageName}.depz.USB_PERMISSION.${UUID.randomUUID()}"
        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                // The answer is read from the system, not from the intent: before Android 13 a
                // receiver registered at run time is open to any app's broadcast.
                onAnswer(manager.hasPermission(device))
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.registerReceiver(receiver, IntentFilter(action), Context.RECEIVER_NOT_EXPORTED)
        } else {
            context.registerReceiver(receiver, IntentFilter(action))
        }
        // Explicit package: Android 14 refuses a mutable PendingIntent with an implicit intent.
        // Mutable: the system adds the device and the answer to the intent.
        val answer = PendingIntent.getBroadcast(
            context, 0, Intent(action).setPackage(context.packageName), PendingIntent.FLAG_MUTABLE,
        )
        val released = AtomicBoolean(false)
        val release = AutoCloseable {
            if (released.compareAndSet(false, true)) {
                context.unregisterReceiver(receiver)
                answer.cancel()
            }
        }
        try {
            manager.requestPermission(device, answer)
        } catch (e: RuntimeException) {
            release.close()
            throw e
        }
        return release
    }

    override fun openLink(device: UsbDevice): Link = AndroidUsbLink.open(manager, device)

    override fun watch(onChange: (deviceName: String, attached: Boolean) -> Unit): AutoCloseable {
        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                val device = usbDeviceOf(intent) ?: return
                onChange(device.deviceName, intent.action == UsbManager.ACTION_USB_DEVICE_ATTACHED)
            }
        }
        // Both actions are protected broadcasts: only the system sends them, to exported and
        // unexported receivers alike.
        val filter = IntentFilter(UsbManager.ACTION_USB_DEVICE_ATTACHED)
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED)
        } else {
            context.registerReceiver(receiver, filter)
        }
        val released = AtomicBoolean(false)
        return AutoCloseable { if (released.compareAndSet(false, true)) context.unregisterReceiver(receiver) }
    }

    private fun usbDeviceOf(intent: Intent): UsbDevice? =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice::class.java)
        } else {
            @Suppress("DEPRECATION")
            intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
        }
}
