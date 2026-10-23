using System.Buffers.Binary;
using Depz.Sensor.Vl53l8;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>
/// The register bridge and DCI transport of a VL53L8 / L5 / L7 sensor, emulated (the Python
/// tests' FakeUldPlatform + ScriptedPlatform): DCI reads and writes land in <c>Dci</c>, plain
/// writes in <c>Reg</c>; the command status answers both the NVM poll (byte 0 = 2) and the DCI
/// polls (byte 1 = 3); firmware banks are recorded, not stored; <c>Scripts</c> give successive
/// byte reads of a register; <c>XtalkReply</c> answers the GET_XTALK command.
/// </summary>
internal sealed class FakeVl53l8Platform : IVl53l8Platform
{
    private const int UiCmdStatus = 0x2C00, UiCmdStart = 0x2C04, UiCmdEnd = 0x2FFF;

    public readonly Dictionary<int, byte> Reg = new();
    public readonly Dictionary<int, byte[]> Dci = new();
    public readonly List<(int Addr, byte[] Data)> Writes = new();
    public readonly List<(int Addr, int Size)> Reads = new();
    public readonly List<(int Page, int Length)> Banks = new();
    public readonly Dictionary<int, Queue<byte>> Scripts = new();
    public byte[] Status = { 0x02, 0x03, 0x00, 0x00 };
    public byte[]? XtalkReply;
    private (int Index, int Size)? _pendingRead;
    private bool _xtalkPending;
    private int _page = -1;

    public void SleepMs(int ms) { }

    public void WriteMulti(ushort addr, byte[] data)
    {
        if (addr == 0x7FFF && data.Length == 1)
            _page = data[0];
        if (data.Length >= 0x5000)
        {
            Writes.Add((addr, data));
            Banks.Add((_page, data.Length));
            return;
        }
        Writes.Add((addr, data));
        if (addr == 0x2FB8)
        {
            _xtalkPending = true;
            _pendingRead = null;
            return;
        }
        if (addr == UiCmdEnd - 11 && data.Length == 12)
        {
            _xtalkPending = false;
            _pendingRead = (data[0] << 8 | data[1], (data[2] << 4) | (data[3] >> 4));
            return;
        }
        if (data.Length >= 12 && data[^5] == 0x0F && data[^4] == 0x05 && data[^3] == 0x01)
        {
            int index = data[0] << 8 | data[1];
            int size = (data[2] << 4) | (data[3] >> 4);
            Dci[index] = Vl53l8FrameDecoder.SwapBuffer(data.AsSpan(4, size));
            return;
        }
        for (int i = 0; i < data.Length; i++)
            Reg[addr + i] = data[i];
    }

    public byte[] ReadMulti(ushort addr, int size)
    {
        Reads.Add((addr, size));
        if (addr == UiCmdStatus)
            return Status.Concat(new byte[Math.Max(0, size - 4)]).Take(size).ToArray();
        if (addr == UiCmdStart && _xtalkPending && XtalkReply != null)
            return XtalkReply.Take(size).ToArray();
        if (addr == UiCmdStart && _pendingRead is (int index, int dsize))
        {
            var stored = Dci.TryGetValue(index, out var d) ? d : Array.Empty<byte>();
            var raw = new byte[dsize + 12];
            Array.Copy(stored, 0, raw, 4, Math.Min(dsize, stored.Length));
            return Vl53l8FrameDecoder.SwapBuffer(raw);
        }
        if (size == 1 && Scripts.TryGetValue(addr, out var seq) && seq.Count > 0)
            return new[] { seq.Count > 1 ? seq.Dequeue() : seq.Peek() };
        return Enumerable.Range(0, size).Select(i => Reg.TryGetValue(addr + i, out byte b) ? b : (byte)0).ToArray();
    }

    public void SetU32(int addr, uint value, bool swapped = true)
    {
        var b = new byte[4];
        BinaryPrimitives.WriteUInt32LittleEndian(b, value);
        if (swapped)
            b = Vl53l8FrameDecoder.SwapBuffer(b);
        for (int i = 0; i < 4; i++)
            Reg[addr + i] = b[i];
    }

    public void FwReports(int frameSize)
    {
        var b = new byte[12];
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(8), (ushort)frameSize);
        Dci[0x5440] = b;
    }

    public void Zones(int resolution)
    {
        byte side = (byte)(resolution == 16 ? 4 : 8);
        Dci[0x5450] = new byte[] { side, side, 0, 0, 0, 0, 0, 0 };
    }

    public int Count(int addr, int size) => Reads.Count(r => r.Addr == addr && r.Size == size);

    public static uint[] U32s(byte[] b) =>
        Enumerable.Range(0, b.Length / 4).Select(i => BinaryPrimitives.ReadUInt32LittleEndian(b.AsSpan(4 * i))).ToArray();
}

