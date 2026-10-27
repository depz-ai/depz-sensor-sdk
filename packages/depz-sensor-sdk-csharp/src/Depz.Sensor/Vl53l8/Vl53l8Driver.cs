using System.Buffers.Binary;
using System.Globalization;
using System.Reflection;
using Depz.Sensor.Devices;

namespace Depz.Sensor.Vl53l8;

/// <summary>What the ULD needs from the board: register access through the bridge (16-bit
/// addresses, auto-increment) and a delay. <see cref="Devices.Vl53l8cxDevice"/> maps it onto the
/// firmware bridge; a test can script it.</summary>
public interface IVl53l8Platform
{
    byte[] ReadMulti(ushort addr, int size);

    void WriteMulti(ushort addr, byte[] data);

    void SleepMs(int ms);
}

/// <summary>The sensor firmware a <see cref="Vl53l8Driver"/> downloads.</summary>
public enum Vl53l8Firmware
{
    /// <summary>VL53L8CX, ST ULD 2.1.0.</summary>
    Cx,

    /// <summary>VL53L8CH, VL53LMZ ULD 2.0.16 (CNH).</summary>
    Ch,

    /// <summary>VL53L5CX / VL53L7CX over I2C, ULD 2.0.1.</summary>
    L7cx,

    /// <summary>VL53L7CH over I2C, VL53LMZ ULD 2.0.16 with the VL53L7 configuration.</summary>
    L7ch,
}

/// <summary>A VL53L8 / L5 / L7 ULD status (vl53l8cx_api.h codes) with where it happened.</summary>
public sealed class Vl53l8Exception : Exception
{
    public const int StatusTimeoutError = 1;
    public const int StatusCorruptedFrame = 2;
    public const int StatusLaserSafety = 3;
    public const int StatusFwChecksumFail = 5;
    public const int McuError = 66;
    public const int StatusInvalidParam = 127;
    public const int StatusError = 255;

    public int Code { get; }

    public Vl53l8Exception(int code, string where = "")
        : base($"VL53L8CX {NameOf(code)} (code {code}) {where}".TrimEnd()) => Code = code;

    private static string NameOf(int code) => code switch
    {
        StatusTimeoutError => "TIMEOUT",
        StatusCorruptedFrame => "CORRUPTED_FRAME",
        StatusLaserSafety => "LASER_SAFETY",
        StatusFwChecksumFail => "FW_CHECKSUM_FAIL",
        McuError => "MCU_ERROR",
        StatusInvalidParam => "INVALID_PARAM",
        StatusError => "ERROR",
        _ => $"0x{code:x}",
    };
}

/// <summary>The motion-indicator block of a frame (VL53L8CX_ResultsData MotionIndicator).</summary>
public sealed record Vl53l8Motion(
    uint GlobalIndicator1, uint GlobalIndicator2, int Status, int NbOfDetectedAggregates, int NbOfAggregates, uint[] Motion);

/// <summary>
/// The ST VL53L8CX ULD 2.1.0 / VL53LMZ ULD 2.0.16 / VL53L5CX-L7CX ULD 2.0.1 on
/// the host, ported from the reference Python driver: init (sensor firmware
/// download), resolution, frequency, mode, integration time, sharpener, target
/// order, ranging, frame parsing, power modes, xtalk margin and calibration,
/// detection thresholds, motion indicator. Register sequences are a 1:1 port —
/// every request matches the Python one, so captures replay in both.
/// </summary>
public sealed class Vl53l8Driver
{
    public const int Resolution4x4 = 16;
    public const int Resolution8x8 = 64;
    public const int RangingModeContinuous = 1;
    public const int RangingModeAutonomous = 3;
    public const int TargetOrderClosest = 1;
    public const int TargetOrderStrongest = 2;
    public const int PowerModeSleep = 0;
    public const int PowerModeWakeup = 1;
    public const int PowerModeDeepSleep = 2;

    public const int ModuleTypeMz = 0;      // VL53L5CX
    public const int ModuleTypeMzevo = 1;   // VL53L7CX / VL53L7CH
    public const int ModuleTypeMzplus = 2;  // VL53L8

    public const int NvmDataSize = 492;
    public const int OffsetBufferSize = 488;
    public const int XtalkBufferSize = 776;

    private const int NbTargetPerZone = 1;
    private const ushort UiCmdStatus = 0x2C00;
    private const ushort UiCmdStart = 0x2C04;
    private const ushort UiCmdEnd = 0x2FFF;

    private const int DciZoneConfig = 0x5450;
    private const int DciFreqHz = 0x5458;
    private const int DciIntTime = 0x545C;
    private const int DciRangingMode = 0xAD30;
    private const int DciDssConfig = 0xAD38;
    private const int DciTargetOrder = 0xAE64;
    private const int DciSharpener = 0xAED8;
    private const int DciSingleRange = 0xD964;
    private const int DciOutputConfig = 0xD968;
    private const int DciOutputEnables = 0xD970;
    private const int DciOutputList = 0xD980;
    private const int DciPipeControl = 0xDB80;
    private const int DciFwFlags = 0xE0C4;
    private const int DciCalCfg = 0x5470;
    private const int DciXtalkCfg = 0xAD94;
    private const int DciMotionDetectorCfg = 0xBFAC;
    private const int DciDetThreshConfig = 0x5488;
    private const int DciDetThreshGlobalConfig = 0xB6E0;
    private const int DciDetThreshStart = 0xB6E8;
    private const int DciDetThreshValidStatus = 0xB9F0;

    private const uint StartBh = 0x0000000D;
    private const uint MetadataBh = 0x54B400C0;
    private const uint CommondataBh = 0x54C00040;
    private const uint AmbientRateBh = 0x54D00104;
    private const uint SpadCountBh = 0x55D00404;
    private const uint NbTargetDetectedBh = 0xDB840401;
    private const uint SignalRateBh = 0xDBC40404;
    private const uint RangeSigmaMmBh = 0xDEC40402;
    private const uint DistanceBh = 0xDF440402;
    private const uint ReflectanceBh = 0xE0440401;
    private const uint TargetStatusBh = 0xE0840401;
    private const uint MotionDetectBh = 0xD85808C0;
    private const int CnhDataIdx = 0xC048;
    private const int MotionDetecIdx = 0xD858;

    private static readonly IReadOnlyDictionary<int, int> ThreshScale = new Dictionary<int, int>
    {
        [Vl53l8Advanced.DistMm] = 4,
        [Vl53l8Advanced.SignalPerSpadKcps] = 2048,
        [Vl53l8Advanced.RangeSigmaMm] = 128,
        [Vl53l8Advanced.AmbientPerSpadKcps] = 2048,
        [Vl53l8Advanced.NbSpadsEnabled] = 256,
        [Vl53l8Advanced.MotionIndicator] = 65535,
    };

