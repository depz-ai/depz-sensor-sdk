using System.Buffers.Binary;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;

namespace Depz.Sensor.Tests;

/// <summary>
/// The VL53L8 SPI bridge (APP_VL53L8, contract 04) or the L5/L7 I2C board (APP_VL53L7, contract 11)
/// in-process over a <see cref="FakeBoardLink"/>: READ_REG / WRITE_REG land on a
/// <see cref="FakeVl53l8Platform"/> (the emulated ULD transport of the driver tests), so the device
/// classes' init, configuration and ranging run end to end over the wire. The I2C board adds
/// PIN_CTRL, GET_INFO, SET_I2C_SPEED, its transfer limits and the RPT_VL53_INFO counters — the
/// Python tests' fake_vl53l7.py board logic. Firmware banks are counted, not stored (as the platform).
/// </summary>
internal sealed class FakeVl53l8Board
{
    public const string L8Software = "APP_VL53L8_v0.92";
    public const string L8DeviceName = "DEPZ ToF Sensor VL53L8 USB v2.1 SER8";
    public const string L7Software = "APP_VL53L7_v0.53";
    public const string L7DeviceName = "DEPZ ToF Sensor VL53L7CX USB v2.1 TXK5KAX6X4";

    /// <summary><see cref="FailNext"/> status meaning "answer nothing at all".</summary>
    public const int Silent = -1;

    private const int UiCmdStart = 0x2C04, DciReadCmdAddr = 0x2FF4, GetXtalkAddr = 0x2FB8, GetNvmAddr = 0x2FD8;

    public readonly FakeBoardLink Link = new();
    public readonly FakeVl53l8Platform P;
    public readonly bool I2c;
    public readonly string SoftwareName;
    public readonly string DeviceName;
    public ulong McuTimeUs = 1_000_000;

    // The I2C board's state (contract 11 §2).
    public volatile bool Streaming;
    public volatile bool Lpn = true;
    public volatile int FrameSize;
    public uint IntEdges;
    public uint FramesDropped = 0; // the fake never drops; the counter is only reported
    public uint I2cErrors;
    public byte LastI2cError = Vl53l7I2cError.Ok;
    public int I2cKhz = 1000; // the firmware default since v0.52
    public readonly List<int> PinActions = new();
    public readonly List<int> SpeedRequests = new();
    public int ChunksSent;

    /// <summary>Firmware bytes written per bank page (0x09..0x0B).</summary>
    public readonly Dictionary<int, int> BankBytes = new();

    private readonly Dictionary<int, Queue<int>> _failures = new();
    private readonly object _lock = new();
    private int _page = -1;
    // Whether the sensor holds a DCI / xtalk command whose answer the next UI_CMD_START read is.
    // The SW reboot that opens every init forgets it (the platform fake keeps a stale one).
    private bool _sensorHasCommand;

    public FakeVl53l8Board(FakeVl53l8Platform platform, bool i2cBoard)
    {
        P = platform;
        I2c = i2cBoard;
        SoftwareName = i2cBoard ? L7Software : L8Software;
        DeviceName = i2cBoard ? L7DeviceName : L8DeviceName;
        Link.OnPacket = Handle;
    }

    /// <summary>Answer the next <paramref name="cmd"/> with <paramref name="status"/> (or <see cref="Silent"/>).</summary>
    public void FailNext(int cmd, int status)
    {
        lock (_lock)
        {
            if (!_failures.TryGetValue(cmd, out var q))
                _failures[cmd] = q = new Queue<int>();
            q.Enqueue(status);
        }
    }

    /// <summary>Every host opcode so far, in order.</summary>
    public List<int> Cmds()
    {
        lock (Link.Sent)
            return Link.Sent.Select(p => p.Cmd).ToList();
    }

    public void ClearSent()
    {
        lock (Link.Sent)
            Link.Sent.Clear();
    }

    /// <summary>One INT edge → one frame in chunks of at most <paramref name="chunkMax"/> bytes (RPT_VL53_FRAME).</summary>
    public void SendFrame(byte[] raw, ulong tsUs, int chunkMax)
    {
        IntEdges++;
        for (int off = 0; off < raw.Length; off += chunkMax)
            SendChunk(raw, tsUs, off, Math.Min(chunkMax, raw.Length - off), raw.Length);
    }