public class Vl53l8DriverTests
{
    private const int DciZoneConfig = 0x5450, DciFreqHz = 0x5458, DciIntTime = 0x545C, DciRangingMode = 0xAD30;
    private const int DciTargetOrder = 0xAE64, DciOutputConfig = 0xD968, DciOutputEnables = 0xD970;
    private const int DciOutputList = 0xD980, DciPipeControl = 0xDB80, DciSingleRange = 0xD964, DciFwFlags = 0xE0C4;
    private const int DciCalCfg = 0x5470, DciMotionCfg = 0xBFAC, DciThreshStart = 0xB6E8, DciThreshValid = 0xB9F0;
    private const int UiCmdEnd = 0x2FFF;

    private static (Vl53l8Driver, FakeVl53l8Platform) Booting(Vl53l8Firmware fw)
    {
        var p = new FakeVl53l8Platform();
        bool i2c = fw == Vl53l8Firmware.L7cx || fw == Vl53l8Firmware.L7ch;
        p.Reg[0x06] = 0x01;                  // sensor boot / MCU boot
        p.Reg[0x21] = (byte)(i2c ? 0x10 : 0x04);
        if (fw == Vl53l8Firmware.Cx)
            p.SetU32(0x2FFC, 0xCADF7CAF);
        else if (fw != Vl53l8Firmware.L7cx)
            p.SetU32(0x2FFC, 0x0C0B6C9E);
        if (i2c)
            (p.Reg[0x00], p.Reg[0x01]) = (0xF0, 0x02);
        return (new Vl53l8Driver(p, fw), p);
    }

    private static (Vl53l8Driver, FakeVl53l8Platform) Ranging(Vl53l8Firmware fw = Vl53l8Firmware.Cx, int resolution = 64)
    {
        var p = new FakeVl53l8Platform();
        p.Zones(resolution);
        return (new Vl53l8Driver(p, fw), p);
    }

    private static readonly (int Addr, byte[] Data)[] Undo =
    {
        (0x7FFF, new byte[] { 0x00 }), (0x14, new byte[] { 0x00 }), (0x15, new byte[] { 0x00 }),
        (0x09, new byte[] { 0x04 }), (0x7FFF, new byte[] { 0x02 }),
    };

    private static void SameWrites(IEnumerable<(int Addr, byte[] Data)> want, IEnumerable<(int Addr, byte[] Data)> got) =>
        Assert.Equal(want.Select(w => (w.Addr, Convert.ToHexString(w.Data))), got.Select(g => (g.Addr, Convert.ToHexString(g.Data))));

    // ── init ─────────────────────────────────────────────────────────────────

    [Fact]
    public void InitDownloadsThreeBanksAndUploadsTheConfiguration()
    {
        var (drv, p) = Booting(Vl53l8Firmware.Cx);
        var notes = new List<string>();
        drv.Init(notes.Add);
        Assert.Equal(new[] { (0x09, 0x8000), (0x0A, 0x8000), (0x0B, 0x5000) }, p.Banks);
        Assert.All(p.Writes.Where(w => w.Data.Length >= 0x5000), w => Assert.Equal(0, w.Addr));
        var addrs = p.Writes.Select(w => w.Addr).ToList();
        Assert.True(addrs.IndexOf(0x2FD8) < addrs.IndexOf(0x2E18));
        Assert.True(addrs.IndexOf(0x2E18) < addrs.IndexOf(0x2CF8));
        Assert.True(addrs.IndexOf(0x2CF8) < addrs.IndexOf(0x2C34));
        var offset = p.Writes.First(w => w.Addr == 0x2E18).Data;
        Assert.Equal(Vl53l8Driver.OffsetBufferSize, offset.Length);
        Assert.Equal(new byte[] { 0, 0, 0, 0x0F, 0x03, 0x01, 0x01, 0xE4 }, offset[^8..]);
        Assert.Equal(Vl53l8Driver.XtalkBufferSize, p.Writes.First(w => w.Addr == 0x2CF8).Data.Length);
        Assert.Equal(new byte[] { 1, 0, 1, 0 }, p.Dci[DciPipeControl]);
        Assert.Equal(new byte[] { 1, 0, 0, 0 }, p.Dci[DciSingleRange]);
        Assert.Contains("Sensor FW checksum OK", notes);
        Assert.Equal("Sensor init done", notes[^1]);
        Assert.Null(drv.ModuleType);
    }

    [Fact]
    public void InitRefusesAWrongFirmwareChecksum()
    {
        var (drv, p) = Booting(Vl53l8Firmware.Cx);
        p.SetU32(0x2FFC, 0xDEADBEEF);
        Assert.Equal(Vl53l8Exception.StatusFwChecksumFail, Assert.Throws<Vl53l8Exception>(() => drv.Init()).Code);
    }