    private readonly IVl53l8Platform _p;
    private readonly uint? _fwChecksum;
    private readonly uint _outputEnableW3;
    private readonly int _frameTail;
    private readonly Vl53l8FrameDecoder _decoder;
    private readonly byte[] _firmware;
    private readonly byte[] _defaultCfg;
    private readonly byte[] _defaultXtalk;
    private readonly byte[] _getNvmCmd;
    private byte[] _offsetData = Array.Empty<byte>();
    private byte[] _xtalkData = Array.Empty<byte>();
    private int _streamCount = 255;
    private bool _motionPresent;

    public Vl53l8Driver(IVl53l8Platform platform, Vl53l8Firmware firmware = Vl53l8Firmware.Cx)
    {
        _p = platform ?? throw new ArgumentNullException(nameof(platform));
        Firmware = firmware;
        IsI2c = firmware == Vl53l8Firmware.L7cx || firmware == Vl53l8Firmware.L7ch;
        _fwChecksum = firmware switch
        {
            Vl53l8Firmware.Cx => 0xCADF7CAF,
            Vl53l8Firmware.L7cx => null, // ULD 2.0.1 has no checksum step
            _ => 0x0C0B6C9E,
        };
        // Output enables word 3: 0 on the LMZ firmware; the frame tail and footer-id offset
        // differ between ULD 2.1.0 (cx) and the 2.0.x drivers.
        _outputEnableW3 = firmware == Vl53l8Firmware.Ch || firmware == Vl53l8Firmware.L7ch ? 0u : 0xC0000000u;
        _frameTail = firmware == Vl53l8Firmware.Cx ? 32 : 24;
        _decoder = new Vl53l8FrameDecoder(firmware == Vl53l8Firmware.Cx
            ? Vl53l8FrameDecoder.FooterIdOffsetCx
            : Vl53l8FrameDecoder.FooterIdOffsetCh);

        string set = SetName(firmware);
        string fwSet = firmware == Vl53l8Firmware.L7ch ? "ch" : set; // l7ch runs the ch firmware
        _firmware = Blob(fwSet, "firmware");
        _defaultCfg = Blob(set, "default_configuration");
        _defaultXtalk = Blob("cx", "default_xtalk"); // identical in every set
        _getNvmCmd = Blob("cx", "get_nvm_cmd");
        if (_firmware.Length != 0x15000)
            throw new InvalidStateException($"firmware.bin ({set}) has the wrong size: {_firmware.Length}");
    }

    public Vl53l8Firmware Firmware { get; }

    /// <summary>The L5/L7 boards: the sensor sits on I2C (other boot sequence, per-target blocks
    /// keep 64 entries in 4×4).</summary>
    public bool IsI2c { get; }

    /// <summary>L5/L7: MZ (VL53L5CX) or MZEVO (VL53L7CX/CH), read during <see cref="Init"/>; else null.</summary>
    public int? ModuleType { get; private set; }

    /// <summary>The frame size the firmware streams (bytes), set by <see cref="StartRanging"/>.</summary>
    public int DataReadSize { get; private set; }

    /// <summary>(firmware, host) frame sizes when the firmware reported another size than the
    /// ULD formula; the firmware value is used.</summary>
    public (int Firmware, int Host)? FrameSizeMismatch { get; private set; }

    /// <summary>Zones the last <see cref="StartRanging"/> used.</summary>
    public int? Resolution { get; private set; }

    /// <summary>The last <see cref="CalibrateXtalk"/> ran into "coverglass too good": the default
    /// xtalk buffer is kept.</summary>
    public bool XtalkCalibrationFailed { get; private set; }

    /// <summary>The 776-byte xtalk buffer the driver uploads (default, calibrated or restored).</summary>
    public byte[] XtalkData => _xtalkData.ToArray();

    private static string SetName(Vl53l8Firmware f) => f switch
    {
        Vl53l8Firmware.Cx => "cx",
        Vl53l8Firmware.Ch => "ch",
        Vl53l8Firmware.L7cx => "l7cx",
        _ => "l7ch",
    };

    private static byte[] Blob(string set, string name)
    {
        string res = $"Depz.Sensor.Vl53l8.{set}.{name}.bin";
        using var s = typeof(Vl53l8Driver).GetTypeInfo().Assembly.GetManifestResourceStream(res)
            ?? throw new InvalidStateException($"embedded resource {res} is missing");
        using var ms = new MemoryStream();
        s.CopyTo(ms);
        return ms.ToArray();
    }

    /// <summary>Whether the silicon ids are the sensor's: (0xF0, 0x0C) on L8 over SPI; revision
    /// 0x02, or 0x01 with device id 0xF0, on L5/L7 over I2C.</summary>
    public static bool SiliconIdMatches(int deviceId, int revisionId, bool i2c) =>
        i2c ? revisionId == 0x02 || (deviceId == 0xF0 && revisionId == 0x01)
            : deviceId == 0xF0 && revisionId == 0x0C;

    // ── low level ────────────────────────────────────────────────────────────

    private int Rd(ushort addr) => _p.ReadMulti(addr, 1)[0];

    private void Wr(ushort addr, int value) => _p.WriteMulti(addr, new[] { (byte)value });

    // _vl53l8cx_poll_for_answer: until buf[pos] & mask == expected; 10 ms period, 2 s timeout.
    private void PollForAnswer(int size, int pos, ushort address, int mask, int expected, string where)
    {
        int timeout = 0;
        while (true)
        {
            byte[] buf = _p.ReadMulti(address, size);
            _p.SleepMs(10);
            if (timeout >= 200)
                throw new Vl53l8Exception(Vl53l8Exception.StatusTimeoutError, where);
            if (size >= 4 && buf[2] >= 0x7F)
                throw new Vl53l8Exception(Vl53l8Exception.McuError, where);
            timeout++;
            if ((buf[pos] & mask) == expected)
                return;
        }
    }

    private void PollForMcuBoot()
    {
        for (int timeout = 0; timeout < 500; timeout++)
        {
            int go2Status0 = Rd(0x06);
            if ((go2Status0 & 0x80) != 0 && (Rd(0x07) & 0x01) != 0)
                return;
            _p.SleepMs(1);
            if ((go2Status0 & 0x01) != 0)
                return;
        }
        throw new Vl53l8Exception(Vl53l8Exception.StatusTimeoutError, "mcu boot");
    }

