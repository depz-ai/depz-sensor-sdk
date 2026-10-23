using System.Collections.Generic;
using Depz.Sensor.Protocol;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53lx;

namespace Depz.Sensor.Usb;

/// <summary>
/// Class resolution (contracts/02_COMMON_COMMANDS.md §4.4, Python reference
/// <c>resolve_sensor_class</c>): which SDK class a device opens as, from its
/// probe answers and its port's USB (vid, pid). This SDK is decode-only (no
/// device layer), so it carries the pure part of §4.4 only.
/// </summary>
public static class Discovery
{
    /// <summary>
    /// Every name <see cref="ResolveSensorClass"/> returns: the contract 07
    /// sensor classes, <c>"vl53lx"</c> (the generic 1D-family class: an
    /// unstamped board, or a VL53L4CD on the APP_VL53L0_4 firmware),
    /// <c>"tmf8829"</c>, <c>"device"</c> (the base device: an unknown
    /// application, or a name that is neither APP_ nor BOOTDEPZ) and
    /// <c>"bootloader"</c> (use the contract 06 client). BNO085 boards run
    /// APP_BNO086 and resolve to <c>"bno086"</c>.
    /// </summary>
    public static readonly IReadOnlyList<string> SensorClassNames = new[]
    {
        "sr04", "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx", "vl53l7ch",
        "vl53l0x", "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx", "vl53lx",
        "bno086", "bno055", "tmf8829", "device", "bootloader",
    };

    /// <summary>
    /// The canonical class name (one of <see cref="SensorClassNames"/>) for a
    /// device's probe answers and its port's USB id — contract 02 §4.4, pure.
    /// <c>softwareName</c> is the GET_NAME_ACTIVE_SOFTWARE answer,
    /// <c>deviceName</c> the GET_DEVICE_NAME answer (<c>""</c>/null when none);
    /// the USB id is null when unknown (a replay, a bridge).
    /// BOOTDEPZ_* → <c>"bootloader"</c>; vl53l8 → <c>"vl53l8ch"</c> if the USB
    /// model is VL53L8CH, else <c>"vl53l8cx"</c>; vl53l7 →
    /// <see cref="Vl53l7Discovery.ResolveModel"/>; vl53lx →
    /// <see cref="Vl53lxProducts.ResolveClass"/> (VL53L4CD on that firmware is
    /// the generic <c>"vl53lx"</c>); any other known token → its class;
    /// anything else → <c>"device"</c>.
    /// </summary>
    public static string ResolveSensorClass(string softwareName, string? deviceName, int? usbVid, int? usbPid)
    {
        // Each USB id, when known, is a u16 (contract 07 §3) — even if the other is not.
        if (usbVid is { } v)
            Protocol.Wire.CheckUint("usbVid", v, 16);
        if (usbPid is { } p)
            Protocol.Wire.CheckUint("usbPid", p, 16);
        Identity ident = IdentityParser.ParseSoftwareName(softwareName);
        if (ident.Mode == "bootloader")
            return "bootloader";
        string? model = UsbIds.UsbModelHint(usbVid, usbPid);
        string name = deviceName ?? "";
        return ident.SensorType switch
        {
            SensorType.Sr04 => "sr04",
            // CX and CH share firmware name and silicon ids: only the production
            // PID tells them apart; the dev default (0483:56DC) falls to the CX base.
            SensorType.Vl53l8 => model == "vl53l8ch" ? "vl53l8ch" : "vl53l8cx",
            SensorType.Vl53l7 => Vl53l7Discovery.ResolveModel(model, name).ToString().ToLowerInvariant(),
            SensorType.Vl53lx => Vl53lxProducts.ResolveClass(model, name).ToString().ToLowerInvariant(),
            SensorType.Vl53l4 => "vl53l4cd",
            SensorType.Bno086 => "bno086",
            SensorType.Bno055 => "bno055",
            SensorType.Tmf8829 => "tmf8829",
            _ => "device",
        };
    }
}