    [Fact]
    public void InitTimesOutWhenTheSensorNeverBoots()
    {
        var (drv, p) = Booting(Vl53l8Firmware.Cx);
        p.Reg[0x06] = 0x00;
        Assert.Equal(Vl53l8Exception.StatusTimeoutError, Assert.Throws<Vl53l8Exception>(() => drv.Init()).Code);
        Assert.Equal(201, p.Count(0x06, 1)); // 2 s at 10 ms, then the timeout
        Assert.Empty(p.Banks);
    }

    [Fact]
    public void InitStopsOnAnMcuErrorInTheCommandStatus()
    {
        var (drv, p) = Booting(Vl53l8Firmware.Cx);
        p.Status = new byte[] { 0x02, 0x03, 0x7F, 0x00 };
        Assert.Equal(Vl53l8Exception.McuError, Assert.Throws<Vl53l8Exception>(() => drv.Init()).Code);
    }

    [Fact]
    public void L7cxInitChecksSiliconSkipsTheChecksumAndReadsTheModuleType()
    {
        var (drv, p) = Booting(Vl53l8Firmware.L7cx);
        p.Dci[DciFwFlags] = new byte[] { 0, Vl53l8Driver.ModuleTypeMzevo, 0, 0, 0, 0, 0, 0 };
        var notes = new List<string>();
        drv.Init(notes.Add);
        Assert.Equal(Vl53l8Driver.ModuleTypeMzevo, drv.ModuleType);
        Assert.Equal("Sensor init done (module type MZEVO)", notes[^1]);
        Assert.DoesNotContain("Sensor FW checksum OK", notes);
        Assert.Equal(0, p.Count(0x2FFC, 4));
        Assert.Contains(p.Writes, w => w.Addr == 0x20 && w.Data[0] == 0x07);
        Assert.Contains(p.Writes, w => w.Addr == 0x20 && w.Data[0] == 0x06);
    }

    [Fact]
    public void L7cxInitRefusesL8Silicon()
    {
        var (drv, p) = Booting(Vl53l8Firmware.L7cx);
        (p.Reg[0x00], p.Reg[0x01]) = (0xF0, 0x0C);
        var e = Assert.Throws<Vl53l8Exception>(() => drv.Init());
        Assert.Equal(Vl53l8Exception.StatusError, e.Code);
        Assert.Contains("not a VL53L5/L7 sensor", e.Message);
        Assert.Empty(p.Banks);
    }

    [Theory]
    [InlineData(Vl53l8Firmware.Cx)]
    [InlineData(Vl53l8Firmware.Ch)]
    [InlineData(Vl53l8Firmware.L7cx)]
    [InlineData(Vl53l8Firmware.L7ch)]
    public void EveryFirmwareSetLoadsAndBoots(Vl53l8Firmware fw)
    {
        var (drv, p) = Booting(fw);
        drv.Init();
        Assert.Equal(3, p.Banks.Count);
        Assert.Equal(fw == Vl53l8Firmware.L7cx || fw == Vl53l8Firmware.L7ch, drv.IsI2c);
    }

    // ── start / stop ranging ─────────────────────────────────────────────────

    [Theory]
    [InlineData(Vl53l8Firmware.Cx, 64, 1452)]
    [InlineData(Vl53l8Firmware.Cx, 16, 540)]
    [InlineData(Vl53l8Firmware.Ch, 64, 1444)]   // 24-byte tail on the 2.0.x firmware
    [InlineData(Vl53l8Firmware.L7cx, 16, 1060)] // per-target blocks keep 64 entries
    [InlineData(Vl53l8Firmware.L7cx, 64, 1444)]
    public void StartRangingSizesTheFrameAndProgramsTheOutputs(Vl53l8Firmware fw, int resolution, int size)
    {
        var (drv, p) = Ranging(fw, resolution);
        p.FwReports(size);
        drv.StartRanging();
        Assert.Equal(size, drv.DataReadSize);
        Assert.Null(drv.FrameSizeMismatch);
        Assert.Equal(resolution, drv.Resolution);
        Assert.Equal(new uint[] { (uint)size, 13 }, FakeVl53l8Platform.U32s(p.Dci[DciOutputConfig]));
        uint w3 = fw == Vl53l8Firmware.Ch || fw == Vl53l8Firmware.L7ch ? 0u : 0xC0000000u;
        Assert.Equal(new uint[] { 0xFFF, 0, 0, w3 }, FakeVl53l8Platform.U32s(p.Dci[DciOutputEnables]));
        var output = FakeVl53l8Platform.U32s(p.Dci[DciOutputList]);
        Assert.Equal(12, output.Length);
        var sizes = output.ToDictionary(bh => (int)(bh >> 16), bh => (int)((bh >> 4) & 0xFFF));
        Assert.Equal(resolution, sizes[0x54D0]);                          // ambient
        Assert.Equal(drv.IsI2c ? 64 : resolution, sizes[0xDF44]);         // distance
        Assert.Equal(0x8C, sizes[0xD858]);                                // motion: not zone-scaled
    }

