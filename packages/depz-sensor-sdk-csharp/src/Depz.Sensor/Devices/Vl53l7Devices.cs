using System.Diagnostics;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;

namespace Depz.Sensor.Devices;

/// <summary>
/// VL53L7CX multizone ToF (8×8 zones, 90° field of view) on the I2C bridge
/// board (contract 11). The board speaks the VL53L8 protocol plus three
/// commands, and the sensor runs the same ST ULD family, so this is a
/// <see cref="Vl53l8cxDevice"/> with the L5/L7 firmware set (ULD 2.0.1),
/// smaller register reads, ranging from 1 Hz, and the board commands
/// <see cref="GetBridgeInfo"/>, <see cref="SetI2cSpeedKhz"/>, <see cref="PinCtrl"/>.
/// L5CX / L7CX have no deep sleep and no threshold auto-stop.
/// </summary>
public class Vl53l7cxDevice : Vl53l8cxDevice
{
    public Vl53l7cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Vl53l7cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Vl53l7cxDevice(DepzDevice from) : base(from) { }

    protected override Vl53l8Firmware FirmwareSet => Vl53l8Firmware.L7cx;

    protected override int ReadChunk => Vl53l7Wire.ReadMaxLen;

    protected override int WriteChunk => Vl53l7Wire.WriteMaxLen;

    protected override int StreamChunk => Vl53l7Wire.StreamChunkMax;

    protected override int MinRangingHz => Vl53l7Frames.MinRangingFrequencyHz;

    /// <summary>The module type this class expects after <see cref="Init"/> (L5 and L7 share a firmware).</summary>
    protected virtual int ExpectedModuleType => Vl53l8Driver.ModuleTypeMzevo;

    /// <summary>The sensor's module type read at <see cref="Init"/>: <see cref="Vl53l8Driver.ModuleTypeMz"/>
    /// (VL53L5CX) or <see cref="Vl53l8Driver.ModuleTypeMzevo"/> (VL53L7CX / CH); null before.</summary>
    public int? ModuleType => UldOrNull?.ModuleType;

    /// <summary>The last <see cref="Vl53l8cxDevice.CalibrateXtalk"/> found nothing to calibrate ("coverglass
    /// too good"): the sensor keeps its default xtalk data.</summary>
    public bool XtalkCalibrationFailed => UldOrNull?.XtalkCalibrationFailed ?? false;

    /// <summary>Init, refusing non-L5/L7 silicon. A module type other than this class expects (a
    /// board labelled L7 carrying an L5, or the reverse) is traced as a warning: ranging still works,
    /// the firmware is shared.</summary>
    public override void Init(Action<string>? progress = null, Action<int, int>? writeProgress = null)
    {
        base.Init(progress, writeProgress);
        if (ModuleType is int got && got != ExpectedModuleType)
            Trace.TraceWarning($"{GetType().Name} expects module type {ModuleName(ExpectedModuleType)}, "
                               + $"the sensor reports {ModuleName(got)} — check the board's device name");
    }

    private static string ModuleName(int t) => t switch
    {
        Vl53l8Driver.ModuleTypeMz => "MZ",
        Vl53l8Driver.ModuleTypeMzevo => "MZEVO",
        Vl53l8Driver.ModuleTypeMzplus => "MZPlus",
        _ => t.ToString(System.Globalization.CultureInfo.InvariantCulture),
    };

    // ── board commands (contract 11 §2) ──────────────────────────────────────

    /// <summary>Bridge counters and pin levels (the sensor is not touched). Read it before and after a
    /// run: during one each call takes the bus from the stream and can cost a frame.</summary>
    public Vl53l7Info GetBridgeInfo() =>
        Request((int)Vl53l7Cmd.GetInfo, null, ExpectReport((int)Vl53l7Rpt.Vl53Info, p => Vl53l7Info.Unpack(p)));

    /// <summary>Sensor bus SCL; the board takes the nearest of 100, 200, 400, 500 … 1000 kHz. Returns
    /// the value in effect. <see cref="BusyException"/> mid-transfer: stop ranging first.</summary>
    public int SetI2cSpeedKhz(int khz)
    {
        if (khz < 1 || khz > 0xFFFF)
            throw new ArgumentOutOfRangeException(nameof(khz), khz, "khz must be 1..65535");
        RequestOk((int)Vl53l7Cmd.SetI2cSpeed, Vl53l7Wire.PackSetI2cSpeed((ushort)khz));
        return GetBridgeInfo().I2cKhz;
    }

    /// <summary>Drive the sensor's LPn / I2C_RST pins (<see cref="Vl53l7PinAction"/>). LpnOff and
    /// SoftCycle drop the sensor's state: <see cref="Init"/> again afterwards.</summary>
    public void PinCtrl(byte action)
    {
        if (action > Vl53l7PinAction.SoftCycle)
            throw new ArgumentOutOfRangeException(nameof(action), action, "pin action must be 0..3 (Vl53l7PinAction)");
        RequestOk((int)Vl53l7Cmd.PinCtrl, Vl53l7Wire.PackPinCtrl(action));
        if (action == Vl53l7PinAction.LpnOff || action == Vl53l7PinAction.SoftCycle)
            ForgetSensor();
    }
}

/// <summary>VL53L5CX ToF (8×8 zones, 63° field of view): the board, firmware and API of
/// <see cref="Vl53l7cxDevice"/>; the sensor reports module type MZ.</summary>
public class Vl53l5cxDevice : Vl53l7cxDevice
{
    public Vl53l5cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Vl53l5cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Vl53l5cxDevice(DepzDevice from) : base(from) { }

    protected override int ExpectedModuleType => Vl53l8Driver.ModuleTypeMz;
}

/// <summary>VL53L7CH: the VL53L7CX superset with CNH histograms. <see cref="Vl53l7cxDevice.Init"/>
/// downloads the CH firmware (VL53LMZ ULD 2.0.16, the VL53L8CH one); CNH frames up to ~7.6 KB stream
/// in chunks.</summary>
public class Vl53l7chDevice : Vl53l7cxDevice
{
    public Vl53l7chDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Vl53l7chDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Vl53l7chDevice(DepzDevice from) : base(from) { }

    protected override Vl53l8Firmware FirmwareSet => Vl53l8Firmware.L7ch;

    /// <summary>The CNH setup armed by <see cref="ConfigureCnh"/>, or null.</summary>
    public Vl53l8CnhSetup? CnhSetup => ArmedCnh;

    /// <summary>Arm CNH for the next start (as <see cref="Vl53l8chDevice.ConfigureCnh"/>); disarms
    /// the motion indicator.</summary>
    public void ConfigureCnh(Vl53l8CnhSetup setup) => ArmCnh(setup);
}