    public void SendChunk(byte[] raw, ulong tsUs, int off, int n, int fullSize)
    {
        var b = new byte[12 + n];
        BinaryPrimitives.WriteUInt64LittleEndian(b, tsUs);
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(8), (ushort)fullSize);
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(10), (ushort)off);
        Array.Copy(raw, off, b, 12, n);
        Link.Inject(Link.Reply((int)Vl53l8Rpt.Vl53Frame, b));
        ChunksSent++;
    }

    /// <summary>RPT_VL53_INFO: the bridge counters (contract 11 §2), no echoed command byte.</summary>
    public byte[] InfoPayload()
    {
        var b = new byte[Vl53l7Wire.InfoSize];
        BinaryPrimitives.WriteUInt32LittleEndian(b, IntEdges);
        BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(4), FramesDropped);
        BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(8), I2cErrors);
        b[12] = LastI2cError;
        b[13] = (byte)(Lpn ? 1 : 0);
        b[14] = (byte)(Streaming ? 1 : 0);
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(15), (ushort)I2cKhz);
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(17), (ushort)FrameSize);
        b[19] = (byte)(Streaming ? 1 : 0);
        return b;
    }

    private static byte[] Status(int cmd, Status s) => FakeBoardLink.Status(cmd, s);

    private byte[] Text(int cmd, string s) =>
        Link.Reply((int)Rpt.Text, new[] { (byte)cmd }.Concat(System.Text.Encoding.ASCII.GetBytes(s)).ToArray());

    private IEnumerable<byte[]> Handle(Packet pkt)
    {
        lock (_lock)
            return HandleLocked(pkt).ToList();
    }

    private IEnumerable<byte[]> HandleLocked(Packet pkt)
    {
        int cmd = pkt.Cmd;
        byte[] pl = pkt.Payload;
        if (_failures.TryGetValue(cmd, out var q) && q.Count > 0)
        {
            int fail = q.Dequeue();
            return fail == Silent ? Array.Empty<byte[]>() : new[] { FakeBoardLink.Status(cmd, (Status)fail) };
        }
        switch (cmd)
        {
            case (int)Cmd.GetNameActiveSoftware:
                return new[] { Text(cmd, SoftwareName) };
            case (int)Cmd.GetDeviceName:
                return new[] { Text(cmd, DeviceName) };
            case (int)Cmd.GetSerial:
                return new[] { Text(cmd, "SN000007") };
            case (int)Vl53l8Cmd.ReadReg:
            case (int)Vl53l8Cmd.WriteReg:
                return Register(cmd, pl);
            case (int)Vl53l8Cmd.StartStream:
                FrameSize = U16(pl, 0);
                Streaming = true;
                return new[] { Status(cmd, Protocol.Status.Ok) };
            case (int)Vl53l8Cmd.StopStream:
                Streaming = false;
                return new[] { Status(cmd, Protocol.Status.Ok) };
        }
        if (I2c && cmd == (int)Vl53l7Cmd.PinCtrl)
            return PinCtrl(cmd, pl);
        if (I2c && cmd == (int)Vl53l7Cmd.GetInfo)
            return new[] { Link.Reply((int)Vl53l7Rpt.Vl53Info, InfoPayload()) };
        if (I2c && cmd == (int)Vl53l7Cmd.SetI2cSpeed)
            return SetI2cSpeed(cmd, pl);
        return new[] { Status(cmd, Protocol.Status.ErrInvalidCmd) };
    }

    private IEnumerable<byte[]> Register(int cmd, byte[] pl)
    {
        bool read = cmd == (int)Vl53l8Cmd.ReadReg;
        int addr = U16(pl, 0);
        int n = read ? U16(pl, 2) : pl.Length - 2;
        if (I2c)
        {
            int max = read ? Vl53l7Wire.ReadMaxLen : Vl53l7Wire.WriteMaxLen;
            if (n < 1 || n > max || addr + n > 0x10000)
                return new[] { Status(cmd, Protocol.Status.ErrInvalidParam) };
            if (!Lpn) // LPn low: the sensor NACKs
            {
                McuTimeUs += 120;
                I2cErrors++;
                LastI2cError = Vl53l7I2cError.Nack;
                return new[] { Status(cmd, Protocol.Status.ErrHardwareFault) };
            }
        }
        if (read)
        {
            byte[] data = addr == UiCmdStart && !_sensorHasCommand
                ? Enumerable.Range(0, n).Select(i => P.Reg.TryGetValue(addr + i, out byte b) ? b : (byte)0).ToArray()
                : P.ReadMulti((ushort)addr, n);
            if (data.Length > n) // the sensor answers exactly what was asked
                data = data[..n];
            var b = new byte[9 + data.Length];
            b[0] = (byte)cmd;
            BinaryPrimitives.WriteUInt64LittleEndian(b.AsSpan(1), McuTimeUs);
            data.CopyTo(b, 9);
            McuTimeUs += 100;
            return new[] { Link.Reply((int)Vl53l8Rpt.RegData, b) };
        }
        byte[] wr = pl[2..];
        if (addr == 0x7FFF && wr.Length == 1)
            _page = wr[0];
        if (_page == 0 && addr == 0x0009 && wr.Length == 1 && wr[0] == 0x04)
            _sensorHasCommand = false; // the SW reboot that opens every init
        if (addr == GetNvmAddr)
            _sensorHasCommand = false; // the NVM answer is a plain register read
        if ((addr == DciReadCmdAddr && wr.Length == 12) || addr == GetXtalkAddr)
            _sensorHasCommand = true;
        if (_page >= 0x09 && _page <= 0x0B && wr.Length >= 1024) // a firmware bank, in the host's write chunks
        {
            BankBytes[_page] = (BankBytes.TryGetValue(_page, out int have) ? have : 0) + wr.Length;
            return new[] { Status(cmd, Protocol.Status.Ok) };
        }
        P.WriteMulti((ushort)addr, wr);
        return new[] { Status(cmd, Protocol.Status.Ok) };
    }

    private IEnumerable<byte[]> PinCtrl(int cmd, byte[] pl)
    {
        if (pl.Length != 1)
            return new[] { Status(cmd, Protocol.Status.ErrPayloadFormat) };
        int action = pl[0];
        if (action > Vl53l7PinAction.SoftCycle)
            return new[] { Status(cmd, Protocol.Status.ErrInvalidParam) };
        PinActions.Add(action);
        switch (action)
        {
            case Vl53l7PinAction.LpnOff:
                Streaming = false;
                Lpn = false;
                break;
            case Vl53l7PinAction.LpnOn:
                Lpn = true;
                break;
            case Vl53l7PinAction.SoftCycle:
                Streaming = false;
                Lpn = true;
                I2cErrors = 0;
                LastI2cError = Vl53l7I2cError.Ok;
                break;
        }
        return new[] { Status(cmd, Protocol.Status.Ok) };
    }

    private IEnumerable<byte[]> SetI2cSpeed(int cmd, byte[] pl)
    {
        if (pl.Length != 2)
            return new[] { Status(cmd, Protocol.Status.ErrPayloadFormat) };
        int khz = U16(pl, 0);
        SpeedRequests.Add(khz);
        if (Streaming)
            return new[] { Status(cmd, Protocol.Status.ErrBusy) };
        int best = Vl53l7Wire.I2cKhzSteps[0];
        foreach (int s in Vl53l7Wire.I2cKhzSteps)
            if (Math.Abs(s - khz) < Math.Abs(best - khz))
                best = s;
        I2cKhz = best;
        return new[] { Status(cmd, Protocol.Status.Ok) };
    }

    private static int U16(byte[] b, int off) => b[off] | b[off + 1] << 8;
}