    // ── DCI access ───────────────────────────────────────────────────────────

    public byte[] DciReadData(int index, int dataSize)
    {
        var cmd = new byte[12];
        cmd[0] = (byte)(index >> 8);
        cmd[1] = (byte)index;
        cmd[2] = (byte)((dataSize & 0xFF0) >> 4);
        cmd[3] = (byte)((dataSize & 0xF) << 4);
        cmd[7] = 0x0F;
        cmd[9] = 0x02;
        cmd[11] = 0x08;
        _p.WriteMulti(UiCmdEnd - 11, cmd);
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, $"dci read 0x{index:X4}");
        byte[] buf = Vl53l8FrameDecoder.SwapBuffer(_p.ReadMulti(UiCmdStart, dataSize + 12));
        var data = new byte[dataSize];
        Array.Copy(buf, 4, data, 0, dataSize);
        return data;
    }

    public void DciWriteData(int index, byte[] data)
    {
        int n = data.Length;
        var buf = new byte[n + 12];
        buf[0] = (byte)(index >> 8);
        buf[1] = (byte)index;
        buf[2] = (byte)((n & 0xFF0) >> 4);
        buf[3] = (byte)((n & 0xF) << 4);
        Vl53l8FrameDecoder.SwapBuffer(data).CopyTo(buf, 4);
        new byte[] { 0x00, 0x00, 0x00, 0x0F, 0x05, 0x01, (byte)((n + 8) >> 8), (byte)(n + 8) }.CopyTo(buf, 4 + n);
        _p.WriteMulti((ushort)(UiCmdEnd - (n + 12) + 1), buf);
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, $"dci write 0x{index:X4}");
    }

    public void DciReplaceData(int index, int dataSize, byte[] newData, int newDataPos)
    {
        byte[] data = DciReadData(index, dataSize);
        Array.Copy(newData, 0, data, newDataPos, newData.Length);
        DciWriteData(index, data);
    }

    // ── offset / xtalk upload ────────────────────────────────────────────────

    // 8x8 → 4x4: each 4x4 zone is the mean of its 2x2 block (C integer division).
    private static void Average4x4<T>(T[] grid, Func<T, long> get, Func<long, T> make)
    {
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++)
                grid[i + 4 * j] = make(FloorDiv(
                    get(grid[2 * i + 16 * j]) + get(grid[2 * i + 16 * j + 1])
                    + get(grid[2 * i + 16 * j + 8]) + get(grid[2 * i + 16 * j + 9]), 4));
        for (int k = 16; k < 64; k++)
            grid[k] = make(0);
    }

    private static long FloorDiv(long a, long b) => a / b - ((a % b != 0) && ((a < 0) != (b < 0)) ? 1 : 0);

    private void SendOffsetData(int resolution)
    {
        var buf = new byte[OffsetBufferSize];
        Array.Copy(_offsetData, buf, Math.Min(OffsetBufferSize, _offsetData.Length));
        if (resolution == Resolution4x4)
        {
            new byte[] { 0x0F, 0x04, 0x04, 0x00, 0x08, 0x10, 0x10, 0x07 }.CopyTo(buf, 0x10);
            buf = Vl53l8FrameDecoder.SwapBuffer(buf);
            var signal = new uint[64];
            var range = new short[64];
            for (int k = 0; k < 64; k++)
            {
                signal[k] = BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(0x3C + 4 * k));
                range[k] = BinaryPrimitives.ReadInt16LittleEndian(buf.AsSpan(0x140 + 2 * k));
            }
            Average4x4(signal, v => v, v => (uint)v);
            Average4x4(range, v => v, v => (short)Math.Max(-32768, Math.Min(32767, v)));
            for (int k = 0; k < 64; k++)
            {
                BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(0x3C + 4 * k), signal[k]);
                BinaryPrimitives.WriteInt16LittleEndian(buf.AsSpan(0x140 + 2 * k), range[k]);
            }
            buf = Vl53l8FrameDecoder.SwapBuffer(buf);
        }
        // Drop the 8-byte NVM header and append the footer at 0x1E0.
        var output = new byte[OffsetBufferSize];
        Array.Copy(buf, 8, output, 0, OffsetBufferSize - 8);
        new byte[] { 0x00, 0x00, 0x00, 0x0F, 0x03, 0x01, 0x01, 0xE4 }.CopyTo(output, OffsetBufferSize - 8);
        _p.WriteMulti(0x2E18, output);
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "offset data");
    }

    private void SendXtalkData(int resolution)
    {
        var buf = new byte[XtalkBufferSize];
        Array.Copy(_xtalkData, buf, Math.Min(XtalkBufferSize, _xtalkData.Length));
        if (resolution == Resolution4x4)
        {
            new byte[] { 0x0F, 0x04, 0x04, 0x17, 0x08, 0x10, 0x10, 0x07 }.CopyTo(buf, 0x08);
            new byte[] { 0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08 }.CopyTo(buf, 0x20);
            buf = Vl53l8FrameDecoder.SwapBuffer(buf);
            var signal = new uint[64];
            for (int k = 0; k < 64; k++)
                signal[k] = BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(0x34 + 4 * k));
            Average4x4(signal, v => v, v => (uint)v);
            for (int k = 0; k < 64; k++)
                BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(0x34 + 4 * k), signal[k]);
            buf = Vl53l8FrameDecoder.SwapBuffer(buf);
            new byte[] { 0xA0, 0xFC, 0x01, 0x00 }.CopyTo(buf, 0x134);
            Array.Clear(buf, 0x078, 4);
        }
        _p.WriteMulti(0x2CF8, buf);
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "xtalk data");
    }

    // ── identity and init ────────────────────────────────────────────────────

    /// <summary>(device id, revision id) of the silicon; <see cref="SiliconIdMatches"/> tells
    /// whether they are the sensor's.</summary>
    public (int DeviceId, int RevisionId) IsAlive()
    {
        Wr(0x7FFF, 0x00);
        int deviceId = Rd(0x00);
        int revisionId = Rd(0x01);
        Wr(0x7FFF, 0x02);
        return (deviceId, revisionId);
    }

    /// <summary>vl53l8cx_init: boot the sensor MCU, download the 84 KB sensor firmware, upload the
    /// NVM offsets, the default xtalk and the default configuration. <paramref name="progress"/>
    /// gets phase texts.</summary>
    public void Init(Action<string>? progress = null)
    {
        void Note(string text) => progress?.Invoke(text);

        ModuleType = null;
        if (IsI2c)
        {
            var (deviceId, revisionId) = IsAlive();
            if (!SiliconIdMatches(deviceId, revisionId, true))
                throw new Vl53l8Exception(Vl53l8Exception.StatusError,
                    $"not a VL53L5/L7 sensor: device_id=0x{deviceId:X2} revision_id=0x{revisionId:X2}");
        }

        Note("SW reboot...");
        Wr(0x7FFF, 0x00);
        Wr(0x0009, 0x04);
        Wr(0x000F, 0x40);
        Wr(0x000A, 0x03);
        Rd(0x7FFF);
        Wr(0x000C, 0x01);

        Wr(0x0101, 0x00);
        Wr(0x0102, 0x00);
        Wr(0x010A, 0x01);
        Wr(0x4002, 0x01);
        Wr(0x4002, 0x00);
        Wr(0x010A, 0x03);
        Wr(0x0103, 0x01);
        Wr(0x000C, 0x00);
        Wr(0x000F, 0x43);
        _p.SleepMs(1);

        Wr(0x000F, 0x40);
        Wr(0x000A, 0x01);
        _p.SleepMs(100);

        Note("Waiting for sensor boot...");
        Wr(0x7FFF, 0x00);
        PollForAnswer(1, 0, 0x06, 0xFF, 1, "sensor boot");

        Wr(0x000E, 0x01);
        Wr(0x7FFF, 0x02);

        // Enable FW access
        if (IsI2c)
        {
            Wr(0x03, 0x0D);
            Wr(0x7FFF, 0x01);
            PollForAnswer(1, 0, 0x21, 0x10, 0x10, "fw access");
        }
        else
        {
            Wr(0x7FFF, 0x01);
            Wr(0x06, 0x01);
            PollForAnswer(1, 0, 0x21, 0xFF, 0x04, "fw access");
        }

        Wr(0x7FFF, 0x00);

        // Enable host access to GO1
        Rd(0x7FFF);
        Wr(0x0C, 0x01);

        // Power ON status
        Wr(0x7FFF, 0x00);
        Wr(0x101, 0x00);
        Wr(0x102, 0x00);
        Wr(0x010A, 0x01);
        Wr(0x4002, 0x01);
        Wr(0x4002, 0x00);
        Wr(0x010A, 0x03);
        Wr(0x103, 0x01);
        Wr(0x400F, 0x00);
        Wr(0x21A, 0x43);
        Wr(0x21A, 0x03);
        Wr(0x21A, 0x01);
        Wr(0x21A, 0x00);
        Wr(0x219, 0x00);
        Wr(0x21B, 0x00);

        // Wake up MCU
        Wr(0x7FFF, 0x00);
        Rd(0x7FFF);
        if (IsI2c)
            Wr(0x0C, 0x00);
        Wr(0x7FFF, 0x01);
        if (IsI2c)
        {
            Wr(0x20, 0x07);
            Wr(0x20, 0x06);
        }

        // Download FW into the sensor
        Note("Downloading sensor FW (84 KB)... bank 1/3");
        Wr(0x7FFF, 0x09);
        _p.WriteMulti(0, Slice(_firmware, 0, 0x8000));
        Note("Downloading sensor FW... bank 2/3");
        Wr(0x7FFF, 0x0A);
        _p.WriteMulti(0, Slice(_firmware, 0x8000, 0x8000));
        Note("Downloading sensor FW... bank 3/3");
        Wr(0x7FFF, 0x0B);
        _p.WriteMulti(0, Slice(_firmware, 0x10000, 0x5000));
        Wr(0x7FFF, 0x01);

        // Check if FW correctly downloaded
        if (IsI2c)
        {
            Wr(0x7FFF, 0x02);
            Wr(0x03, 0x0D);
            Wr(0x7FFF, 0x01);
            PollForAnswer(1, 0, 0x21, 0x10, 0x10, "fw downloaded");
        }
        else
        {
            Wr(0x7FFF, 0x01);
            Wr(0x06, 0x03);
            _p.SleepMs(5);
        }

        Wr(0x7FFF, 0x00);
        Rd(0x7FFF);
        Wr(0x0C, 0x01);

        // Reset MCU and wait boot
        Note("Booting sensor MCU...");
        Wr(0x7FFF, 0x00);
        Wr(0x114, 0x00);
        Wr(0x115, 0x00);
        Wr(0x116, 0x42);
        Wr(0x117, 0x00);
        Wr(0x0B, 0x00);
        Rd(0x7FFF);
        Wr(0x0C, 0x00);
        Wr(0x0B, 0x01);

        PollForMcuBoot();

        Wr(0x7FFF, 0x02);

        if (_fwChecksum is uint expected)
        {
            uint crc = BinaryPrimitives.ReadUInt32LittleEndian(Vl53l8FrameDecoder.SwapBuffer(_p.ReadMulti(0x2FFC, 4)));
            if (crc != expected)
                throw new Vl53l8Exception(Vl53l8Exception.StatusFwChecksumFail,
                    $"crc=0x{crc:X8} (expected 0x{expected:X8} for {SetName(Firmware)})");
            Note("Sensor FW checksum OK");
        }

        Note("Reading NVM offset data...");
        _p.WriteMulti(0x2FD8, _getNvmCmd);
        PollForAnswer(4, 0, UiCmdStatus, 0xFF, 2, "nvm read");
        _offsetData = Slice(_p.ReadMulti(UiCmdStart, NvmDataSize), 0, OffsetBufferSize);
        SendOffsetData(Resolution4x4);

        Note("Uploading default xtalk...");
        _xtalkData = _defaultXtalk;
        SendXtalkData(Resolution4x4);

        Note("Uploading default configuration...");
        _p.WriteMulti(0x2C34, _defaultCfg);
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "default config");

        DciWriteData(DciPipeControl, new byte[] { NbTargetPerZone, 0x00, 0x01, 0x00 });
        DciWriteData(DciSingleRange, U32(0x01));
        if (IsI2c)
        {
            // Readable once the sensor FW runs; tells L5 (MZ) from L7 (MZEVO): one blob, one board.
            ModuleType = DciReadData(DciFwFlags, 8)[1];
            Note($"Sensor init done (module type {ModuleType switch { ModuleTypeMz => "MZ", ModuleTypeMzevo => "MZEVO", ModuleTypeMzplus => "MZPlus", _ => ModuleType?.ToString(CultureInfo.InvariantCulture) }})");
        }
        else
        {
            Note("Sensor init done");
        }
    }

    private static byte[] Slice(byte[] b, int offset, int count)
    {
        var s = new byte[count];
        Array.Copy(b, offset, s, 0, count);
        return s;
    }

    private static byte[] U32(uint v)
    {
        var b = new byte[4];
        BinaryPrimitives.WriteUInt32LittleEndian(b, v);
        return b;
    }

    private static byte[] U32s(IReadOnlyList<uint> words)
    {
        var b = new byte[words.Count * 4];
        for (int i = 0; i < words.Count; i++)
            BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(4 * i), words[i]);
        return b;
    }

    // ── configuration ────────────────────────────────────────────────────────

    public int GetResolution()
    {
        byte[] buf = DciReadData(DciZoneConfig, 8);
        return buf[0] * buf[1];
    }

    public void SetResolution(int resolution)
    {
        if (resolution == Resolution4x4 || resolution == Resolution8x8)
        {
            bool four = resolution == Resolution4x4;
            byte[] buf = DciReadData(DciDssConfig, 16);
            buf[0x04] = (byte)(four ? 64 : 16);
            buf[0x06] = (byte)(four ? 64 : 16);
            buf[0x09] = (byte)(four ? 4 : 1);
            DciWriteData(DciDssConfig, buf);
            buf = DciReadData(DciZoneConfig, 8);
            buf[0x00] = (byte)(four ? 4 : 8);
            buf[0x01] = (byte)(four ? 4 : 8);
            buf[0x04] = (byte)(four ? 8 : 4);
            buf[0x05] = (byte)(four ? 8 : 4);
            DciWriteData(DciZoneConfig, buf);
        }
        else
        {
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_resolution");
        }
        SendOffsetData(resolution);
        SendXtalkData(resolution);
    }

    public int GetRangingFrequencyHz() => DciReadData(DciFreqHz, 4)[1];

    public void SetRangingFrequencyHz(int hz)
    {
        if (hz < 0 || hz > 255)
            throw new ArgumentOutOfRangeException(nameof(hz), hz, "ranging frequency is a u8 field (0..255 Hz)");
        DciReplaceData(DciFreqHz, 4, new[] { (byte)hz }, 0x01);
    }

    public void SetRangingMode(int mode)
    {
        byte[] buf = DciReadData(DciRangingMode, 8);
        uint singleRange;
        if (mode == RangingModeContinuous)
        {
            buf[0x01] = 0x1;
            buf[0x03] = 0x3;
            singleRange = 0x00;
        }
        else if (mode == RangingModeAutonomous)
        {
            buf[0x01] = 0x3;
            buf[0x03] = 0x2;
            singleRange = 0x01;
        }
        else
        {
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_ranging_mode");
        }
        DciWriteData(DciRangingMode, buf);
        DciWriteData(DciSingleRange, U32(singleRange));
    }

    public int GetRangingMode() =>
        DciReadData(DciRangingMode, 8)[0x01] == 0x1 ? RangingModeContinuous : RangingModeAutonomous;

    public int GetIntegrationTimeMs() =>
        (int)(BinaryPrimitives.ReadUInt32LittleEndian(DciReadData(DciIntTime, 20)) / 1000);

    /// <summary>2..1000 ms; no effect in continuous mode.</summary>
    public void SetIntegrationTimeMs(int timeMs)
    {
        if (timeMs < 2 || timeMs > 1000)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_integration_time_ms");
        DciReplaceData(DciIntTime, 20, U32((uint)(timeMs * 1000)), 0x00);
    }

    /// <summary>0..99 %, rounded to nearest on the way back (ST truncates and loses a count for
    /// 95 of the 100 legal values; the stored byte is the same).</summary>
    public int GetSharpenerPercent() =>
        (int)Math.Round(DciReadData(DciSharpener, 16)[0xD] * 100 / 255.0, MidpointRounding.ToEven);

    public void SetSharpenerPercent(int pct)
    {
        if (pct >= 100)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_sharpener_percent");
        if (pct < 0)
            throw new ArgumentOutOfRangeException(nameof(pct), pct, "sharpener is 0..99 %");
        DciReplaceData(DciSharpener, 16, new[] { (byte)(pct * 255 / 100) }, 0xD);
    }

    public int GetTargetOrder() => DciReadData(DciTargetOrder, 4)[0];

    public void SetTargetOrder(int order)
    {
        if (order != TargetOrderClosest && order != TargetOrderStrongest)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_target_order");
        DciReplaceData(DciTargetOrder, 4, new[] { (byte)order }, 0x0);
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    private static (int Type, int Size, int Idx) BhFields(uint bh) =>
        ((int)(bh & 0xF), (int)((bh >> 4) & 0xFFF), (int)((bh >> 16) & 0xFFFF));

    private static uint BhSetSize(uint bh, int size) => (bh & ~0xFFF0u) | ((uint)(size & 0xFFF) << 4);

    /// <summary>Program the output list and start. <paramref name="cnhDataSize"/> (bytes, from
    /// <see cref="Vl53l8CnhSetup.RequiredMemory"/>) appends the CNH block to every frame.</summary>
    public void StartRanging(int? cnhDataSize = null)
    {
        int resolution = GetResolution();
        // A zero / odd resolution sizes the output config to nothing and faults the sensor MCU.
        if (resolution != Resolution4x4 && resolution != Resolution8x8)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam,
                $"bad resolution {resolution} — call set_resolution first");
        DataReadSize = 0;
        _streamCount = 255;

        var enable = new uint[] { 0x00000FFF, 0, 0, _outputEnableW3 };
        var output = new List<uint>
        {
            StartBh, MetadataBh, CommondataBh, AmbientRateBh, SpadCountBh, NbTargetDetectedBh,
            SignalRateBh, RangeSigmaMmBh, DistanceBh, ReflectanceBh, TargetStatusBh, MotionDetectBh,
        };
        if (cnhDataSize is int cnh)
        {
            // vl53lmz_add_output_block: CNH block, type 4, size in 32-bit words.
            output.Add(((uint)CnhDataIdx << 16) | ((uint)((cnh / 4) & 0xFFF) << 4) | 4);
            enable[0] |= 1u << (output.Count - 1);
        }

        int size = 0;
        for (int i = 0; i < output.Count; i++)
        {
            if (output[i] == 0 || (enable[i / 32] & (1u << (i % 32))) == 0)
                continue;
            var (type, bhSize, idx) = BhFields(output[i]);
            if (type >= 0x1 && type < 0x0D)
            {
                if (IsI2c)
                {
                    // vl53lmz_start_ranging (2.0.16), as on L5/L7 hardware: per-target blocks above
                    // 0x6C90 keep their 64-entry size even in 4x4 (the frame parser trims them).
                    if (idx >= 0x54D0 && idx < 0x5890)
                        bhSize = resolution;
                    else if (idx < 0x6C90)
                        bhSize = resolution * NbTargetPerZone;
                }
                else if (idx >= 0x54D0 && idx < 0x54D0 + 960)
                {
                    bhSize = resolution;
                }
                else if (idx != CnhDataIdx) // the CNH block keeps its size
                {
                    bhSize = resolution * NbTargetPerZone;
                }
                output[i] = BhSetSize(output[i], bhSize);
                size += type * bhSize;
            }
            else
            {
                size += bhSize;
            }
            size += 4;
        }
        DataReadSize = size + _frameTail;
        Resolution = resolution;

        DciWriteData(DciOutputList, U32s(output));
        DciWriteData(DciOutputConfig, U32s(new[] { (uint)DataReadSize, (uint)(output.Count + 1) }));
        DciWriteData(DciOutputEnables, U32s(enable));

        // Start xshut bypass (interrupt mode)
        Wr(0x7FFF, 0x00);
        Wr(0x09, 0x05);
        Wr(0x7FFF, 0x02);

        // Start the ranging session
        _p.WriteMulti(UiCmdEnd - 3, new byte[] { 0x00, 0x03, 0x00, 0x00 });
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "start ranging");

        // The firmware tells the frame size it streams; the released blobs report 4 bytes less
        // than the C formula, so trust the firmware value.
        int fwSize = BinaryPrimitives.ReadUInt16LittleEndian(DciReadData(0x5440, 12).AsSpan(0x8));
        FrameSizeMismatch = null;
        if (fwSize != DataReadSize)
        {
            FrameSizeMismatch = (fwSize, DataReadSize);
            DataReadSize = fwSize;
        }

        if (DciReadData(DciFwFlags, 8)[0x6] != 0)
            throw new Vl53l8Exception(Vl53l8Exception.StatusLaserSafety);
    }

    public void StopRanging()
    {
        uint autoStopFlag = BinaryPrimitives.ReadUInt32LittleEndian(_p.ReadMulti(0x2FFC, 4));
        if (autoStopFlag != 0x4FF)
        {
            Wr(0x7FFF, 0x00);
            // Provoke MCU stop
            Wr(0x15, 0x16);
            Wr(0x14, 0x01);
            // Poll for G02 status 0 MCU stop
            int tmp = 0;
            int timeout = 0;
            while (((tmp & 0x80) >> 7) == 0)
            {
                tmp = Rd(0x06);
                _p.SleepMs(10);
                timeout++;
                if (timeout > 500)
                    break;
            }
        }
        // GO2 status 1: the C driver ORs a 0x84/0x85 mismatch into its status; non-fatal here.
        if ((Rd(0x06) & 0x80) != 0)
            Rd(0x07);
        // Undo MCU stop
        Wr(0x7FFF, 0x00);
        Wr(0x14, 0x00);
        Wr(0x15, 0x00);
        // Stop xshut bypass
        Wr(0x09, 0x04);
        Wr(0x7FFF, 0x02);
    }

    /// <summary>Poll mode: a new frame is ready.</summary>
    public bool CheckDataReady()
    {
        byte[] buf = _p.ReadMulti(0x0, 4);
        if (buf[0] != _streamCount && buf[0] != 255 && buf[1] == 0x05
            && (buf[2] & 0x05) == 0x05 && (buf[3] & 0x10) == 0x10)
        {
            _streamCount = buf[0];
            return true;
        }
        if ((buf[3] & 0x80) != 0)
            throw new Vl53l8Exception(buf[2], "GO2 error status");
        return false;
    }

    /// <summary>Poll mode: read one frame from register 0x00 and parse it.</summary>
    public Vl53l8Frame GetRangingData(ulong timestampUs = 0) => ParseFrame(timestampUs, _p.ReadMulti(0x0, DataReadSize));

    /// <summary>Parse one raw frame (<see cref="DataReadSize"/> bytes) — polled, or pushed by the
    /// board's stream. L5/L7 frames are trimmed to the zones in use.</summary>
    /// <exception cref="Vl53l8FrameDecoder.CorruptedFrameException">Header / footer ids differ.</exception>
    public Vl53l8Frame ParseFrame(ulong timestampUs, byte[] raw)
    {
        if (raw.Length > 0)
            _streamCount = raw[0];
        Vl53l8Frame frame = IsI2c && Resolution is int n
            ? _decoder.ParseFrame(timestampUs, raw, n)
            : _decoder.ParseFrame(timestampUs, raw);
        return _motionPresent ? frame with { Motion = ParseMotion(Vl53l8FrameDecoder.SwapBuffer(raw)) } : frame;
    }

    private static Vl53l8Motion? ParseMotion(byte[] buf)
    {
        int drs = buf.Length;
        int i = 16;
        while (i + 4 <= drs)
        {
            var (t, s, idx) = BhFields(BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(i)));
            int msize = t > 0x1 && t < 0xD ? t * s : s;
            if (i + 4 + msize > drs)
                break;
            if (idx == MotionDetecIdx)
            {
                var motion = new uint[32];
                for (int k = 0; k < 32; k++)
                    motion[k] = BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(i + 16 + 4 * k));
                return new Vl53l8Motion(
                    BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(i + 4)),
                    BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(i + 8)),
                    buf[i + 12], buf[i + 13], buf[i + 14], motion);
            }
            i += msize + 4;
        }
        return null;
    }

    // ── power modes ──────────────────────────────────────────────────────────

    public int GetPowerMode()
    {
        Wr(0x7FFF, 0x00);
        int tmp = Rd(0x09);
        int mode;
        if (tmp == 0x04)
        {
            mode = PowerModeWakeup;
        }
        else if (tmp == 0x02)
        {
            // ULD 2.0.1 (l7cx) has no deep sleep and never reads 0x000F.
            mode = Firmware == Vl53l8Firmware.L7cx ? PowerModeSleep
                : Rd(0x000F) == 0x43 ? PowerModeDeepSleep : PowerModeSleep;
        }
        else
        {
            Wr(0x7FFF, 0x02);
            throw new Vl53l8Exception(Vl53l8Exception.StatusError, "get_power_mode");
        }
        Wr(0x7FFF, 0x02);
        return mode;
    }

    /// <summary>Wakeup, sleep or deep sleep (not on L5CX/L7CX). Waking from deep sleep re-runs
    /// <see cref="Init"/>: the sensor firmware is lost.</summary>
    public void SetPowerMode(int powerMode)
    {
        if (Firmware == Vl53l8Firmware.L7cx && powerMode != PowerModeSleep && powerMode != PowerModeWakeup)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_power_mode: VL53L5CX/L7CX have no deep sleep");
        int current = GetPowerMode();
        if (powerMode == current)
            return;
        if (powerMode == PowerModeWakeup && Firmware == Vl53l8Firmware.L7cx)
        {
            Wr(0x7FFF, 0x00);
            Wr(0x09, 0x04);
            PollForAnswer(1, 0, 0x06, 0x01, 1, "wakeup");
            Wr(0x7FFF, 0x02);
        }
        else if (powerMode == PowerModeWakeup)
        {
            Wr(0x7FFF, 0x00);
            Wr(0x09, 0x04);
            int stored = Rd(0x000F);
            if (stored == 0x43)
                Wr(0x000F, 0x40);
            PollForAnswer(1, 0, 0x06, 0x01, 1, "wakeup");
            Wr(0x7FFF, 0x02);
            if (stored == 0x43)
                Init();
        }
        else if (powerMode == PowerModeSleep)
        {
            Wr(0x7FFF, 0x00);
            Wr(0x09, 0x02);
            PollForAnswer(1, 0, 0x06, 0x01, 0, "sleep");
            Wr(0x7FFF, 0x02);
        }
        else if (powerMode == PowerModeDeepSleep)
        {
            Wr(0x7FFF, 0x00);
            Wr(0x09, 0x02);
            PollForAnswer(1, 0, 0x06, 0x01, 0, "deep sleep");
            Wr(0x000F, 0x43);
            Wr(0x7FFF, 0x02);
        }
        else
        {
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_power_mode");
        }
    }

    // ── xtalk ────────────────────────────────────────────────────────────────

    /// <summary>Xtalk margin, whole kcps/SPAD (raw / 2048 rounded down, as ST).</summary>
    public int GetXtalkMargin() =>
        Vl53l8Advanced.XtalkMarginKcps(BinaryPrimitives.ReadUInt32LittleEndian(DciReadData(DciXtalkCfg, 16)));

    /// <summary>Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); anything else is INVALID_PARAM, nothing written.</summary>
    public void SetXtalkMargin(int marginKcps)
    {
        if (marginKcps < 0 || marginKcps > Vl53l8Advanced.XtalkMarginMaxKcps)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "set_xtalk_margin");
        DciReplaceData(DciXtalkCfg, 16, U32(Vl53l8Advanced.XtalkMarginRaw(marginKcps)), 0x00);
    }

    private byte[] ReadXtalkBuffer()
    {
        _p.WriteMulti(0x2FB8, Vl53l8Tables.GetXtalkCmd.ToArray());
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "get xtalk");
        byte[] buf = _p.ReadMulti(UiCmdStart, XtalkBufferSize + 4);
        var output = new byte[XtalkBufferSize];
        Array.Copy(buf, 8, output, 0, XtalkBufferSize - 8);
        new byte[] { 0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04 }.CopyTo(output, XtalkBufferSize - 8);
        return output;
    }

    /// <summary>The live 776-byte xtalk calibration buffer (save / restore); the resolution is
    /// restored afterwards.</summary>
    public byte[] GetCaldataXtalk()
    {
        int resolution = GetResolution();
        SetResolution(Resolution8x8);
        _xtalkData = ReadXtalkBuffer();
        SetResolution(resolution);
        return _xtalkData.ToArray();
    }

    /// <summary>Restore a saved 776-byte xtalk buffer; it is uploaded again by the next
    /// resolution change and at once here.</summary>
    public void SetCaldataXtalk(byte[] xtalkData)
    {
        if (xtalkData is null || xtalkData.Length != XtalkBufferSize)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, $"xtalk blob must be {XtalkBufferSize} bytes");
        int resolution = GetResolution();
        _xtalkData = xtalkData.ToArray();
        SetResolution(resolution);
    }

    // ── detection thresholds ─────────────────────────────────────────────────

    public int GetDetectionThresholdsEnable() => DciReadData(DciDetThreshGlobalConfig, 8)[1];

    public void SetDetectionThresholdsEnable(bool enabled)
    {
        var grp = new byte[] { 0x01, (byte)(enabled ? 0x01 : 0x00), 0x01, 0x00 };
        DciReplaceData(DciDetThreshGlobalConfig, 8, grp, 0x00);
        DciReplaceData(DciDetThreshConfig, 20, new[] { (byte)(enabled ? 0x04 : 0x0C) }, 0x11);
    }

    /// <summary>The 64 thresholds, low / high rescaled to the units of their measurement.</summary>
    public IReadOnlyList<Vl53l8Advanced.DetectionThreshold> GetDetectionThresholds()
    {
        byte[] raw = DciReadData(DciDetThreshStart, Vl53l8Advanced.NbThresholds * 12);
        var result = new List<Vl53l8Advanced.DetectionThreshold>();
        for (int k = 0; k < Vl53l8Advanced.NbThresholds; k++)
        {
            int b = k * 12;
            int meas = raw[b + 8];
            int scale = ThreshScale.TryGetValue(meas, out int sc) ? sc : 1;
            result.Add(new Vl53l8Advanced.DetectionThreshold(
                (int)FloorDiv(BinaryPrimitives.ReadInt32LittleEndian(raw.AsSpan(b)), scale),
                (int)FloorDiv(BinaryPrimitives.ReadInt32LittleEndian(raw.AsSpan(b + 4)), scale),
                meas, raw[b + 9], raw[b + 10], raw[b + 11]));
        }
        return result;
    }

    /// <summary>Program the 64 thresholds (missing entries are zeros; more than 64 is refused).</summary>
    public void SetDetectionThresholds(IReadOnlyList<Vl53l8Advanced.DetectionThreshold> thresholds)
    {
        byte[] packed = Vl53l8Advanced.PackDetectionThresholds(thresholds);
        DciWriteData(DciDetThreshValidStatus, Vl53l8Advanced.ThresholdValidStatus());
        DciWriteData(DciDetThreshStart, packed);
    }

    public void SetDetectionThresholdsAutoStop(bool autoStop)
    {
        if (Firmware == Vl53l8Firmware.L7cx)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "detection-threshold auto-stop: not in the L5CX/L7CX ULD");
        DciReplaceData(DciPipeControl, 4, new[] { (byte)(autoStop ? 1 : 0) }, 0x03);
    }

    // ── motion indicator ─────────────────────────────────────────────────────

    /// <summary>A default motion-indicator configuration for <paramref name="resolution"/>, written
    /// to the sensor; frames carry motion output from here on.</summary>
    public MotionConfig MotionIndicatorInit(int resolution)
    {
        var cfg = MotionConfig.InitDefault(Resolution4x4);
        MotionIndicatorSetResolution(cfg, resolution);
        _motionPresent = true;
        return cfg;
    }

    /// <summary>Stop surfacing the motion block: frames parse with <c>Motion</c> null. Called when
    /// the VL53LMZ CNH setup overwrites the shared DCI 0xBFAC struct (contract 04 §7, ERRATA B4).</summary>
    public void MotionIndicatorDisarm() => _motionPresent = false;

    public void MotionIndicatorSetResolution(MotionConfig cfg, int resolution)
    {
        if (resolution != Resolution4x4 && resolution != Resolution8x8)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "motion set_resolution");
        cfg.SetResolution(resolution);
        DciWriteData(DciMotionDetectorCfg, cfg.Pack());
    }

    public void MotionIndicatorSetDistanceMotion(MotionConfig cfg, int distanceMinMm, int distanceMaxMm)
    {
        if (distanceMaxMm - distanceMinMm > 1500 || distanceMinMm < 400 || distanceMaxMm > 4000)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "motion set_distance");
        cfg.RefBinOffset = (int)((distanceMinMm / 37.5348 - 4.0) * 2048.5);
        cfg.FeatureLength = (byte)(int)(((distanceMaxMm - distanceMinMm) / 10.0 + 30.02784) / 15.01392 + 0.5);
        DciWriteData(DciMotionDetectorCfg, cfg.Pack());
    }

    // ── xtalk calibration run ────────────────────────────────────────────────

    // _vlxxx_program_output_config of the ST xtalk plugin (every version): the 17-block
    // calibration output list, sized with the 0x54D0+960 rule, 24-byte tail.
    private void ProgramXtalkOutputConfig(int resolution)
    {
        var output = Vl53l8Tables.XtalkCalOutput.ToArray();
        var enable = Vl53l8Tables.XtalkCalOutputEnable.ToArray();
        int size = 0;
        for (int i = 0; i < output.Length; i++)
        {
            if (output[i] == 0 || (enable[i / 32] & (1u << (i % 32))) == 0)
                continue;
            var (type, bhSize, idx) = BhFields(output[i]);
            if (type >= 0x1 && type < 0x0D)
            {
                bhSize = idx >= 0x54D0 && idx < 0x54D0 + 960 ? resolution : resolution * NbTargetPerZone;
                output[i] = BhSetSize(output[i], bhSize);
                size += type * bhSize;
            }
            else
            {
                size += bhSize;
            }
            size += 4;
        }
        size += 24;
        DciWriteData(DciOutputList, U32s(output));
        DciWriteData(DciOutputConfig, U32s(new[] { (uint)size, (uint)(output.Length + 1) }));
        DciWriteData(DciOutputEnables, U32s(enable));
    }

    /// <summary>vl53l8cx_calibrate_xtalk: on-device crosstalk calibration against a flat target at
    /// <paramref name="distanceMm"/> (600..3000) of <paramref name="reflectancePercent"/> (1..99),
    /// averaging <paramref name="nbSamples"/> (1..16). Takes seconds; the configuration is saved
    /// and restored around it. Without cover glass the firmware answers "coverglass too good":
    /// <see cref="XtalkCalibrationFailed"/>, and the default buffer stays.</summary>
    public void CalibrateXtalk(int reflectancePercent, int nbSamples, int distanceMm)
    {
        if (reflectancePercent < 1 || reflectancePercent > 99)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "reflectance 1..99");
        if (distanceMm < 600 || distanceMm > 3000)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "distance 600..3000");
        if (nbSamples < 1 || nbSamples > 16)
            throw new Vl53l8Exception(Vl53l8Exception.StatusInvalidParam, "nb_samples 1..16");

        int savedResolution = GetResolution();
        int savedFrequency = GetRangingFrequencyHz();
        int savedIntegration = GetIntegrationTimeMs();
        int savedSharpener = GetSharpenerPercent();
        int savedTargetOrder = GetTargetOrder();
        int savedMargin = GetXtalkMargin();
        int savedMode = GetRangingMode();

        SetResolution(Resolution8x8);
        byte[] table = (Firmware switch
        {
            Vl53l8Firmware.L7cx => Vl53l8Tables.CalibrateXtalkL7cx,
            Vl53l8Firmware.Cx => Vl53l8Tables.CalibrateXtalkCx,
            _ => Vl53l8Tables.CalibrateXtalkLmz,
        }).ToArray();
        _p.WriteMulti(0x2C28, table);
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "calib cmd");
        var dist = new byte[2];
        BinaryPrimitives.WriteUInt16LittleEndian(dist, (ushort)(distanceMm * 4));
        var refl = new byte[2];
        BinaryPrimitives.WriteUInt16LittleEndian(refl, (ushort)(reflectancePercent * 16));
        DciReplaceData(DciCalCfg, 8, dist, 0x00);
        DciReplaceData(DciCalCfg, 8, refl, 0x02);
        DciReplaceData(DciCalCfg, 8, new[] { (byte)nbSamples }, 0x04);
        ProgramXtalkOutputConfig(Resolution8x8);
        _p.WriteMulti(UiCmdEnd - 3, new byte[] { 0x00, 0x03, 0x00, 0x00 });
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "calib start");

        XtalkCalibrationFailed = false;
        for (int timeout = 0; ; timeout++)
        {
            byte[] buf = _p.ReadMulti(0x0, 4);
            if (buf[0] != Vl53l8Exception.StatusError)
            {
                if (buf[2] >= 0x7F && ((buf[3] & 0x80) >> 7) == 1)
                {
                    // ST: "Coverglass too good for Xtalk calibration" (STATUS_XTALK_FAILED).
                    _xtalkData = _defaultXtalk;
                    XtalkCalibrationFailed = true;
                }
                break;
            }
            if (timeout >= 400)
                throw new Vl53l8Exception(Vl53l8Exception.StatusError, "xtalk calibration");
            _p.SleepMs(50);
        }
        _xtalkData = ReadXtalkBuffer();
        _p.WriteMulti(0x2C34, _defaultCfg);
        PollForAnswer(4, 1, UiCmdStatus, 0xFF, 0x03, "restore cfg");

        SetResolution(savedResolution);
        SetRangingFrequencyHz(savedFrequency);
        SetIntegrationTimeMs(savedIntegration);
        SetSharpenerPercent(savedSharpener);
        SetTargetOrder(savedTargetOrder);
        SetXtalkMargin(savedMargin);
        SetRangingMode(savedMode);
    }
}
