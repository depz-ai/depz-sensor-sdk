using System.Buffers.Binary;

namespace Depz.Sensor.Bno086;

/// <summary>Bridge commands of the BNO086 board (contract 05 §2): the MCU only shuttles raw SHTP frames.</summary>
public enum Bno086Cmd
{
    /// <summary>Hardware reset through nRST; RPT_STATUS OK.</summary>
    SensorReset = 0x32,

    /// <summary>1 ms WAKE (PS0) pulse; RPT_STATUS OK.</summary>
    SensorWakeUp = 0x33,

    /// <summary>Payload = one raw SHTP frame; RPT_STATUS OK or ERR_BUSY (both TX slots full).</summary>
    SendShtpPacket = 0x34,
}

public enum Bno086Rpt
{
    /// <summary>rpt_data_t: cmd u8 (always 0x00, ERRATA E2), timestamp_us u64, raw SHTP frame.</summary>
    Data = 0x91,
}

/// <summary>One RPT_DATA report: an SHTP frame captured from the sensor bus, with the MCU uptime.</summary>
public sealed record Bno086Data(int Cmd, ulong TimestampUs, byte[] Shtp)
{
    /// <summary>Two 64-byte SHTP transmit slots on the MCU (ERRATA E2); a third send gets ERR_BUSY,
    /// to back off at least this long.</summary>
    public static readonly TimeSpan BusyBackoff = TimeSpan.FromMilliseconds(200);

    public static Bno086Data Unpack(ReadOnlySpan<byte> payload)
    {
        Protocol.Wire.NeedBytes("RPT_DATA", payload, 9);
        return new(payload[0], BinaryPrimitives.ReadUInt64LittleEndian(payload[1..]), payload[9..].ToArray());
    }
}

/// <summary>A SH-2 level failure: a non-zero command status, an FRS error.</summary>
public sealed class Sh2Exception : Exception
{
    public Sh2Exception(string message) : base(message) { }
}

/// <summary>Report ids on SHTP channel 2 (control).</summary>
public enum ControlReport
{
    CommandResponse = 0xF1,
    CommandRequest = 0xF2,
    FrsReadResponse = 0xF3,
    FrsReadRequest = 0xF4,
    FrsWriteResponse = 0xF5,
    FrsWriteData = 0xF6,
    FrsWriteRequest = 0xF7,
    ProductIdResponse = 0xF8,
    ProductIdRequest = 0xF9,
    GetFeatureResponse = 0xFC,
    SetFeatureCommand = 0xFD,
    GetFeatureRequest = 0xFE,

    /// <summary>Host → hub; answered by Flush Completed (0xEF) on the input channel.</summary>
    ForceSensorFlush = 0xF0,
}

/// <summary>The <c>command</c> of a Command Request / Response (0xF2 / 0xF1).</summary>
public enum Sh2Command
{
    Errors = 0x01,
    Counter = 0x02,
    Tare = 0x03,
    Initialize = 0x04,
    SaveDcd = 0x06,
    MeCalibrate = 0x07,
    PeriodicDcdConfig = 0x09,
    GetOscillatorType = 0x0A,
    ClearDcdAndReset = 0x0B,

    /// <summary>BNO086 only; no response.</summary>
    InteractiveCalibration = 0x0E,
}

public enum OscillatorType
{
    Internal = 0,
    ExtCrystal = 1,
    ExtClock = 2,
}

/// <summary>The <c>source</c> of an error record (SH-2 §6.4.1).</summary>
public enum ErrorSource
{
    MotionEngine = 1,
    MotionHub = 2,
    SensorHub = 3,
    Chip = 4,

    /// <summary>The end of the error queue.</summary>
    NoMoreErrors = 255,
}

/// <summary>The rotation vector a tare refers to (Tare Now P2).</summary>
public enum TareBasis
{
    RotationVector = 0,
    GameRotationVector = 1,
    GeomagneticRotationVector = 2,
    GyroIntegratedRv = 3,
    ArvrStabilizedRv = 4,
    ArvrStabilizedGameRv = 5,
}

/// <summary>Tare axes, a bitmap.</summary>
[Flags]
public enum TareAxis
{
    X = 1,
    Y = 2,
    Z = 4,
    All = 7,
}

