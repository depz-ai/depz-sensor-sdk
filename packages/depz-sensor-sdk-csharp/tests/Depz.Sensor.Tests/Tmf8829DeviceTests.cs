using System.Buffers.Binary;
using System.Text.Json;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Tmf8829;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>
/// Fake TMF8829 SPI register bridge + sensor (the Python tests/fake_tmf8829.py): the bridge answers
/// READ_REG / WRITE_REG / START_STREAM / STOP_STREAM / GET_INFO; the sensor is a 256-byte register file with
/// the side effects the host sequence relies on — soft reset → bootloader, bootloader commands at CMD_STAT,
/// the firmware download through FIFO 0xFF, application commands (presets, LOAD_CONFIG_PAGE, WRITE_PAGE,
/// MEASURE, STOP) and the config page with the "preset does not reset everything" behaviour.
/// </summary>
internal sealed class FakeTmf8829
{
    private const int RegAppId = 0x00, RegCmdStat = 0x08, RegSerial = 0x1C, RegCidRid = 0x20, RegCfg = 0x22, CfgSize = 190;
    private const int RegIntStatus = 0xE1, RegId = 0xE3, RegReset = 0xF7, RegEnable = 0xF8, RegFifoStatus = 0xFA, RegFifo = 0xFF;

    // fp_mode each preset command loads
    private static readonly Dictionary<int, int> PresetFpMode = new()
    {
        [0x40] = 0, [0x41] = 0, [0x42] = 0, [0x43] = 2, [0x44] = 2, [0x45] = 3, [0x46] = 3, [0x47] = 5, [0x48] = 5, [0x49] = 0,
    };

    public readonly FakeBoardLink Link = new();
    public readonly byte[] Regs = new byte[256];
    public readonly byte[] Cfg = new byte[CfgSize];
    public readonly List<byte> Fifo = new();
    public byte[]? WFifoParams;
    public string Mode = "app-old";
    public bool I2cOff;

    // fault injection
    public int DeviceId = 0x9E;
    public readonly Dictionary<int, int> BlStatus = new();
    public readonly Dictionary<int, int> AppStatus = new();
    public int BusyPolls;
    public bool StuckBeforeBootloader;
    public int? CidOverride;
    public readonly Dictionary<int, int> StuckBytes = new();
    public readonly Dictionary<int, int> BridgeStatus = new();
    public readonly HashSet<int> Silent = new();
    public int ShortRead;
    /// <summary>Packets sent after a CMD_STAT write is applied, before its WRITE_REG reply.</summary>
    public Func<int, IEnumerable<byte[]>>? OnCommand;
    public bool Streaming;
    public int IntMask, Flags;

    private int _pendingCmd;
    private int _frameSeq;

    public FakeTmf8829()
    {
        Regs[RegAppId] = 0x01; // an application left running by an earlier session
        Cfg[0] = 33;
        BinaryPrimitives.WriteUInt16LittleEndian(Cfg.AsSpan(2), 586);
        Cfg[0x2A - RegCfg] = 1;
        Link.OnPacket = Handle;
    }

    public List<byte[]> Sent(int cmd)
    {
        lock (Link.Sent)
            return Link.Sent.Where(p => p.Cmd == cmd).Select(p => p.Payload.ToArray()).ToList();
    }

    public List<int[]> Writes() => Sent(0x33).Select(p => p.Select(b => (int)b).ToArray()).ToList();

    public int LogCount
    {
        get { lock (Link.Sent) return Link.Sent.Count; }
    }

    public List<(int Cmd, byte[] Payload)> LogFrom(int n)
    {
        lock (Link.Sent)
            return Link.Sent.Skip(n).Select(p => (p.Cmd, p.Payload.ToArray())).ToList();
    }

