using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Tests;

/// <summary>An in-memory board: every host packet goes to <c>OnPacket</c>, whose reply bytes
/// the device then reads. <c>Inject</c> pushes unsolicited bytes; <c>Unplug</c> makes reads fail.</summary>
internal sealed class FakeBoardLink : ILink
{
    private readonly Queue<byte[]> _rx = new();
    private readonly PacketParser _parser = new();
    private int _seq;
    private bool _closed;
    private bool _unplugged;

    public Func<Packet, IEnumerable<byte[]>>? OnPacket;
    public readonly List<Packet> Sent = new();

    public string Name => "fake";

    public bool IsClosed
    {
        get { lock (_rx) return _closed; }
    }

    public static byte[] Status(int cmd, Status status) =>
        Framing.BuildPacket((int)Rpt.Status, new[] { (byte)cmd, (byte)status });

    public byte[] Reply(int cmd, byte[] payload) => Framing.BuildPacket(cmd, payload, _seq++ & 0xFF);

    public void Inject(byte[] bytes)
    {
        lock (_rx)
        {
            _rx.Enqueue(bytes);
            Monitor.PulseAll(_rx);
        }
    }

    public void Unplug()
    {
        lock (_rx)
        {
            _unplugged = true;
            Monitor.PulseAll(_rx);
        }
    }

    public byte[] Read(TimeSpan timeout)
    {
        lock (_rx)
        {
            if (_rx.Count == 0 && !_closed && !_unplugged)
                Monitor.Wait(_rx, timeout);
            if (_closed)
                throw new LinkClosedException("fake closed");
            if (_unplugged)
                throw new DeviceLostException("fake unplugged");
            return _rx.Count > 0 ? _rx.Dequeue() : Array.Empty<byte>();
        }
    }

    public void Write(byte[] data)
    {
        foreach (var ev in _parser.Feed(data))
        {
            if (ev is not Packet p)
                continue;
            lock (Sent)
                Sent.Add(p);
            foreach (var r in OnPacket?.Invoke(p) ?? Array.Empty<byte[]>())
                Inject(r);
        }
    }

    public void Close()
    {
        lock (_rx)
        {
            _closed = true;
            Monitor.PulseAll(_rx);
        }
    }
}