/// <summary>
/// A device class over its <see cref="FakeVl53l8Board"/>: the booting sensor the driver tests use
/// (silicon ids, boot flags, checksum word, module type), the device on top, and the raw frame the
/// programmed output list expects.
/// </summary>
internal sealed class Vl53l8Rig : IDisposable
{
    public static readonly TimeSpan Timeout = TimeSpan.FromMilliseconds(500);
    public const int L7Frame8x8 = 1444; // contract 11 §2: one 1536-byte chunk
    public const int L7Frame4x4 = 1060;
    public const int L8Frame8x8 = 1452;
    public const int L8Frame4x4 = 540;
    public const int DciOutputList = 0xD980, DciFwFlags = 0xE0C4, DciFreqHz = 0x5458, DciIntTime = 0x545C;
    public const int DciRangingMode = 0xAD30, DciTargetOrder = 0xAE64, DciMotionCfg = 0xBFAC;

    public readonly Vl53l8cxDevice Dev;
    public readonly FakeVl53l8Board Board;

    public FakeVl53l8Platform P => Board.P;

    private Vl53l8Rig(Vl53l8cxDevice dev, FakeVl53l8Board board)
    {
        Dev = dev;
        Board = board;
    }

    /// <summary>A booting sensor of <paramref name="fw"/> behind the matching board, reporting
    /// <paramref name="moduleType"/> (L5/L7 only).</summary>
    public static FakeVl53l8Board NewBoard(Vl53l8Firmware fw, int moduleType = Vl53l8Driver.ModuleTypeMzevo)
    {
        var p = new FakeVl53l8Platform();
        bool i2c = fw == Vl53l8Firmware.L7cx || fw == Vl53l8Firmware.L7ch;
        p.Reg[0x06] = 0x01;                  // sensor boot / MCU boot
        p.Reg[0x21] = (byte)(i2c ? 0x10 : 0x04);
        if (fw == Vl53l8Firmware.Cx)
            p.SetU32(0x2FFC, 0xCADF7CAF);
        else if (fw != Vl53l8Firmware.L7cx)
            p.SetU32(0x2FFC, 0x0C0B6C9E);
        p.Reg[0x00] = 0xF0;
        p.Reg[0x01] = (byte)(i2c ? 0x02 : 0x0C);
        if (i2c)
            p.Dci[DciFwFlags] = new byte[] { 0, (byte)moduleType, 0, 0, 0, 0, 0, 0 };
        return new FakeVl53l8Board(p, i2c);
    }