    public int EmitFrame(byte[] raw, long timestampUs, long edgeUs = 0, int? seq = null, int[]? chunks = null)
    {
        int s = seq ?? _frameSeq;
        _frameSeq = (s + 1) & 0xFF;
        var offsets = chunks ?? Enumerable.Range(0, (raw.Length + 1023) / 1024).Select(i => i * 1024).ToArray();
        for (int i = 0; i < offsets.Length; i++)
        {
            int off = offsets[i];
            int end = i + 1 < offsets.Length ? offsets[i + 1] : raw.Length;
            var data = raw.AsSpan(off, Math.Min(end, off + 1024) - off);
            var p = new byte[21 + data.Length];
            BinaryPrimitives.WriteUInt64LittleEndian(p, (ulong)timestampUs);
            BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(8), (ushort)raw.Length);
            BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(10), (ushort)off);
            p[12] = (byte)s;
            BinaryPrimitives.WriteUInt64LittleEndian(p.AsSpan(13), (ulong)edgeUs);
            data.CopyTo(p.AsSpan(21));
            Link.Inject(Link.Reply(0x93, p));
        }
        return s;
    }

    public byte[] Event(int code, int intStatus, int frameId = 0, int fifoStatus = 0, ulong ts = 987654321)
    {
        var p = new byte[12];
        BinaryPrimitives.WriteUInt64LittleEndian(p, ts);
        (p[8], p[9], p[10], p[11]) = ((byte)intStatus, (byte)code, (byte)frameId, (byte)fifoStatus);
        return Link.Reply(0x94, p);
    }

    public void EmitEvent(int code, int intStatus, int frameId = 0, int fifoStatus = 0) =>
        Link.Inject(Event(code, intStatus, frameId, fifoStatus));

    private IEnumerable<byte[]> Handle(Transport.Packet pkt)
    {
        int cmd = pkt.Cmd;
        byte[] payload = pkt.Payload;
        if (Silent.Contains(cmd))
            return Array.Empty<byte[]>();
        if (BridgeStatus.TryGetValue(cmd, out int bs))
            return new[] { FakeBoardLink.Status(cmd, (Status)bs) };
        switch (cmd)
        {
            case 0x32:
            {
                int addr = payload[0], n = BinaryPrimitives.ReadUInt16LittleEndian(payload.AsSpan(1));
                var data = Enumerable.Range(0, Math.Max(0, n - ShortRead)).Select(i => (byte)Read8(Math.Min(addr + i, RegFifo)));
                var head = new byte[9];
                head[0] = 0x32;
                BinaryPrimitives.WriteUInt64LittleEndian(head.AsSpan(1), 5_000_000);
                return new[] { Link.Reply(0x91, head.Concat(data).ToArray()) };
            }
            case 0x33:
            {
                WriteRegs(payload[0], payload.AsSpan(1).ToArray());
                var extra = payload[0] == RegCmdStat && OnCommand != null ? OnCommand(payload[1]) : Array.Empty<byte[]>();
                return extra.Concat(new[] { FakeBoardLink.Status(cmd, Status.Ok) }).ToList();
            }
            case 0x35:
                Streaming = true;
                (IntMask, Flags) = (payload[0], payload[1]);
                _frameSeq = 0;
                return new[] { FakeBoardLink.Status(cmd, Status.Ok) };
            case 0x36:
                Streaming = false;
                return new[] { FakeBoardLink.Status(cmd, Status.Ok) };
            case 0x37:
            {
                var p = new byte[25];
                BinaryPrimitives.WriteUInt32LittleEndian(p, 7);
                BinaryPrimitives.WriteUInt32LittleEndian(p.AsSpan(4), 6);
                BinaryPrimitives.WriteUInt32LittleEndian(p.AsSpan(8), 1);
                (p[20], p[21], p[22], p[23], p[24]) = (1, (byte)(Streaming ? 1 : 0), (byte)IntMask, 0x01, 0x00);
                return new[] { Link.Reply(0x92, p) };
            }
            default:
                return new[] { FakeBoardLink.Status(cmd, Status.ErrInvalidCmd) };
        }
    }

    private int Read8(int a)
    {
        if (a == RegId)
            return DeviceId;
        if (a == RegEnable)
            return Mode == "bootloader" && !I2cOff ? 0 : Regs[a] | 0x80; // 0 over SPI until the sensor's I2C is off
        if (a == RegCmdStat && BusyPolls > 0)
        {
            BusyPolls--;
            return _pendingCmd;
        }
        return Regs[a];
    }

    private void WriteRegs(int addr, byte[] data)
    {
        if (addr == RegFifo)
        {
            Fifo.AddRange(data);
            return;
        }
        if (addr == RegCmdStat)
        {
            Command(data);
            return;
        }
        for (int i = 0; i < data.Length; i++)
        {
            int a = addr + i;
            byte b = data[i];
            if (a == RegReset && (b & 0x40) != 0)
                SoftReset();
            else if (a == RegIntStatus)
                Regs[a] &= (byte)~b; // write-1-to-clear
            else if (a == RegFifoStatus)
                Regs[a] &= (byte)~(b & 0x30);
            else
                Regs[a] = b;
        }
    }

    private void SoftReset()
    {
        Mode = "bootloader";
        I2cOff = false;
        Regs[RegAppId] = (byte)(StuckBeforeBootloader ? 0x00 : 0x80);
        Regs[RegCmdStat] = 0;
        Fifo.Clear();
        (Cfg[0], Cfg[1]) = (33, 0);
        Cfg[0x2A - RegCfg] = 1;
    }

    private void Command(byte[] data)
    {
        int cmd = data[0];
        _pendingCmd = cmd;
        int st;
        if (Mode == "bootloader")
        {
            st = BlStatus.TryGetValue(cmd, out int s) ? s : 0;
            if (st == 0)
            {
                if (cmd == 0x22)
                {
                    I2cOff = true;
                }
                else if (cmd == 0x45)
                {
                    WFifoParams = data.AsSpan(2, data[1]).ToArray();
                }
                else if (cmd == 0x16)
                {
                    Mode = "app";
                    Regs[RegAppId] = 0x01;
                    (Regs[1], Regs[2], Regs[3]) = (2, 200, 0);
                    BinaryPrimitives.WriteUInt32LittleEndian(Regs.AsSpan(RegSerial), 0x12345678);
                }
            }
        }
        else
        {
            st = AppStatus.TryGetValue(cmd, out int s) ? s : cmd == 0x10 ? 0x01 : 0x00;
            if (st <= 0x01)
                AppCommand(cmd);
        }
        Regs[RegCmdStat] = (byte)st;
    }

    private void AppCommand(int cmd)
    {
        if (PresetFpMode.TryGetValue(cmd, out int fp))
        {
            Cfg[0x26 - RegCfg] = (byte)fp;
            Regs[RegCidRid] = (byte)cmd;
        }
        else if (cmd == 0x16)
        {
            Cfg.CopyTo(Regs, RegCfg);
            Regs[RegCidRid] = (byte)(CidOverride ?? 0x16);
        }
        else if (cmd == 0x15)
        {
            Array.Copy(Regs, RegCfg, Cfg, 0, CfgSize);
            foreach (var kv in StuckBytes)
                Cfg[kv.Key] = (byte)kv.Value;
        }
    }
}