    [Fact]
    public void StartRangingWithCnhAppendsBlock13()
    {
        var (drv, p) = Ranging(Vl53l8Firmware.Ch);
        int cnh = 1708, size = 1444 + cnh + 4;
        p.FwReports(size);
        drv.StartRanging(cnh);
        var output = FakeVl53l8Platform.U32s(p.Dci[DciOutputList]);
        Assert.Equal(13, output.Length);
        Assert.Equal((0xC048u << 16) | ((uint)(cnh / 4) << 4) | 4, output[12]);
        Assert.Equal(0x1FFFu, FakeVl53l8Platform.U32s(p.Dci[DciOutputEnables])[0]);
        Assert.Equal(new uint[] { (uint)size, 14 }, FakeVl53l8Platform.U32s(p.Dci[DciOutputConfig]));
    }

    [Fact]
    public void StartRangingTrustsTheFirmwareFrameSize()
    {
        var (drv, p) = Ranging();
        p.FwReports(1448);
        drv.StartRanging();
        Assert.Equal((1448, 1452), drv.FrameSizeMismatch);
        Assert.Equal(1448, drv.DataReadSize);
    }

    [Fact]
    public void StartRangingWithoutAResolutionIsRefusedBeforeAnyWrite()
    {
        var p = new FakeVl53l8Platform();
        p.Dci[DciZoneConfig] = new byte[8];
        var drv = new Vl53l8Driver(p);
        Assert.Equal(Vl53l8Exception.StatusInvalidParam, Assert.Throws<Vl53l8Exception>(() => drv.StartRanging()).Code);
        Assert.False(p.Dci.ContainsKey(DciOutputList));
    }

    [Fact]
    public void StartRangingReportsALaserSafetyFault()
    {
        var (drv, p) = Ranging();
        p.FwReports(1452);
        p.Dci[DciFwFlags] = new byte[] { 0, 0, 0, 0, 0, 0, 1, 0 };
        Assert.Equal(Vl53l8Exception.StatusLaserSafety, Assert.Throws<Vl53l8Exception>(() => drv.StartRanging()).Code);
    }

    [Fact]
    public void StopRangingProvokesAnMcuStopThenUndoesIt()
    {
        var p = new FakeVl53l8Platform();
        p.Reg[0x06] = 0x80;
        p.Reg[0x07] = 0x84;
        new Vl53l8Driver(p).StopRanging();
        SameWrites(new[] { (0x7FFF, new byte[] { 0 }), (0x15, new byte[] { 0x16 }), (0x14, new byte[] { 0x01 }) }.Concat(Undo), p.Writes);
        Assert.Equal((0x2FFC, 4), p.Reads[0]); // the auto-stop flag, not swapped
    }

    [Fact]
    public void StopRangingAfterAutoStopSkipsTheMcuStop()
    {
        var p = new FakeVl53l8Platform();
        p.SetU32(0x2FFC, 0x4FF, swapped: false);
        new Vl53l8Driver(p).StopRanging();
        SameWrites(Undo, p.Writes);
    }

    [Fact]
    public void StopRangingGivesUpOnAStuckMcuAndStillUndoes()
    {
        var p = new FakeVl53l8Platform();
        new Vl53l8Driver(p).StopRanging();
        Assert.Equal(502, p.Count(0x06, 1)); // 501 polls, then the GO2 status read
        SameWrites(Undo, p.Writes.Skip(p.Writes.Count - 5));
    }

    // ── poll mode and frame parsing ──────────────────────────────────────────

    private static void Go2(FakeVl53l8Platform p, byte b0, byte b1, byte b2, byte b3) =>
        (p.Reg[0], p.Reg[1], p.Reg[2], p.Reg[3]) = (b0, b1, b2, b3);

    [Fact]
    public void CheckDataReadyFiresOncePerStreamCount()
    {
        var p = new FakeVl53l8Platform();
        var drv = new Vl53l8Driver(p);
        Go2(p, 5, 0x05, 0x05, 0x10);
        Assert.True(drv.CheckDataReady());
        Assert.False(drv.CheckDataReady());
        Go2(p, 6, 0x05, 0x05, 0x10);
        Assert.True(drv.CheckDataReady());
        foreach (var (b0, b1, b2, b3) in new (byte, byte, byte, byte)[] { (255, 5, 5, 0x10), (7, 4, 5, 0x10), (7, 5, 4, 0x10), (7, 5, 5, 0) })
        {
            Go2(p, b0, b1, b2, b3);
            Assert.False(drv.CheckDataReady());
        }
        Go2(p, 8, 0x00, 0x42, 0x80);
        Assert.Equal(0x42, Assert.Throws<Vl53l8Exception>(() => drv.CheckDataReady()).Code);
    }

