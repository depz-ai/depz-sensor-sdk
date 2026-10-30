using Depz.Sensor.Devices;
using Depz.Sensor.Tests;
using Depz.Sensor.Unity;
using Depz.Sensor.Usb;
using Xunit;

namespace Depz.Sensor.Unity.Tests;

public class DepzDevicesUnityTests
{
    // No OS has a port of this name: Windows' COM space stops far below, POSIX has no such path.
    private const string Missing = "COM4095";
    private const string Sr04Hw = "VID_1BCF&PID_EC78";
    private const string L8cxHw = "VID_1BCF&PID_ED4B";

    [Fact]
    public void A_missing_port_is_a_DeviceLostException_from_the_link()
    {
        var e = Assert.Throws<DeviceLostException>(() => new WindowsSerialLink(Missing));
        Assert.Contains(Missing, e.Message);
        Assert.NotNull(e.InnerException);
        Assert.Throws<DeviceLostException>(() => new WindowsSerialLink("/dev/ttyDEPZ-nonexistent"));
        Assert.Throws<ArgumentException>(() => new WindowsSerialLink(""));
    }

    [Fact]
    public void ProbePort_on_a_missing_port_is_null()
    {
        Assert.Null(DepzDevicesUnity.ProbePort(Missing));
    }

    [Fact]
    public void OpenPort_on_a_missing_port_is_a_DeviceLostException()
    {
        var e = Assert.Throws<DeviceLostException>(() => DepzDevicesUnity.OpenPort(Missing, registry: new FakeRegistry()));
        Assert.Contains(Missing, e.Message);
        Assert.Throws<ArgumentException>(() => DepzDevicesUnity.OpenPort("", registry: new FakeRegistry()));
    }

    [Fact]
    public void OpenDevice_on_a_candidate_whose_port_is_gone_is_a_DeviceLostException()
    {
        var reg = new FakeRegistry().Present(Missing).UsbPort("VID_1BCF&PID_EC78", "SN000042", Missing);
        Assert.Throws<DeviceLostException>(() => DepzDevicesUnity.OpenDevice(registry: reg));
        Assert.Throws<DeviceLostException>(() => DepzDevicesUnity.OpenDevice("SN000042", registry: reg));
        Assert.Throws<DeviceLostException>(() => DepzDevicesUnity.OpenDevice(0, registry: reg));
        Assert.Throws<DeviceLostException>(() => DepzDevicesUnity.OpenSr04(registry: reg));
        Assert.Throws<DeviceLostException>(() => DepzDevicesUnity.OpenDevice(
            new DeviceInfo(Missing, "app", null, "APP_usonic_SR04", "0.97", "", "", 0x1BCF, 0xEC78, "SN000042")));
    }

    [Fact]
    public void ListDepzDevices_skips_candidates_that_do_not_open()
    {
        var reg = new FakeRegistry().Present(Missing).UsbPort("VID_1BCF&PID_EC78", "SN000042", Missing);
        Assert.Empty(DepzDevicesUnity.ListDepzDevices(registry: reg));
        Assert.Empty(DepzDevicesUnity.ListDepzDevices(ports: new[] { Missing }, matchUsb: false, registry: reg));
        Assert.Empty(DepzDevicesUnity.ListDepzDevices(registry: new FakeRegistry()));
    }

    [Fact]
    public void No_candidate_is_a_NoDepzDeviceException()
    {
        var empty = new FakeRegistry();
        var e = Assert.Throws<NoDepzDeviceException>(() => DepzDevicesUnity.OpenDevice(registry: empty));
        Assert.Contains("OpenPort", e.Message);
        Assert.Throws<NoDepzDeviceException>(() => DepzDevicesUnity.OpenDevice(0, registry: empty));
        Assert.Throws<NoDepzDeviceException>(() => DepzDevicesUnity.OpenVl53l8(registry: empty));
        // a DEPZ board is there, but not the serial asked for
        var reg = new FakeRegistry().Present("COM3").UsbPort("VID_1BCF&PID_EC78", "SN000042", "COM3");
        Assert.Throws<NoDepzDeviceException>(() => DepzDevicesUnity.OpenDevice("SN000099", registry: reg));
        Assert.Throws<NoDepzDeviceException>(() => DepzDevicesUnity.OpenDevice(1, registry: reg));
        // a port with a non-DEPZ id is no candidate
        var foreign = new FakeRegistry().Present("COM6").UsbPort("VID_067B&PID_2303", "X", "COM6");
        Assert.Throws<NoDepzDeviceException>(() => DepzDevicesUnity.OpenDevice(registry: foreign));
    }

    [Fact]
    public void The_real_registry_reads_nothing_off_windows()
    {
        if (OperatingSystem.IsWindows())
            return;
        Assert.False(WindowsRegistry.IsAvailable);
        var reg = new WindowsRegistry();
        Assert.Null(reg.SubKeyNames(WindowsSerialPorts.UsbEnumKey));
        Assert.Null(reg.ValueNames(WindowsSerialPorts.SerialCommKey));
        Assert.Null(reg.GetString(WindowsSerialPorts.SerialCommKey, @"\Device\Serial0"));
        Assert.Empty(DepzDevicesUnity.ListPorts());
        var e = Assert.Throws<NoDepzDeviceException>(() => DepzDevicesUnity.OpenDevice());
        Assert.Contains("Windows", e.Message);
    }

    [Fact]
    public void Depz_candidates_are_filtered_by_usb_id_and_sorted_by_serial_then_port()
    {
        var reg = new FakeRegistry().Present("COM3", "COM10", "COM4", "COM6", "COM7", "COM1")
            .UsbPort(Sr04Hw, "SN000042", "COM3")
            .UsbPort(L8cxHw, "SN000041", "COM10")
            .UsbPort(L8cxHw, "SN000042", "COM7")                                 // same serial: port breaks the tie
            .UsbPort("VID_0483&PID_56DC", "5&1234&0&1", "COM4", attached: true)  // dev default, no USB serial: Windows-generated id
            .UsbPort("VID_067B&PID_2303", "SN000001", "COM6");                   // not DEPZ
        var candidates = DepzDevicesUnity.ListDepzPorts(reg);
        Assert.Equal(new[] { "COM10", "COM3", "COM7", "COM4" }, candidates.Select(p => p.Port));
        Assert.Equal(new SerialPortInfo("COM4", 0x0483, 0x56DC, null), candidates[3]);
        Assert.True(candidates.All(p => UsbIds.IsKnownDepzUsb(p.Vid, p.Pid)));
        Assert.Equal(6, DepzDevicesUnity.ListPorts(reg).Count);
    }
}