public class Tmf8829DeviceTests
{
    private const int W = 0x33, R = 0x32;
    private static readonly JsonElement V = Vectors.Load("tmf8829.json");
    private static readonly Dictionary<string, byte[]> Raw = V.GetProperty("frames").EnumerateArray()
        .ToDictionary(c => c.GetProperty("name").GetString()!, c => Vectors.Hex(c.GetProperty("raw").GetString()!));
    private static readonly Dictionary<string, JsonElement> Meas = V.GetProperty("measurements").EnumerateArray()
        .ToDictionary(m => m.GetProperty("name").GetString()!, m => m);
    private static readonly HashSet<string> Corrupt = V.GetProperty("frames").EnumerateArray()  // results frames whose pixel decode fails
        .Where(c => c.TryGetProperty("error", out _)).Select(c => c.GetProperty("name").GetString()!).ToHashSet();

    private static (FakeTmf8829 Fake, Tmf8829Device Dev) Make()
    {
        var fake = new FakeTmf8829();
        return (fake, new Tmf8829Device(fake.Link, TimeSpan.FromMilliseconds(500)));
    }

    private static void WaitFor<T>(List<T> list, int n)
    {
        var until = DateTime.UtcNow + TimeSpan.FromSeconds(2);
        while (DateTime.UtcNow < until)
        {
            lock (list)
                if (list.Count >= n)
                    break;
            Thread.Sleep(5);
        }
        lock (list)
            Assert.Equal(n, list.Count);
    }

    private static void Play(FakeTmf8829 fake, string name)
    {
        foreach (var f in Meas[name].GetProperty("frames").EnumerateArray())
            fake.EmitFrame(Raw[f.GetProperty("raw_case").GetString()!], f.GetProperty("timestamp_us").GetInt64(), f.GetProperty("edge_us").GetInt64());
    }

