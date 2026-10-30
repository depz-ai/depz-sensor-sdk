using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Tests;
using Depz.Sensor.Unity;
using Xunit;

namespace Depz.Sensor.Unity.Tests;

// UnityEngine.Debug.Lines is one static list: these tests run one after the other.
[Collection(nameof(DepzSensorBehaviourTests))]
public class DepzSensorBehaviourTests
{
    private sealed class Probe : DepzSensorBehaviour
    {
        public readonly FakeBoardLink Link = new();
        public readonly DepzDevice Opened;
        public Exception? ThrowInOnOpened;
        public int OnOpenedCalls;

        public Probe() => Opened = new DepzDevice(Link, TimeSpan.FromMilliseconds(100));

        protected override DepzDevice OpenDevice() => Opened;

        protected override void OnOpened(DepzDevice device)
        {
            OnOpenedCalls++;
            if (ThrowInOnOpened != null)
                throw ThrowInOnOpened;
        }

        public void RunStart() => Start();
    }

    // CSHARP-8: a request after the open that fails (here GET_NAME_ACTIVE_SOFTWARE gets no
    // answer) closes the device and logs; the component is not left half-started.
    [Fact]
    public void A_failing_request_after_the_open_closes_the_device_and_logs()
    {
        UnityEngine.Debug.Lines.Clear();
        var probe = new Probe();
        probe.RunStart();
        Assert.Null(probe.Device);
        Assert.True(probe.Opened.IsClosed);
        Assert.Equal(0, probe.OnOpenedCalls);
        Assert.Contains(UnityEngine.Debug.Lines, l => l.StartsWith("error: DEPZ:", StringComparison.Ordinal));
    }

    [Fact]
    public void An_exception_from_OnOpened_closes_the_device_and_logs()
    {
        UnityEngine.Debug.Lines.Clear();
        var probe = new Probe { ThrowInOnOpened = new InvalidOperationException("subclass bug") };
        probe.Link.OnPacket = p => new[]
        {
            probe.Link.Reply((int)Rpt.Text, new[] { (byte)p.Cmd }.Concat("APP_usonic_SR04_v0.97"u8.ToArray()).Append((byte)0).ToArray()),
        };
        probe.RunStart();
        Assert.Equal(1, probe.OnOpenedCalls);
        Assert.Null(probe.Device);
        Assert.True(probe.Opened.IsClosed);
        Assert.Contains("exception: subclass bug", UnityEngine.Debug.Lines);
    }
}