/// <summary>P0 of Interactive Calibration (0x0E, SH-2 §6.4.12.1): what the host knows about the
/// device's motion. Unknown is the hub's initial state.</summary>
public enum MotionIntent
{
    Unknown = 0,
    StationaryNoVibration = 1,
    StationaryWithVibration = 2,
    InMotion = 3,
    InMotionAccelerating = 4,
}

/// <summary>FRS record ids (SH-2 figure 28).</summary>
public static class FrsRecordId
{
    public const int StaticCalibrationAgm = 0x7979;
    public const int NominalCalibration = 0x4D4D;
    public const int DynamicCalibration = 0x1F1F;
    public const int MePowerMgmt = 0xD3E2;
    /// <summary>Mounting quaternion, 4 × Q30 words.</summary>
    public const int SystemOrientation = 0x2D3E;
    public const int AccelOrientation = 0x2D41;
    public const int GyroscopeOrientation = 0x2D46;
    public const int MagnetometerOrientation = 0x2D4C;
    public const int ArvrStabilizationRv = 0x3E2D;
    public const int ArvrStabilizationGrv = 0x3E2E;
    /// <summary>One 32-bit word; empty unless programmed.</summary>
    public const int SerialNumber = 0x4B4B;
    /// <summary>Free for the user, up to 64 words.</summary>
    public const int UserRecord = 0x74B4;
    public const int SigMotionDetectConfig = 0xC274;
    public const int ShakeDetectConfig = 0x7D7D;
    public const int StabilityDetectorConfig = 0xED85;
    public const int ActivityTrackerConfig = 0xED88;
    /// <summary>Reference, sync interval, max error, prediction (SH-2 §4.3.24).</summary>
    public const int GyroIntegratedRvConfig = 0xA1A2;

    /// <summary>Sensor id → its metadata FRS record.</summary>
    public static readonly IReadOnlyDictionary<int, int> MetadataRecords = new Dictionary<int, int>
    {
        [0x14] = 0xE301, [0x01] = 0xE302, [0x04] = 0xE303, [0x06] = 0xE304, [0x15] = 0xE305,
        [0x02] = 0xE306, [0x07] = 0xE307, [0x16] = 0xE308, [0x03] = 0xE309, [0x0F] = 0xE30A,
        [0x05] = 0xE30B, [0x08] = 0xE30C, [0x09] = 0xE30D, [0x10] = 0xE313, [0x18] = 0xE314,
        [0x11] = 0xE315, [0x12] = 0xE316, [0x13] = 0xE317, [0x19] = 0xE318, [0x1E] = 0xE31C,
        [0x28] = 0xE322, [0x29] = 0xE323, [0x2A] = 0xE324, [0x2B] = 0xE325,
    };
}

public enum FrsStatus
{
    NoError = 0,
    UnrecognizedFrsType = 1,
    Busy = 2,
    ReadCompleted = 3,
    OffsetOutOfRange = 4,
    RecordEmpty = 5,
    BlockCompleted = 6,
    BlockAndReadCompleted = 7,
    DeviceError = 8,
}

public enum FrsWriteStatus
{
    WordsReceived = 0,
    UnrecognizedFrsType = 1,
    Busy = 2,
    WriteCompleted = 3,
    WriteModeReady = 4,
    WriteFailed = 5,
    NotInWriteMode = 6,
    InvalidLength = 7,
    RecordValid = 8,
    RecordInvalid = 9,
}

