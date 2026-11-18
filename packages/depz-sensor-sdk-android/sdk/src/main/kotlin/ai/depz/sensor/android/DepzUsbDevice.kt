package ai.depz.sensor.android

import ai.depz.sensor.usb.UsbIds
import android.hardware.usb.UsbDevice

/**
 * One attached USB device as [DepzAndroid.discover] lists it: a snapshot taken at that moment.
 *
 * @property usbDevice the Android device this describes, for [android.hardware.usb.UsbManager]
 * @property deviceName Android's name of the device (`/dev/bus/usb/001/005`): unique among the
 *   attached devices, new after every re-plug
 * @property vendorId the USB vendor id
 * @property productId the USB product id
 * @property serial the USB serial number. **`null` while the app holds no USB permission for the
 *   device** (Android 10+ hides the serial until then), and for a device that reports none: ask
 *   with [DepzAndroid.requestPermission], then [DepzAndroid.discover] again to read it
 * @property productName the product string the device reports, if any
 */
class DepzUsbDevice internal constructor(
    val usbDevice: UsbDevice,
    val deviceName: String,
    val vendorId: Int,
    val productId: Int,
    val serial: String?,
    val productName: String?,
) {
    /**
     * What tells this board from the others: the USB [serial] when it is known — the same on every
     * plug and every host — else [deviceName], which holds only until the board is unplugged. So
     * the id of a board changes once, when the permission is granted and its serial becomes
     * readable.
     */
    val id: String get() = serial ?: deviceName

    /**
     * The sensor model the USB product id stands for (`"sr04"`, `"vl53l8cx"`, `"bno086"`…), or
     * `null` for an id without one. A hint, as on desktop: what the board is, it answers itself
     * when [DepzAndroid.open] probes it.
     */
    val modelHint: String? get() = UsbIds.usbModelHint(vendorId, productId)

    override fun toString(): String =
        "DepzUsbDevice(%s %04X:%04X%s)".format(id, vendorId, productId, productName?.let { " \"$it\"" } ?: "")
}
