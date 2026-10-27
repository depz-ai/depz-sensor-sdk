using System.Buffers.Binary;
using System.Text;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53lx;

namespace Depz.Sensor.Tests;

/// <summary>
/// The <c>APP_VL53L0_4</c> register bridge in-process (contract 12, protocol v2.01): the Python tests'
/// <c>fake_vl53lx.py</c> over a <see cref="FakeBoardLink"/>. The MCU knows no sensor, so neither does this
/// fake: behind the bridge sits a 64 KiB register file pre-loaded with what a booted VL53L1-family die
/// answers (boot status 3, oscillator, model id, PLL, ROI centre) — enough for the host ULDs to run their
/// init, timing, offset, xtalk, threshold, calibration and ranging sequences. The result block (0x0089,
/// 17 B) and the data-ready bit (0x0031 bit 0) are plain registers.
/// Every packet the host sends is logged (<see cref="Rx"/>); <see cref="FailNext"/> answers the next
/// <c>cmd</c> with an error status (or nothing); <see cref="Stream(byte[], long, int, int?)"/> pushes an
/// RPT_VL53_STREAM.
/// </summary>
internal sealed class FakeVl53lxBridge
{
    public const string SoftwareName = "APP_VL53L0_4_v0.25";
    public const int DieModelId = 0xEBAA;
    public const int DieOsc = 0xBCCC;
    public const int DiePll = 0x0123;

    public const int ReadReg = (int)Vl53lxCmd.ReadReg;
    public const int WriteReg = (int)Vl53lxCmd.WriteReg;
    public const int Xshut = (int)Vl53lxCmd.Xshut;
    public const int StartStream = (int)Vl53lxCmd.StartStream;
    public const int StopStream = (int)Vl53lxCmd.StopStream;
    public const int GetInfo = (int)Vl53lxCmd.GetInfo;
    public const int SetI2cSpeed = (int)Vl53lxCmd.SetI2cSpeed;
    public const int SetAddrWidth = (int)Vl53lxCmd.SetAddrWidth;
    public const int ClearI2cErrors = (int)Vl53lxCmd.ClearI2cErrors;
    public const int GetDeviceName = (int)Cmd.GetDeviceName;

    /// <summary>"silent": no reply at all to the next command of that opcode.</summary>
    public const int Silent = -1;

    public readonly FakeBoardLink Link = new();
    public readonly byte[] Regs = new byte[0x10000];

    /// <summary>Every packet the host sent, in order.</summary>
    public readonly List<(int Cmd, byte[] Payload)> Rx = new();

    public long McuTimeUs = 5_000_000;
    public byte[] Info = new byte[Vl53lxWire.InfoSize];
    public readonly string DeviceName;
    public readonly bool NoClear;

    private readonly Dictionary<int, Queue<int>> _failures = new();
    private readonly object _lock = new();

    public FakeVl53lxBridge(string deviceName, bool noClearI2cErrors = false)
    {
        DeviceName = deviceName;
        NoClear = noClearI2cErrors;
        Regs[0x00E5] = 0x03;
        SetWord(0x0006, DieOsc);
        SetWord(0x010F, DieModelId);
        SetWord(0x00DE, DiePll);
        Regs[0x013E] = 0x91; // ROI_CONFIG__MODE_ROI_CENTRE_SPAD
        Link.OnPacket = Handle;
    }

    // ── register file ────────────────────────────────────────────────────────

    public void SetWord(int addr, int v)
    {
        Regs[addr] = (byte)(v >> 8);
        Regs[addr + 1] = (byte)v;
    }

    public int Word(int addr) => Regs[addr] << 8 | Regs[addr + 1];

    public long Dword(int addr) => (long)Word(addr) << 16 | (uint)Word(addr + 2);

    /// <summary>Put a result block at 0x0089 (what the next poll reads).</summary>
    public void SetResultBlock(byte[] raw) => Array.Copy(raw, 0, Regs, 0x0089, raw.Length);

    // ── traffic ──────────────────────────────────────────────────────────────

    public void FailNext(int cmd, int status)
    {
        lock (_lock)
        {
            if (!_failures.TryGetValue(cmd, out var q))
                _failures[cmd] = q = new Queue<int>();
            q.Enqueue(status);
        }
    }

