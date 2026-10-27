using Depz.Sensor.Devices;
using Depz.Sensor.Unity;
using Depz.Sensor.Usb;
using Xunit;

namespace Depz.Sensor.Unity.Tests;

public class WindowsSerialPortsTests
{
    private const string Sr04 = "VID_1BCF&PID_EC78";
    private const string L8cx = "VID_1BCF&PID_ED4B";

    [Fact]
    public void Lists_the_present_ports_with_their_usb_ids()
    {
        var reg = new FakeRegistry().Present("COM3", "COM1").UsbPort(Sr04, "SN000042", "COM3");
        var ports = WindowsSerialPorts.List(reg);
        Assert.Equal(new[] { new SerialPortInfo("COM1", null, null, null), new SerialPortInfo("COM3", 0x1BCF, 0xEC78, "SN000042") }, ports);
    }

    [Fact]
    public void A_stale_entry_whose_port_is_absent_is_ignored()
    {
        // The board was COM7 last time; it is unplugged now (COM7 not in SERIALCOMM).
        var reg = new FakeRegistry().Present("COM1").UsbPort(Sr04, "SN000042", "COM7", attached: false);
        Assert.Equal(new[] { new SerialPortInfo("COM1", null, null, null) }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void An_instance_without_PortName_is_not_a_port()
    {
        var reg = new FakeRegistry().Present("COM3").Key($@"{WindowsSerialPorts.UsbEnumKey}\{Sr04}\SN000042\Device Parameters");
        Assert.Equal(new[] { new SerialPortInfo("COM3", null, null, null) }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void Duplicate_claims_resolve_to_the_attached_device()
    {
        // Windows gave COM5 to an old board once; a new board holds it now.
        var reg = new FakeRegistry().Present("COM5")
            .UsbPort(Sr04, "SN_OLD", "COM5", attached: false)
            .UsbPort(L8cx, "SN_NEW", "COM5", attached: true);
        Assert.Equal(new[] { new SerialPortInfo("COM5", 0x1BCF, 0xED4B, "SN_NEW") }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void Duplicate_claims_of_one_model_keep_the_model_and_drop_the_serial()
    {
        var reg = new FakeRegistry().Present("COM5")
            .UsbPort(Sr04, "SN000001", "COM5", attached: false)
            .UsbPort(Sr04, "SN000002", "COM5", attached: false);
        Assert.Equal(new[] { new SerialPortInfo("COM5", 0x1BCF, 0xEC78, null) }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void Duplicate_claims_of_different_models_give_no_ids()
    {
        var reg = new FakeRegistry().Present("COM5")
            .UsbPort(Sr04, "SN000001", "COM5", attached: false)
            .UsbPort(L8cx, "SN000002", "COM5", attached: false);
        Assert.Equal(new[] { new SerialPortInfo("COM5", null, null, null) }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void A_composite_interface_takes_the_serial_of_its_parent()
    {
        var reg = new FakeRegistry().Present("COM9", "COM11")
            .UsbParent(L8cx, "SN000001", "7&2a1b3c4d&0")
            .UsbParent(L8cx, "SN000002", "7&5e6f7a8b&0")
            .UsbPort(L8cx + "&MI_00", "7&2a1b3c4d&0&0000", "COM9")
            .UsbPort(L8cx + "&MI_00", "7&5e6f7a8b&0&0000", "COM11");
        Assert.Equal(
            new[] { new SerialPortInfo("COM11", 0x1BCF, 0xED4B, "SN000002"), new SerialPortInfo("COM9", 0x1BCF, 0xED4B, "SN000001") },
            WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void A_composite_interface_falls_back_to_the_only_parent_without_a_prefix()
    {
        var reg = new FakeRegistry().Present("COM9")
            .Key($@"{WindowsSerialPorts.UsbEnumKey}\{L8cx}\SN000001")
            .UsbPort(L8cx + "&MI_00", "7&2a1b3c4d&0&0000", "COM9");
        Assert.Equal(new[] { new SerialPortInfo("COM9", 0x1BCF, 0xED4B, "SN000001") }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void A_composite_interface_with_ambiguous_parents_has_no_serial()
    {
        var reg = new FakeRegistry().Present("COM9")
            .Key($@"{WindowsSerialPorts.UsbEnumKey}\{L8cx}\SN000001")
            .Key($@"{WindowsSerialPorts.UsbEnumKey}\{L8cx}\SN000002")
            .UsbPort(L8cx + "&MI_00", "7&2a1b3c4d&0&0000", "COM9");
        Assert.Equal(new[] { new SerialPortInfo("COM9", 0x1BCF, 0xED4B, null) }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void A_locked_usb_tree_lists_the_ports_without_ids()
    {
        var reg = new FakeRegistry().Present("COM3").UsbPort(Sr04, "SN000042", "COM3").Lock(WindowsSerialPorts.UsbEnumKey);
        Assert.Equal(new[] { new SerialPortInfo("COM3", null, null, null) }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void A_locked_device_instance_is_skipped_and_the_rest_listed()
    {
        var reg = new FakeRegistry().Present("COM3", "COM4")
            .UsbPort(Sr04, "SN000042", "COM3").UsbPort(Sr04, "SN000043", "COM4")
            .Lock($@"{WindowsSerialPorts.UsbEnumKey}\{Sr04}\SN000042\Device Parameters");
        Assert.Equal(
            new[] { new SerialPortInfo("COM3", null, null, null), new SerialPortInfo("COM4", 0x1BCF, 0xEC78, "SN000043") },
            WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void A_locked_or_missing_SERIALCOMM_lists_nothing()
    {
        Assert.Empty(WindowsSerialPorts.List(new FakeRegistry()));
        Assert.Empty(WindowsSerialPorts.List(new FakeRegistry().Present("COM3").Lock(WindowsSerialPorts.SerialCommKey)));
    }

    [Fact]
    public void Non_usb_enum_entries_are_ignored()
    {
        var reg = new FakeRegistry().Present("COM3")
            .Key($@"{WindowsSerialPorts.UsbEnumKey}\ROOT_HUB30\4&1a2b3c&0\Device Parameters", ("PortName", "COM3"));
        Assert.Equal(new[] { new SerialPortInfo("COM3", null, null, null) }, WindowsSerialPorts.List(reg));
    }

    [Fact]
    public void Depz_candidates_are_filtered_by_usb_id_and_sorted_by_serial_then_port()
    {
        var reg = new FakeRegistry().Present("COM3", "COM10", "COM4", "COM6", "COM7", "COM1")
            .UsbPort(Sr04, "SN000042", "COM3")
            .UsbPort(L8cx, "SN000041", "COM10")
            .UsbPort(L8cx, "SN000042", "COM7")                                   // same serial: port breaks the tie
            .UsbPort("VID_0483&PID_56DC", "5&1234&0&1", "COM4", attached: true)  // dev default, no USB serial: Windows-generated id
            .UsbPort("VID_067B&PID_2303", "SN000001", "COM6");                   // not DEPZ
        var candidates = DepzDevicesUnity.ListDepzPorts(reg);
        Assert.Equal(new[] { "COM10", "COM3", "COM7", "COM4" }, candidates.Select(p => p.Port));
        Assert.Equal(new SerialPortInfo("COM4", 0x0483, 0x56DC, null), candidates[3]);
        Assert.True(candidates.All(p => UsbIds.IsKnownDepzUsb(p.Vid, p.Pid)));
        Assert.Equal(6, DepzDevicesUnity.ListPorts(reg).Count);
    }
}