    public static Vl53l8Rig L7(int moduleType = Vl53l8Driver.ModuleTypeMzevo)
    {
        var b = NewBoard(Vl53l8Firmware.L7cx, moduleType);
        return new Vl53l8Rig(new Vl53l7cxDevice(b.Link, Timeout), b);
    }

    public static Vl53l8Rig L5(int moduleType = Vl53l8Driver.ModuleTypeMz)
    {
        var b = NewBoard(Vl53l8Firmware.L7cx, moduleType);
        return new Vl53l8Rig(new Vl53l5cxDevice(b.Link, Timeout), b);
    }

    public static Vl53l8Rig L7ch(int moduleType = Vl53l8Driver.ModuleTypeMzevo)
    {
        var b = NewBoard(Vl53l8Firmware.L7ch, moduleType);
        return new Vl53l8Rig(new Vl53l7chDevice(b.Link, Timeout), b);
    }

    public static Vl53l8Rig Cx()
    {
        var b = NewBoard(Vl53l8Firmware.Cx);
        return new Vl53l8Rig(new Vl53l8cxDevice(b.Link, Timeout), b);
    }

    public static Vl53l8Rig Ch()
    {
        var b = NewBoard(Vl53l8Firmware.Ch);
        return new Vl53l8Rig(new Vl53l8chDevice(b.Link, Timeout), b);
    }

    /// <summary>Init(), then the zone config and firmware frame size the ULD reads before ranging
    /// at <paramref name="resolution"/>.</summary>
    public void Ready(int resolution, int frameSize)
    {
        Dev.Init();
        P.Zones(resolution);
        P.FwReports(frameSize);
        Board.ClearSent();
    }

    /// <summary>A raw frame (as read from register 0) for the programmed output list: every zone's
    /// distance <paramref name="mm"/>, one target per zone; the id echo in header and footer agree.</summary>
    public byte[] Frame(int mm) => Frame((idx, msize) =>
    {
        var b = new byte[msize];
        if (idx == 0xDF44)
            for (int k = 0; 2 * k + 2 <= msize; k++)
                BinaryPrimitives.WriteInt16LittleEndian(b.AsSpan(2 * k), (short)(mm * 4));
        if (idx == 0xDB84)
            for (int k = 0; k < Math.Min(64, msize); k++)
                b[k] = 1;
        return b;
    });

    public byte[] Frame(Func<int, int, byte[]> fill)
    {
        var output = FakeVl53l8Platform.U32s(P.Dci[DciOutputList]);
        var buf = new byte[Dev.Uld.DataReadSize];
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
        int footer = FooterOffset;
        buf[buf.Length - footer] = 0x12;
        buf[buf.Length - footer + 1] = 0x34;
        return Vl53l8FrameDecoder.SwapBuffer(buf);
    }

    public int FooterOffset => Dev.Firmware == Vl53l8Firmware.Cx
        ? Vl53l8FrameDecoder.FooterIdOffsetCx
        : Vl53l8FrameDecoder.FooterIdOffsetCh;

    /// <summary>Run <paramref name="action"/> after a short delay on another thread.</summary>
    public static Task Later(Action action) => Task.Run(() =>
    {
        Thread.Sleep(50);
        action();
    });

    public void Dispose()
    {
        Dev.Close();
        Board.Link.Close();
    }
}