    // A raw frame (as read from register 0) for the programmed output list.
    private static byte[] Frame(Vl53l8Driver drv, uint[] output, Func<int, int, byte[]> fill, int footerOff)
    {
        var buf = new byte[drv.DataReadSize];
        buf[8] = 0x12;
        buf[9] = 0x34;
        int i = 16;
        foreach (uint bh in output)
        {
            int t = (int)(bh & 0xF), s = (int)((bh >> 4) & 0xFFF), idx = (int)(bh >> 16);
            int msize = t > 1 && t < 0xD ? t * s : s;
            BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(i), bh);
            fill(idx, msize).CopyTo(buf, i + 4);
            i += 4 + msize;
        }
        buf[buf.Length - footerOff] = 0x12;
        buf[buf.Length - footerOff + 1] = 0x34;
        return Vl53l8FrameDecoder.SwapBuffer(buf);
    }

    private static byte[] Fill4x4(int idx, int msize)
    {
        var b = new byte[msize];
        switch (idx)
        {
            case 0xDF44: // distance ×4: 403 → 400, -5 → 0 (negative clamped, as ST), 8 → 2
                BinaryPrimitives.WriteInt16LittleEndian(b, 400 * 4 + 3);
                BinaryPrimitives.WriteInt16LittleEndian(b.AsSpan(2), -5);
                for (int k = 2; k < 16; k++)
                    BinaryPrimitives.WriteInt16LittleEndian(b.AsSpan(2 * k), 8);
                break;
            case 0xDEC4: // sigma ×128
                for (int k = 0; k < 16; k++)
                    BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(2 * k), 256);
                break;
            case 0xDB84: // nb target detected: zone 1 has none
                for (int k = 0; k < 16; k++)
                    b[k] = (byte)(k == 1 ? 0 : 1);
                break;
            case 0xE084:
                for (int k = 0; k < 16; k++)
                    b[k] = 5;
                break;
            case 0xDBC4: // signal ×2048: zone k carries k
                for (int k = 0; k < 16; k++)
                    BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(4 * k), (uint)k);
                break;
            case 0x54D0: // ambient ×2048: 3 kcps/SPAD in every zone
                for (int k = 0; 4 * k + 4 <= msize; k++)
                    BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(4 * k), 3 * 2048);
                break;
            case 0xE044: // reflectance ×2: 41 → 20.5 %
                for (int k = 0; k < 16; k++)
                    b[k] = 41;
                break;
            case 0x54B4: // metadata: −10 °C
                b[8] = 0xF6;
                break;
        }
        return b;
    }

    [Fact]
    public void GetRangingDataReadsTheFrameAndScales()
    {
        var (drv, p) = Ranging(Vl53l8Firmware.Cx, 16);
        p.FwReports(540);
        drv.StartRanging();
        var raw = Frame(drv, FakeVl53l8Platform.U32s(p.Dci[DciOutputList]), Fill4x4, 12);
        for (int i = 0; i < raw.Length; i++)
            p.Reg[i] = raw[i];
        var f = drv.GetRangingData();
        Assert.Equal((0, 540), p.Reads[^1]);
        Assert.Equal(16, f.DistanceMm.Length);
        Assert.Equal(new[] { 400, 0, 2 }, f.DistanceMm.Take(3));   // negative → 0, as ST (contract 04 §5)
        Assert.Equal(2.0, f.RangeSigmaMm[0]);                       // raw ÷128, mm
        Assert.Equal(Enumerable.Range(0, 16).Select(k => k / 2048.0), f.SignalPerSpad);   // kcps/SPAD
        Assert.Equal(3.0, f.AmbientPerSpad[0]);                     // raw ÷2048, kcps/SPAD
        Assert.Equal(20.5, f.Reflectance[0]);                       // raw ÷2, %
        Assert.Equal(new byte[] { 5, 255 }, f.TargetStatus.Take(2));
        Assert.Equal(-10, f.SiliconTempDegc);
        Assert.Null(f.CnhRaw);
        Assert.Null(f.Motion);
    }

    [Fact]
    public void AFrameWhoseFooterIdDiffersIsCorrupted()
    {
        var (drv, p) = Ranging(Vl53l8Firmware.Cx, 16);
        p.FwReports(540);
        drv.StartRanging();
        var buf = Vl53l8FrameDecoder.SwapBuffer(Frame(drv, FakeVl53l8Platform.U32s(p.Dci[DciOutputList]), Fill4x4, 12));
        buf[buf.Length - 12] ^= 0xFF;
        Assert.Throws<Vl53l8FrameDecoder.CorruptedFrameException>(() => drv.ParseFrame(0, Vl53l8FrameDecoder.SwapBuffer(buf)));
    }

    [Fact]
    public void TheMotionBlockIsParsedOnceTheIndicatorIsConfigured()
    {
        var (drv, p) = Ranging(Vl53l8Firmware.Cx, 16);
        p.FwReports(540);
        drv.MotionIndicatorInit(16);
        drv.StartRanging();
        byte[] Fill(int idx, int msize)
        {
            if (idx != 0xD858)
                return Fill4x4(idx, msize);
            var b = new byte[msize];
            BinaryPrimitives.WriteUInt32LittleEndian(b, 7);
            BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(4), 9);
            (b[8], b[9], b[10]) = (1, 2, 16);
            for (int k = 0; k < 32; k++)
                BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(12 + 4 * k), (uint)(100 + k));
            return b;
        }
        var m = drv.ParseFrame(0, Frame(drv, FakeVl53l8Platform.U32s(p.Dci[DciOutputList]), Fill, 12)).Motion;
        Assert.NotNull(m);
        Assert.Equal((7u, 9u, 1, 2, 16), (m!.GlobalIndicator1, m.GlobalIndicator2, m.Status, m.NbOfDetectedAggregates, m.NbOfAggregates));
        Assert.Equal(Enumerable.Range(100, 32).Select(x => (uint)x), m.Motion);
    }

    [Fact]
    public void L7FramesAreTrimmedToTheZonesInUse()
    {
        var (drv, p) = Ranging(Vl53l8Firmware.L7cx, 16);
        p.FwReports(1060);
        drv.StartRanging();
        byte[] Fill(int idx, int msize)
        {
            var b = new byte[msize];
            if (idx == 0xDF44)
                for (int k = 0; k < 64; k++)
                    BinaryPrimitives.WriteInt16LittleEndian(b.AsSpan(2 * k), (short)(k < 16 ? 40 : 999));
            if (idx == 0xDB84)
                for (int k = 0; k < 64; k++)
                    b[k] = 1;
            return b;
        }
        var f = drv.ParseFrame(0, Frame(drv, FakeVl53l8Platform.U32s(p.Dci[DciOutputList]), Fill, 4));
        Assert.Equal(Enumerable.Repeat(10, 16), f.DistanceMm);
        Assert.Equal(16, f.TargetStatus.Length);
        Assert.Equal(16, f.NbSpadsEnabled.Length);
    }

    // ── power modes ──────────────────────────────────────────────────────────

    [Fact]
    public void PowerModes()
    {
        var p = new FakeVl53l8Platform();
        var drv = new Vl53l8Driver(p);
        p.Reg[0x09] = 0x07;
        Assert.Equal(Vl53l8Exception.StatusError, Assert.Throws<Vl53l8Exception>(() => drv.GetPowerMode()).Code);
        Assert.Equal((0x7FFF, (byte)0x02), (p.Writes[^1].Addr, p.Writes[^1].Data[0])); // page restored

        p.Writes.Clear();
        p.Reg[0x09] = 0x04;
        drv.SetPowerMode(Vl53l8Driver.PowerModeWakeup); // already awake: only the page dance
        SameWrites(new[] { (0x7FFF, new byte[] { 0 }), (0x7FFF, new byte[] { 2 }) }, p.Writes);

        p.Reg[0x06] = 0x00;
        drv.SetPowerMode(Vl53l8Driver.PowerModeDeepSleep);
        SameWrites(new[] { (0x7FFF, new byte[] { 0 }), (0x09, new byte[] { 2 }), (0x0F, new byte[] { 0x43 }), (0x7FFF, new byte[] { 2 }) },
            p.Writes.Skip(p.Writes.Count - 4));

        p.Reg[0x09] = 0x04;
        p.Reg[0x06] = 0x01; // never clears bit 0
        Assert.Equal(Vl53l8Exception.StatusTimeoutError,
            Assert.Throws<Vl53l8Exception>(() => drv.SetPowerMode(Vl53l8Driver.PowerModeSleep)).Code);
        Assert.Equal(Vl53l8Exception.StatusInvalidParam, Assert.Throws<Vl53l8Exception>(() => drv.SetPowerMode(7)).Code);
    }

    [Fact]
    public void L7cxHasNoDeepSleep()
    {
        var p = new FakeVl53l8Platform();
        var drv = new Vl53l8Driver(p, Vl53l8Firmware.L7cx);
        (p.Reg[0x09], p.Reg[0x0F]) = (0x02, 0x43);
        Assert.Equal(Vl53l8Driver.PowerModeSleep, drv.GetPowerMode());
        Assert.Equal(0, p.Count(0x0F, 1));
        Assert.Equal(Vl53l8Exception.StatusInvalidParam,
            Assert.Throws<Vl53l8Exception>(() => drv.SetPowerMode(Vl53l8Driver.PowerModeDeepSleep)).Code);
        Assert.Equal(Vl53l8Exception.StatusInvalidParam,
            Assert.Throws<Vl53l8Exception>(() => drv.SetDetectionThresholdsAutoStop(true)).Code);
    }

    // ── xtalk, thresholds, motion ────────────────────────────────────────────

    [Theory]
    [InlineData(-0.5)]
    [InlineData(10000.5)]
    [InlineData(double.NaN)]
    public void XtalkMarginOutOfRangeIsRefused(double bad)
    {
        var p = new FakeVl53l8Platform();
        Assert.Throws<Vl53l8Exception>(() => new Vl53l8Driver(p).SetXtalkMargin(bad));
        Assert.False(p.Dci.ContainsKey(0xAD94));
    }

    private static byte[] XtalkReply() => Enumerable.Range(0, 1024).Select(i => (byte)i).ToArray();

    [Fact]
    public void CaldataXtalkReadsTheBufferAndRestoresTheResolution()
    {
        var (drv, p) = Ranging(Vl53l8Firmware.Cx, 16);
        p.XtalkReply = XtalkReply();
        byte[] blob = drv.GetCaldataXtalk();
        Assert.Contains(p.Writes, w => w.Addr == 0x2FB8 && w.Data.SequenceEqual(Vl53l8Tables.GetXtalkCmd.ToArray()));
        Assert.Equal(Vl53l8Driver.XtalkBufferSize, blob.Length);
        Assert.Equal(p.XtalkReply[8..Vl53l8Driver.XtalkBufferSize], blob[..^8]);
        Assert.Equal(new byte[] { 0, 0, 0, 0x0F, 0, 0x01, 0x03, 0x04 }, blob[^8..]);
        Assert.Equal(blob, drv.XtalkData);
        Assert.Equal(16, drv.GetResolution());
        Assert.Throws<Vl53l8Exception>(() => drv.SetCaldataXtalk(new byte[10]));
    }

    [Fact]
    public void DetectionThresholds()
    {
        var p = new FakeVl53l8Platform();
        var drv = new Vl53l8Driver(p);
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            drv.SetDetectionThresholds(Enumerable.Repeat(new Vl53l8Advanced.DetectionThreshold(0, 0, 1, 0, 0, 0), 65).ToList()));
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            drv.SetDetectionThresholds(new[] { new Vl53l8Advanced.DetectionThreshold(0, int.MaxValue / 4 + 1, Vl53l8Advanced.DistMm, 0, 0, 0) }));
        Assert.False(p.Dci.ContainsKey(DciThreshStart));

        drv.SetDetectionThresholds(Array.Empty<Vl53l8Advanced.DetectionThreshold>());
        Assert.Equal(Enumerable.Repeat((byte)5, 8), p.Dci[DciThreshValid]);
        Assert.Equal(new byte[64 * 12], p.Dci[DciThreshStart]);

        drv.SetDetectionThresholds(new[] { new Vl53l8Advanced.DetectionThreshold(100, 300, Vl53l8Advanced.DistMm, 1, 7, 0) });
        Assert.Equal(new Vl53l8Advanced.DetectionThreshold(100, 300, Vl53l8Advanced.DistMm, 1, 7, 0), drv.GetDetectionThresholds()[0]);
    }

    [Fact]
    public void MotionIndicatorWindow()
    {
        var p = new FakeVl53l8Platform();
        var drv = new Vl53l8Driver(p);
        var cfg = drv.MotionIndicatorInit(16);
        Assert.Equal(Enumerable.Range(0, 16).Select(i => (sbyte)i).Concat(Enumerable.Repeat((sbyte)-1, 48)), cfg.MapId);
        Assert.Throws<Vl53l8Exception>(() => drv.MotionIndicatorSetResolution(cfg, 32));
        foreach (var (lo, hi) in new[] { (400, 1901), (300, 1000), (3000, 4001) })
            Assert.Throws<Vl53l8Exception>(() => drv.MotionIndicatorSetDistanceMotion(cfg, lo, hi));
        drv.MotionIndicatorSetDistanceMotion(cfg, 1000, 2500);
        Assert.Equal((int)((1000 / 37.5348 - 4.0) * 2048.5), cfg.RefBinOffset);
        Assert.Equal((int)((150.0 + 30.02784) / 15.01392 + 0.5), cfg.FeatureLength);
        Assert.Equal(cfg.Pack(), p.Dci[DciMotionCfg]);
    }

    // ── xtalk calibration ────────────────────────────────────────────────────

    [Theory]
    [InlineData(0, 4, 600)]
    [InlineData(100, 4, 600)]
    [InlineData(16, 0, 600)]
    [InlineData(16, 17, 600)]
    [InlineData(16, 4, 599)]
    [InlineData(16, 4, 3001)]
    public void CalibrationArgumentsAreCheckedFirst(int reflectance, int samples, int distance)
    {
        var p = new FakeVl53l8Platform();
        var e = Assert.Throws<Vl53l8Exception>(() => new Vl53l8Driver(p).CalibrateXtalk(reflectance, samples, distance));
        Assert.Equal(Vl53l8Exception.StatusInvalidParam, e.Code);
        Assert.Empty(p.Writes);
    }

    private static (Vl53l8Driver, FakeVl53l8Platform) Calibration(Vl53l8Firmware fw = Vl53l8Firmware.Cx)
    {
        var (drv, p) = Ranging(fw, 16);
        p.Dci[DciFreqHz] = new byte[] { 0, 15, 0, 0 };
        p.Dci[DciIntTime] = new byte[] { 0x20, 0x4E, 0, 0 }.Concat(new byte[16]).ToArray(); // 20000 µs
        p.Dci[DciRangingMode] = new byte[] { 0, 3, 0, 2, 0, 0, 0, 0 };                       // autonomous
        p.Dci[DciTargetOrder] = new byte[] { Vl53l8Driver.TargetOrderClosest, 0, 0, 0 };
        p.XtalkReply = XtalkReply();
        return (drv, p);
    }

    [Theory]
    [InlineData(Vl53l8Firmware.Cx)]
    [InlineData(Vl53l8Firmware.Ch)]
    [InlineData(Vl53l8Firmware.L7cx)]
    public void CalibrationRunsTheFirmwareTableAndRestoresTheSettings(Vl53l8Firmware fw)
    {
        var (drv, p) = Calibration(fw);
        drv.CalibrateXtalk(16, 4, 600);
        byte[] table = (fw switch
        {
            Vl53l8Firmware.Cx => Vl53l8Tables.CalibrateXtalkCx,
            Vl53l8Firmware.L7cx => Vl53l8Tables.CalibrateXtalkL7cx,
            _ => Vl53l8Tables.CalibrateXtalkLmz,
        }).ToArray();
        var addrs = p.Writes.Select(w => w.Addr).ToList();
        Assert.Contains(p.Writes, w => w.Addr == 0x2C28 && w.Data.SequenceEqual(table));
        Assert.True(addrs.IndexOf(0x2C28) < addrs.IndexOf(UiCmdEnd - 3));
        Assert.True(addrs.IndexOf(UiCmdEnd - 3) < addrs.IndexOf(0x2FB8));
        var cal = p.Dci[DciCalCfg];
        Assert.Equal((2400, 256, 4), (BinaryPrimitives.ReadUInt16LittleEndian(cal), BinaryPrimitives.ReadUInt16LittleEndian(cal.AsSpan(2)), (int)cal[4]));
        Assert.Equal(Vl53l8Tables.XtalkCalOutput.ToArray().Select(bh => bh & ~0xFFF0u),
            FakeVl53l8Platform.U32s(p.Dci[DciOutputList]).Select(bh => bh & ~0xFFF0u));
        Assert.Equal(new uint[] { 1204, 18 }, FakeVl53l8Platform.U32s(p.Dci[DciOutputConfig]));
        Assert.False(drv.XtalkCalibrationFailed);
        Assert.Equal(p.XtalkReply![8..Vl53l8Driver.XtalkBufferSize], drv.XtalkData[..^8]);
        Assert.True(addrs.IndexOf(0x2FB8) < addrs.IndexOf(0x2C34));
        Assert.Equal(16, drv.GetResolution());
        Assert.Equal(15, drv.GetRangingFrequencyHz());
        Assert.Equal(20, drv.GetIntegrationTimeMs());
        Assert.Equal(Vl53l8Driver.RangingModeAutonomous, drv.GetRangingMode());
        Assert.Equal(Vl53l8Driver.TargetOrderClosest, drv.GetTargetOrder());
    }

    [Fact]
    public void CoverglassTooGoodIsFlaggedNotThrown()
    {
        var (drv, p) = Calibration();
        Go2(p, 0x00, 0x00, 0x7F, 0x80);
        drv.CalibrateXtalk(16, 4, 600);
        Assert.True(drv.XtalkCalibrationFailed);
    }

    [Fact]
    public void CalibrationWaitsThenTimesOut()
    {
        var (drv, p) = Calibration();
        p.Reg[0] = Vl53l8Exception.StatusError; // still calibrating
        var e = Assert.Throws<Vl53l8Exception>(() => drv.CalibrateXtalk(16, 4, 600));
        Assert.Equal(Vl53l8Exception.StatusError, e.Code);
        Assert.Contains("xtalk calibration", e.Message);
        Assert.Equal(401, p.Count(0x0, 4)); // 400 waits of 50 ms, 401 reads
    }

    [Fact]
    public void SharpenerReadsBackRoundedToNearest()
    {
        var p = new FakeVl53l8Platform();
        var drv = new Vl53l8Driver(p);
        p.Dci[0xAED8] = new byte[16];
        drv.SetSharpenerPercent(25);
        Assert.Equal(25, drv.GetSharpenerPercent());
        Assert.Throws<Vl53l8Exception>(() => drv.SetSharpenerPercent(100));
        Assert.Throws<ArgumentOutOfRangeException>(() => drv.SetSharpenerPercent(-1));
        Assert.Throws<ArgumentOutOfRangeException>(() => drv.SetRangingFrequencyHz(256));
    }
}