/// <summary>Get Feature Response (0xFC), 17 bytes: the rates in effect.</summary>
public sealed record FeatureResponse(int SensorId, int Flags, int Sensitivity, uint IntervalUs, uint BatchUs, uint CfgWord)
{
    public static FeatureResponse Unpack(ReadOnlySpan<byte> p)
    {
        Protocol.Wire.NeedBytes("Get Feature Response", p, 17);
        if (p[0] != (int)ControlReport.GetFeatureResponse)
            throw new Sh2Exception($"not a get-feature response: 0x{p[0]:X2}");
        return new(p[1], p[2], BinaryPrimitives.ReadUInt16LittleEndian(p[3..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[5..]), BinaryPrimitives.ReadUInt32LittleEndian(p[9..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[13..]));
    }
}

/// <summary>Product ID Response (0xF8), 16 bytes, one per software component (the SH-2
/// application first); the reset cause (SH-2 §6.4.5.2) is set on the first only.</summary>
public sealed record ProductId(int ResetCause, int SwVersionMajor, int SwVersionMinor, uint SwPartNumber,
    uint SwBuildNumber, int SwVersionPatch)
{
    public string Version => $"{SwVersionMajor}.{SwVersionMinor}.{SwVersionPatch}";

    public static ProductId Unpack(ReadOnlySpan<byte> p)
    {
        Protocol.Wire.NeedBytes("Product ID Response", p, 16);
        if (p[0] != (int)ControlReport.ProductIdResponse)
            throw new Sh2Exception($"not a product-id response: 0x{p[0]:X2}");
        return new(p[1], p[2], p[3], BinaryPrimitives.ReadUInt32LittleEndian(p[4..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[8..]), BinaryPrimitives.ReadUInt16LittleEndian(p[12..]));
    }
}

/// <summary>Command Response (0xF1), 16 bytes. <see cref="CommandSeq"/> echoes the request (correlate
/// on it plus <see cref="Command"/>); <see cref="ResponseSeq"/> counts responses to one request; R0 is
/// the status of most commands (0 = success).</summary>
public sealed record CommandResponse(int Seq, int Command, int CommandSeq, int ResponseSeq, byte[] R)
{
    public int Status => R[0];

    public static CommandResponse Unpack(ReadOnlySpan<byte> p)
    {
        Protocol.Wire.NeedBytes("Command Response", p, 16);
        if (p[0] != (int)ControlReport.CommandResponse)
            throw new Sh2Exception($"not a command response: 0x{p[0]:X2}");
        return new(p[1], p[2], p[3], p[4], p.Slice(5, 11).ToArray());
    }
}

/// <summary>FRS Read Response (0xF3), 16 bytes, up to two data words.</summary>
public sealed record FrsReadResponse(int Status, int DataLength, int OffsetWords, uint Data0, uint Data1, int FrsType)
{
    public static FrsReadResponse Unpack(ReadOnlySpan<byte> p)
    {
        Protocol.Wire.NeedBytes("FRS Read Response", p, 16);
        if (p[0] != (int)ControlReport.FrsReadResponse)
            throw new Sh2Exception($"not an FRS read response: 0x{p[0]:X2}");
        return new(p[1] & 0x0F, p[1] >> 4, BinaryPrimitives.ReadUInt16LittleEndian(p[2..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[4..]), BinaryPrimitives.ReadUInt32LittleEndian(p[8..]),
            BinaryPrimitives.ReadUInt16LittleEndian(p[12..]));
    }
}

/// <summary>FRS Write Response (0xF5), 4 bytes.</summary>
public sealed record FrsWriteResponse(int Status, int OffsetWords)
{
    public static FrsWriteResponse Unpack(ReadOnlySpan<byte> p)
    {
        Protocol.Wire.NeedBytes("FRS Write Response", p, 4);
        if (p[0] != (int)ControlReport.FrsWriteResponse)
            throw new Sh2Exception($"not an FRS write response: 0x{p[0]:X2}");
        return new(p[1], BinaryPrimitives.ReadUInt16LittleEndian(p[2..]));
    }
}

/// <summary>One error-queue entry (command 0x01 response, R0..R5).</summary>
public sealed record ErrorRecord(int Severity, int Seq, int Source, int Error, int Module, int Code)
{
    public static ErrorRecord From(CommandResponse r) => new(r.R[0], r.R[1], r.R[2], r.R[3], r.R[4], r.R[5]);
}

/// <summary>Per-sensor event counts (command 0x02, two responses).</summary>
public sealed record Counts(int SensorId, uint Offered, uint Accepted, uint On, uint Attempted);

/// <summary>ME calibration enables as the sensor reports them.</summary>
public sealed record CalibrationConfig(bool Accel, bool Gyro, bool Mag, bool Planar);

/// <summary>A sensor's metadata FRS record (revision-gated fields are 0 on older records);
/// <see cref="RawWords"/> is authoritative.</summary>
public sealed record SensorMetadata(
    int MeVersion, int MhVersion, int ShVersion, uint RangeRaw, uint ResolutionRaw, int Revision, int PowerMaQ10,
    uint MinPeriodUs, uint MaxPeriodUs, int FifoMax, int FifoReserved, int BatchBufferBytes,
    int QPoint1, int QPoint2, int QPoint3, uint[] RawWords)
{
    public static SensorMetadata FromWords(IReadOnlyList<uint> words)
    {
        uint W(int i) => i < words.Count ? words[i] : 0;
        // Word 3 is power_mA u16 (Q10) | revision u16 << 16.
        int revision = (int)(W(3) >> 16);
        return new SensorMetadata(
            (int)(W(0) & 0xFF), (int)(W(0) >> 8 & 0xFF), (int)(W(0) >> 16 & 0xFF),
            W(1), W(2), revision, (int)(W(3) & 0xFFFF), W(4), revision >= 4 ? W(9) : 0,
            (int)(W(5) & 0xFFFF), (int)(W(5) >> 16), (int)(W(6) & 0xFFFF),
            (int)(W(7) & 0xFFFF), (int)(W(7) >> 16), revision >= 3 ? (int)(W(8) >> 16) : 0,
            words.ToArray());
    }
}

/// <summary>SH-2 hub protocol: the request parameters and the control-cargo walk the device layer
/// needs on top of <see cref="Sh2Control"/> (contract 05 §6). Ported from the reference sh2.py.</summary>
public static class Sh2Hub
{
    public const int CountsGet = 0;
    public const int CountsClear = 1;
    /// <summary>ME Calibration subcommand: report the current configuration.</summary>
    public const int MeCalGet = 0x01;

    // Fixed lengths of the sensor → host control reports; one cargo may hold several.
    private static readonly IReadOnlyDictionary<int, int> ControlResponseLengths = new Dictionary<int, int>
    {
        [(int)ControlReport.CommandResponse] = 16,
        [(int)ControlReport.FrsReadResponse] = 16,
        [(int)ControlReport.FrsWriteResponse] = 4,
        [(int)ControlReport.ProductIdResponse] = 16,
        [(int)ControlReport.GetFeatureResponse] = 17,
    };

    /// <summary>Split a control cargo into its reports; an unknown id or a short tail ends the walk
    /// (the rest is one piece: its length is unknowable).</summary>
    public static List<byte[]> SplitControlCargo(byte[] payload)
    {
        var output = new List<byte[]>();
        int pos = 0;
        while (pos < payload.Length)
        {
            if (!ControlResponseLengths.TryGetValue(payload[pos], out int n) || pos + n > payload.Length)
            {
                output.Add(payload.AsSpan(pos).ToArray());
                break;
            }
            output.Add(payload.AsSpan(pos, n).ToArray());
            pos += n;
        }
        return output;
    }

    public static byte[] CountsGetParams(int sensorId) => new[] { (byte)CountsGet, U8(sensorId, nameof(sensorId)) };

    public static byte[] CountsClearParams(int sensorId) => new[] { (byte)CountsClear, U8(sensorId, nameof(sensorId)) };

    /// <summary>Errors of <paramref name="severity"/> or greater (0 = all).</summary>
    public static byte[] ErrorsParams(int severity = 0) => new[] { U8(severity, nameof(severity)) };

    public static byte[] TareNowParams(TareAxis axes = TareAxis.All, TareBasis basis = TareBasis.RotationVector) =>
        new[] { (byte)0x00, U8((int)axes, nameof(axes)), U8((int)basis, nameof(basis)) };

    public static byte[] PersistTareParams() => new byte[] { 0x01 };

    /// <summary>Tare subcommand 2: the reorientation quaternion as four Q14 int16 (all zero clears).</summary>
    public static byte[] SetReorientationParams(double x, double y, double z, double w)
    {
        var p = new byte[9];
        p[0] = 0x02;
        var q = new[] { x, y, z, w };
        for (int i = 0; i < 4; i++)
        {
            double v = Math.Round(q[i] * (1 << 14), MidpointRounding.ToEven); // Python round()
            if (double.IsNaN(v) || v < short.MinValue || v > short.MaxValue)
                throw new ArgumentOutOfRangeException(nameof(x), q[i], "quaternion component out of Q14 int16 range");
            BinaryPrimitives.WriteInt16LittleEndian(p.AsSpan(1 + 2 * i), (short)v);
        }
        return p;
    }

    /// <summary>ME Calibration (0x07): subcommand 0 configures, 1 (<see cref="MeCalGet"/>) reads back.</summary>
    public static byte[] MeCalibrationParams(bool accel, bool gyro, bool mag, bool planar = false, int subcommand = 0) =>
        new[] { B(accel), B(gyro), B(mag), U8(subcommand, nameof(subcommand)), B(planar) };

    /// <summary>Interactive Calibration (0x0E, BNO086 only): P0 = motion intent; no response.</summary>
    public static byte[] InteractiveCalibrationParams(int intent) => new[] { U8(intent, nameof(intent)) };

    /// <summary>Force Sensor Flush (0xF0, SH-2 §6.5.6); the hub answers with Flush Completed (0xEF).</summary>
    public static byte[] ForceFlush(int sensorId) => new[] { (byte)ControlReport.ForceSensorFlush, U8(sensorId, nameof(sensorId)) };

    /// <summary>Periodic DCD save (0x09): P0 0 = enable, 1 = disable; no response.</summary>
    public static byte[] PeriodicDcdParams(bool enable) => new[] { (byte)(enable ? 0x00 : 0x01) };

    private static byte B(bool v) => (byte)(v ? 1 : 0);

    private static byte U8(int v, string name) => (byte)Protocol.Wire.CheckUint(name, v, 8);
}

/// <summary>A multi-packet FRS read: send <see cref="Request"/>, <see cref="Feed"/> every 0xF3 of the
/// record until it returns true; <see cref="Words"/> holds the record. Error statuses throw.</summary>
public sealed class FrsReadSession
{
    public FrsReadSession(int frsType) => FrsType = frsType;

    public int FrsType { get; }
    public List<uint> Words { get; } = new();
    public bool Done { get; private set; }

    public byte[] Request() => Sh2Control.FrsRead(FrsType);

    public bool Feed(FrsReadResponse r)
    {
        if (r.FrsType != FrsType)
            return Done; // another record's traffic
        if (r.Status is (int)FrsStatus.UnrecognizedFrsType or (int)FrsStatus.Busy or (int)FrsStatus.OffsetOutOfRange
            or (int)FrsStatus.RecordEmpty or (int)FrsStatus.DeviceError)
            throw new Sh2Exception($"FRS read 0x{FrsType:X4} failed: {(FrsStatus)r.Status}");
        if (r.DataLength > 0)
            Words.Add(r.Data0);
        if (r.DataLength > 1)
            Words.Add(r.Data1);
        if (r.Status is (int)FrsStatus.ReadCompleted or (int)FrsStatus.BlockAndReadCompleted)
            Done = true;
        return Done;
    }
}

/// <summary>A multi-packet FRS write: send <see cref="Request"/>; for every 0xF5 <see cref="Feed"/>
/// returns the next Write Data payload, or null; <see cref="Done"/> on WRITE_COMPLETED. Error statuses throw.</summary>
public sealed class FrsWriteSession
{
    private int _offset;

    public FrsWriteSession(int frsType, IReadOnlyList<uint> words)
    {
        FrsType = frsType;
        Words = words.ToArray();
    }

    public int FrsType { get; }
    public uint[] Words { get; }
    public bool Done { get; private set; }

    public byte[] Request() => Sh2Control.FrsWrite(FrsType, Words.Length);

    public byte[]? Feed(FrsWriteResponse r)
    {
        if (r.Status is (int)FrsWriteStatus.UnrecognizedFrsType or (int)FrsWriteStatus.Busy
            or (int)FrsWriteStatus.WriteFailed or (int)FrsWriteStatus.NotInWriteMode
            or (int)FrsWriteStatus.InvalidLength or (int)FrsWriteStatus.RecordInvalid)
            throw new Sh2Exception($"FRS write 0x{FrsType:X4} failed: {(FrsWriteStatus)r.Status}");
        if (r.Status == (int)FrsWriteStatus.WriteCompleted)
        {
            Done = true;
            return null;
        }
        if (r.Status is (int)FrsWriteStatus.WriteModeReady or (int)FrsWriteStatus.WordsReceived)
        {
            if (_offset >= Words.Length)
                return null;
            var chunk = Words.Skip(_offset).Take(2).ToList();
            byte[] data = Sh2Control.FrsWriteDataReport(_offset, chunk);
            _offset += chunk.Count;
            return data;
        }
        return null; // RECORD_VALID and friends: informational
    }
}
