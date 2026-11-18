package ai.depz.sensor.android

import ai.depz.sensor.devices.DepzException

/**
 * The user refused the app the USB permission for [device], so it cannot be opened.
 *
 * @property device the board the permission was asked for
 */
class UsbPermissionDeniedException(val device: DepzUsbDevice) :
    DepzException("USB permission denied for ${device.id}")