    [Fact]
    public void InitRunsTheContractSequenceAndDownloadsTheFirmware()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        var progress = new List<(int, int)>();
        var (ver, serial) = dev.Init((d, t) => progress.Add((d, t)));
        Assert.Equal(("2.200.0", 0x12345678u), (ver, serial));
        Assert.Null(dev.Config);

        var w = fake.Writes();
        Assert.Equal(new[] { 0xF8, 0x14 }, w[0]);   // PON | bootmonitor
        Assert.Equal(new[] { 0xF7, 0x40 }, w[1]);   // soft reset
        Assert.Equal(new byte[] { 0xE3, 1, 0 }, fake.Sent(R)[0]);
        Assert.Equal(new[] { 0x08, 0x22 }, w[2]);   // BL I2C_OFF
        Assert.Equal(new[] { 0x08, 0x45, 6, 0x00, 0x00, 0x01, 0x00, 3611 & 0xFF, 3611 >> 8 }, w[3]);
        var fifo = w.Where(x => x[0] == 0xFF).ToList();
        Assert.Equal((14444 + 1022) / 1023, fifo.Count);
        Assert.Equal(Tmf8829Firmware.Image(), fake.Fifo.ToArray());
        var after = w.Skip(w.IndexOf(fifo[^1]) + 1).ToList();
        Assert.Equal(new[] { 0x08, 0x16 }, after[0]);  // START_RAM_APP
        Assert.Equal(new[] { 0xF8, 0xA4 }, after[1]);  // powerup_select = RAM
        Assert.Equal(fifo.Count, progress.Count);
        Assert.Equal((14444, 14444), progress[^1]);
        Assert.Equal(new[] { new byte[] { 0x00, 4, 0 }, new byte[] { 0x1C, 4, 0 } }, fake.Sent(R).TakeLast(2));
        Assert.Equal("app", fake.Mode);
    }

    [Fact]
    public void InitSurvivesABusyCmdStatAndRefusesAWrongSensor()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        fake.BusyPolls = 3;
        Assert.Equal("2.200.0", dev.Init().AppVersion);
        fake.DeviceId = 0x00;
        var e = Assert.Throws<Tmf8829Exception>(() => dev.Init());
        Assert.Contains("sensor ID 0x00, expected 0x9E", e.Message);
        fake.DeviceId = 0x9E;
        fake.BlStatus[0x45] = 2;
        Assert.Equal("bootloader cmd 0x45 -> ERR_ADDR", Assert.Throws<Tmf8829Exception>(() => dev.Init()).Message);
        fake.BlStatus.Clear();
        fake.BlStatus[0x16] = 7;
        Assert.Equal("bootloader cmd 0x16 -> 0x7", Assert.Throws<Tmf8829Exception>(() => dev.Init()).Message);
        fake.BlStatus.Clear();
        fake.StuckBeforeBootloader = true;
        Assert.Contains("sensor bootloader: timeout, reg 0x00 = 0x00", Assert.Throws<Tmf8829Exception>(() => dev.Init()).Message);
    }

    [Fact]
    public void ConfigureWritesEveryFieldAPresetDoesNotReset()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        fake.Cfg[0x2B - 0x22] = 1;
        fake.Cfg[0x2C - 0x22] = 1;
        fake.Cfg[0x2A - 0x22] = 0x3C;
        fake.Cfg[0] = 100;
        var cfg = dev.Configure();
        var w = fake.Writes();
        Assert.Equal(new[] { 0x08, 0x40 }, w[0]);
        Assert.Equal(new[] { 0x08, 0x16 }, w[1]);
        Assert.Equal((0x22, 191), (w[2][0], w[2].Length));
        Assert.Equal(new[] { 0x08, 0x15 }, w[3]);
        Assert.Equal(new[] { 0x08, 0x16 }, w[4]);
        Assert.Equal(("8x8", (8, 8), false, false), (cfg.Preset, cfg.Grid, cfg.Histograms, cfg.RefFrame));
        var want = new Dictionary<string, int>
        {
            ["period_ms"] = 33, ["kilo_iterations"] = 586, ["fp_mode"] = 0, ["nr_peaks"] = 1, ["signal_strength"] = 0,
            ["noise_strength"] = 0, ["xtalk"] = 0, ["dump_histograms"] = 0, ["ref_spad_frame"] = 0,
        };
        foreach (var kv in want)
            Assert.Equal(kv.Value, cfg.Fields[kv.Key]);
        Assert.Same(cfg, dev.Config);
    }

    [Fact]
    public void ConfigureWritesTheCallerOptionsAndRefusesBadOnes()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        var cfg = dev.Configure("32x32", periodMs: 300, nrPeaks: 2, signalStrength: true, noiseStrength: true, xtalk: true,
            histograms: true, refFrame: true, fields: new Dictionary<string, int> { ["power_modes"] = 3 });
        Assert.Equal(new[] { 0x08, 0x45 }, fake.Writes()[0]);
        var page = fake.Writes()[2].Skip(1).ToArray();
        Assert.Equal(300, page[0] | page[1] << 8);
        Assert.Equal(2344, page[2] | page[3] << 8);
        Assert.Equal(2 | 0x08 | 0x10 | 0x20, page[0x2A - 0x22]);
        Assert.Equal((1, 1, 3), (page[0x2B - 0x22], page[0x2C - 0x22], page[0x2E - 0x22]));
        Assert.Equal(((32, 32), true, true, 3), (cfg.Grid, cfg.Histograms, cfg.RefFrame, cfg.Fields["fp_mode"]));

        int n = fake.LogCount;
        Assert.Throws<ArgumentException>(() => dev.Configure("9x9"));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.Configure(nrPeaks: 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.Configure(nrPeaks: 5));
        Assert.Equal(n, fake.LogCount);

        fake.AppStatus[0x15] = 0x04;
        Assert.Equal("sensor cmd 0x15 -> ERR_CONFIG_RESULT_SIZE",
            Assert.Throws<Tmf8829Exception>(() => dev.Configure("48x32", nrPeaks: 4)).Message);
        fake.AppStatus.Clear();
        fake.StuckBytes[0x78 - 0x22] = 5;
        Assert.Equal("config fields did not stick: fov_correction wrote 3, reads 5",
            Assert.Throws<Tmf8829Exception>(() => dev.Configure(fields: new Dictionary<string, int> { ["fov_correction"] = 3 })).Message);
        fake.StuckBytes.Clear();
        fake.CidOverride = 0x40;
        Assert.Equal("CID_RID 0x40 after LOAD_CONFIG_PAGE", Assert.Throws<Tmf8829Exception>(() => dev.ConfigPage()).Message);
    }

    [Fact]
    public void StartArmsTheStreamThenMeasuresAndStopUndoesIt()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        dev.Start();
        Assert.True(dev.Streaming);
        Assert.Equal("8x8", dev.Config!.Preset);
        var w = fake.Writes();
        int i = w.FindIndex(x => x[0] == 0xFA);
        Assert.Equal(new[] { new[] { 0xFA, 0x30 }, new[] { 0xE1, 0xFF }, new[] { 0xE2, 0x01 } }, w.Skip(i).Take(3));
        Assert.Equal(new[] { new byte[] { 0x01, 0x00 } }, fake.Sent(0x35));
        Assert.Equal(new[] { 0x08, 0x10 }, w[^1]); // MEASURE after the stream is armed
        Assert.Throws<InvalidStateException>(() => dev.Configure());
        dev.Start(); // no-op while streaming
        Assert.Single(fake.Sent(0x35));

        int n = fake.LogCount;
        dev.Stop();
        var tail = fake.LogFrom(n).Where(p => p.Cmd != R).Select(p => new[] { p.Cmd }.Concat(p.Payload.Select(b => (int)b)).ToArray());
        Assert.Equal(new[] { new[] { W, 0x08, 0xFF }, new[] { W, 0xE2, 0x00 }, new[] { W, 0xE1, 0xFF }, new[] { 0x36 } }, tail);
        Assert.False(dev.Streaming);
        dev.Stop(); // the sensor is stopped again, the bridge left alone
        Assert.Single(fake.Sent(0x36));

        dev.Configure("8x8", histograms: true, refFrame: true);
        dev.Start();
        Assert.Contains(fake.Writes(), x => x.SequenceEqual(new[] { 0xE2, 0x09 }));
        Assert.Equal(new byte[] { 0x09, 0x01 }, fake.Sent(0x35)[^1]);
        dev.Stop();

        fake.AppStatus[0x10] = 0x03;
        Assert.Equal("sensor cmd 0x10 -> ERR_APPLICATION", Assert.Throws<Tmf8829Exception>(() => dev.Start()).Message);
        Assert.False(dev.Streaming);
        Assert.Equal(3, fake.Sent(0x36).Count);
    }

    [Fact]
    public void StopSwallowsTheBadHeaderAStopRaisesAndOtherEventsPass()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        var events = new List<Tmf8829InterruptEvent>();
        var sub = dev.OnEvent(e =>
        {
            if (e is Tmf8829InterruptEvent t)
                lock (events) events.Add(t);
        });
        dev.Start();
        fake.OnCommand = cmd => cmd == 0xFF ? new[] { fake.Event(Tmf8829EventCode.BadHeader, 0x08, 0xFF, 0x04) } : Array.Empty<byte[]>();
        dev.Stop();
        Thread.Sleep(50);
        lock (events) Assert.Empty(events);

        fake.Link.Inject(fake.Link.Reply(0x94, new byte[5])); // truncated: ignored
        fake.EmitEvent(Tmf8829EventCode.Int, 0x02);
        fake.EmitEvent(Tmf8829EventCode.BadHeader, 0x01, 0x55);
        WaitFor(events, 2);
        Assert.Equal(new Tmf8829Event(987654321, 0x02, 0, 0, 0), events[0].Event);
        Assert.Equal((1, 0x55), (events[1].Event.Code, events[1].Event.FrameId));
        sub.Dispose();
    }

    [Fact]
    public void CloseStopsARunningStreamEvenWhenTheStopIsRefused()
    {
        var (fake, dev) = Make();
        dev.Start();
        dev.Close();
        Assert.Contains(fake.Writes(), x => x.SequenceEqual(new[] { 0x08, 0xFF }));
        Assert.Single(fake.Sent(0x36));

        var (fake2, dev2) = Make();
        dev2.Start();
        fake2.AppStatus[0xFF] = 0x0A;
        dev2.Close();
        Assert.True(dev2.IsClosed);
    }

    public static IEnumerable<object[]> MeasurementNames() => Meas.Keys.Select(k => new object[] { k });

    [Theory]
    [MemberData(nameof(MeasurementNames))]
    public void StreamedFramesMatchTheGoldenMeasurements(string name)
    {
        var (fake, dev) = Make();
        using var _ = dev;
        var got = new List<Tmf8829Measurement>();
        dev.OnMeasurement(m => { lock (got) got.Add(m); });
        dev.Start();
        Play(fake, name);
        var want = Meas[name].GetProperty("expect").EnumerateArray().ToArray();
        WaitFor(got, want.Length);
        var shadow = new Tmf8829MeasurementBuilder();
        for (int k = 0; k < want.Length; k++)
        {
            var fields = VectorShapes.Tmf8829Meas(got[k], shadow);
            fields["ratio_q16"] = got[k].Times[^1].RatioQ16;
            Tmf8829VectorTests.AssertFields($"{name}#{k}", want[k], fields);
        }
        int nCorrupt = Meas[name].GetProperty("frames").EnumerateArray()
            .Count(f => Corrupt.Contains(f.GetProperty("raw_case").GetString()!));
        Assert.Equal((0, 0, nCorrupt), (dev.FramesDropped, dev.FramesDiscarded, dev.FrameParseErrors));
        Assert.Equal(want[^1].GetProperty("ratio_q16").GetUInt32(), dev.ClockRatioQ16);
    }

    [Fact]
    public void HistogramFramesAreReassembledFrom1024ByteChunks()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        var got = new List<Tmf8829Measurement>();
        dev.OnMeasurement(m => { lock (got) got.Add(m); });
        dev.Start();
        Play(fake, "8x8_hist");
        WaitFor(got, 1);
        Assert.Equal(new[] { 25377, 25377 }, got[0].Histograms.Select(h => h.Raw.Length));
        Assert.Equal(Raw["8x8_hist#0"], got[0].Histograms[0].Raw);
    }

    [Fact]
    public void GetMeasurementTakesTheNextAndTimesOut()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        using var all = dev.Measurements(4);
        dev.Start();
        var t = new Thread(() => { Thread.Sleep(50); fake.EmitFrame(Raw["8x8#0"], 2125575099, 2125575071); });
        t.Start();
        Assert.Equal(1u, dev.GetMeasurement(TimeSpan.FromSeconds(2)).FNumber);
        t.Join();
        Assert.Equal(1u, all.Take(TimeSpan.FromSeconds(1)).FNumber);
        Assert.Throws<DepzTimeoutException>(() => dev.GetMeasurement(TimeSpan.FromMilliseconds(100)));
    }

    [Fact]
    public void LostAndBrokenFramesAreCountedNotRaised()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        var got = new List<Tmf8829Measurement>();
        dev.OnMeasurement(m => { lock (got) got.Add(m); });
        dev.Start();
        fake.EmitFrame(Raw["8x8#0"], 2125575099, 2125575071, seq: 0);
        fake.EmitFrame(Raw["8x8#1"], 2125672180, 2125672161, seq: 3);              // two frames dropped by the bridge
        fake.EmitFrame(Raw["8x8_full#0"], 2125937131, 0, seq: 4, chunks: new[] { 0 }); // cut short
        fake.EmitFrame(Raw["8x8#1"].AsSpan(0, 40).ToArray(), 1, 0, seq: 5);         // parse error
        fake.Link.Inject(fake.Link.Reply(0x93, new byte[10]));                       // truncated chunk header
        WaitFor(got, 2);
        Thread.Sleep(50);
        Assert.Equal((2, 1, 1), (dev.FramesDropped, dev.FramesDiscarded, dev.FrameParseErrors));
        dev.Stop();
        dev.Start();
        Assert.Equal((0, 0, 0), (dev.FramesDropped, dev.FramesDiscarded, dev.FrameParseErrors));
    }

    [Fact]
    public void RegisterAccessChunksAndChecksItsReplies()
    {
        var (fake, dev) = Make();
        using var _ = dev;
        Assert.Equal(2000, dev.ReadRegisters(0xF0, 2000).Length);
        Assert.Equal(new[] { new byte[] { 0xF0, 0x00, 0x04 }, new byte[] { 0xFF, 976 & 0xFF, 976 >> 8 } }, fake.Sent(R));

        var progress = new List<(int, int)>();
        dev.WriteRegisters(0xFF, Enumerable.Repeat((byte)7, 2100).ToArray(), (d, t) => progress.Add((d, t)));
        Assert.Equal(new[] { (0xFF, 1023), (0xFF, 1023), (0xFF, 54) }, fake.Sent(W).Select(p => ((int)p[0], p.Length - 1)));
        Assert.Equal(new[] { (1023, 2100), (2046, 2100), (2100, 2100) }, progress);

        dev.WriteRegister(0xE2, 0x109);
        Assert.Equal(new byte[] { 0xE2, 0x09 }, fake.Sent(W)[^1]);

        int n = fake.LogCount;
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.ReadRegisters(0x100, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.WriteRegisters(0x100, new byte[1]));
        Assert.Equal(n, fake.LogCount);

        fake.ShortRead = 1;
        Assert.Equal("READ_REG 0x00: expected 4, got 3", Assert.Throws<DepzException>(() => dev.ReadRegisters(0x00, 4)).Message);
        fake.ShortRead = 0;
        fake.BridgeStatus[W] = 0x03;
        Assert.Throws<StatusException>(() => dev.WriteRegisters(0xE2, new byte[] { 1 }));
        fake.BridgeStatus.Clear();
        dev.WriteRegisters(0xE2, new byte[] { 1 });
        Assert.Equal(1, fake.Regs[0xE2]);
    }

    [Fact]
    public void SensorStateAndBridgeInfo()
    {
        var (_, dev) = Make();
        using var __ = dev;
        dev.Init();
        var st = dev.SensorState();
        Assert.Equal(new[] { "ID", "REVID", "ENABLE", "RESET", "APP_ID", "CMD_STAT", "INT_STATUS", "INT_ENAB", "FIFOSTATUS" }, st.Keys);
        Assert.Equal((0x9E, 0x01, 0x80), (st["ID"], st["APP_ID"], st["ENABLE"] & 0x80));
        dev.Start();
        var info = dev.GetBridgeInfo();
        Assert.Equal((7u, 6u, 1u), (info.IntEdges, info.Frames, info.FramesAborted));
        Assert.Equal((true, (byte)0x01), (info.Streaming, info.IntMask));
    }
}