    /// <summary>Payloads of one opcode, in order.</summary>
    public List<byte[]> Sent(int cmd)
    {
        lock (Rx)
            return Rx.Where(e => e.Cmd == cmd).Select(e => e.Payload).ToList();
    }

    public List<int> Cmds()
    {
        lock (Rx)
            return Rx.Select(e => e.Cmd).ToList();
    }

    /// <summary>Every WRITE_REG as (addr, data).</summary>
    public List<(int Addr, byte[] Data)> Writes() =>
        Sent(WriteReg).Select(p => (U16(p, 0), p.Skip(2).ToArray())).ToList();

    /// <summary>The register writes as "ADDR:hex" strings, for exact comparison.</summary>
    public List<string> WriteKeys() => Writes().Select(w => $"{w.Addr:X4}:{ToHex(w.Data)}").ToList();

    public static string Key(int addr, params int[] data) => $"{addr:X4}:{ToHex(data.Select(b => (byte)b).ToArray())}";

    public void ClearRx()
    {
        lock (Rx)
            Rx.Clear();
    }

    public int RxCount()
    {
        lock (Rx)
            return Rx.Count;
    }

    /// <summary>RPT_VL53_STREAM: ts u64, addr u16, len u16, data.</summary>
    public void Stream(byte[] data, long tsUs, int addr = 0x0089, int? length = null)
    {
        int n = length ?? data.Length;
        var b = new byte[12 + data.Length];
        BinaryPrimitives.WriteUInt64LittleEndian(b, (ulong)tsUs);
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(8), (ushort)addr);
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(10), (ushort)n);
        Array.Copy(data, 0, b, 12, data.Length);
        Send((int)Vl53lxRpt.Stream, b);
    }

    public void Send(int cmd, byte[] payload) => Link.Inject(Link.Reply(cmd, payload));

    private byte[] Text(int cmd, string s) =>
        Link.Reply((int)Rpt.Text, new[] { (byte)cmd }.Concat(Encoding.ASCII.GetBytes(s)).ToArray());

    private IEnumerable<byte[]> Handle(Packet pkt)
    {
        int cmd = pkt.Cmd;
        byte[] p = pkt.Payload;
        lock (Rx)
            Rx.Add((cmd, p));
        lock (_lock)
        {
            if (_failures.TryGetValue(cmd, out var q) && q.Count > 0)
            {
                int fail = q.Dequeue();
                return fail == Silent ? Array.Empty<byte[]>() : new[] { FakeBoardLink.Status(cmd, (Status)fail) };
            }
            if (cmd == GetDeviceName)
                return new[] { Text(cmd, DeviceName) };
            if (cmd == (int)Cmd.GetNameActiveSoftware)
                return new[] { Text(cmd, SoftwareName) };
            if (cmd == ReadReg)
            {
                int addr = U16(p, 0);
                int n = U16(p, 2);
                int avail = Math.Max(0, Math.Min(n, Regs.Length - addr)); // short past 0xFFFF
                var b = new byte[9 + avail];
                b[0] = (byte)cmd;
                BinaryPrimitives.WriteUInt64LittleEndian(b.AsSpan(1), (ulong)McuTimeUs);
                Array.Copy(Regs, addr, b, 9, avail);
                McuTimeUs += 100;
                return new[] { Link.Reply((int)Vl53lxRpt.RegData, b) };
            }
            if (cmd == WriteReg)
            {
                Array.Copy(p, 2, Regs, U16(p, 0), p.Length - 2);
                return new[] { FakeBoardLink.Status(cmd, Status.Ok) };
            }
            if (cmd == GetInfo)
                return new[] { Link.Reply((int)Vl53lxRpt.Info, Info) };
            if (cmd == ClearI2cErrors && NoClear)
                return new[] { FakeBoardLink.Status(cmd, Status.ErrInvalidCmd) };
            if (cmd == Xshut || cmd == StartStream || cmd == StopStream || cmd == SetI2cSpeed
                || cmd == SetAddrWidth || cmd == ClearI2cErrors)
                return new[] { FakeBoardLink.Status(cmd, Status.Ok) };
            return new[] { FakeBoardLink.Status(cmd, Status.ErrInvalidCmd) };
        }
    }

    public static int U16(byte[] b, int off) => b[off] | b[off + 1] << 8;

    public static byte[] Hex(string s) => Convert.FromHexString(s);

    public static string ToHex(byte[] b) => Convert.ToHexString(b).ToLowerInvariant();
}
