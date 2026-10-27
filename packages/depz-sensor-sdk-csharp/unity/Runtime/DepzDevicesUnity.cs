#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using Depz.Sensor.Devices;
using Depz.Sensor.Usb;

namespace Depz.Sensor.Unity
{
    /// <summary>
    /// <see cref="DepzDevices"/> for Unity on Windows: the same discovery and
    /// open calls over <see cref="WindowsSerialPorts"/> (the registry) and
    /// <see cref="WindowsSerialLink"/>. Selection follows contract 02 §4.3 (only
    /// ports with a known DEPZ USB id, ordered by USB serial) and the class comes
    /// from the probe, run by <see cref="DepzDevices.OpenOnLink"/> on the one
    /// connection the device then keeps. Every discovering call takes an optional
    /// <see cref="IRegistry"/> (default: the Windows registry), so it is testable
    /// with a fake. Off Windows there is no registry: discovery finds nothing and
    /// <see cref="OpenPort"/> is the way in.
    /// </summary>
    public static class DepzDevicesUnity
    {
        private static readonly IRegistry Windows = new WindowsRegistry();

        /// <summary>Every serial port present, with USB ids where the registry gives them.</summary>
        public static IReadOnlyList<SerialPortInfo> ListPorts(IRegistry? registry = null) =>
            WindowsSerialPorts.List(registry ?? Windows);

        /// <summary>The ports with a known DEPZ USB id, sorted by USB serial (none last), then by port
        /// (contract 02 §4.3): the discovery candidates, index 0 is what <see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/> opens.</summary>
        public static IReadOnlyList<SerialPortInfo> ListDepzPorts(IRegistry? registry = null) =>
            SerialPorts.BySerial(ListPorts(registry).Where(p => UsbIds.IsKnownDepzUsb(p.Vid, p.Pid))).ToList();

        /// <summary>Probe one port: open it, run the contract 02 §4.4 probe, close. Null when the port cannot
        /// be opened or nothing DEPZ-shaped answers.</summary>
        public static DeviceInfo? ProbePort(string port, TimeSpan? timeout = null)
        {
            WindowsSerialLink link;
            try
            {
                link = new WindowsSerialLink(port);
            }
            catch (DepzException)
            {
                return null;
            }
            return DepzDevices.ProbeLink(link, timeout);
        }

        /// <summary>Probe the candidate ports and return every DEPZ device found, ordered by USB serial.
        /// With <paramref name="matchUsb"/> (default) only ports with a known DEPZ USB id are probed;
        /// <paramref name="ports"/> replaces enumeration with an explicit list.</summary>
        public static IReadOnlyList<DeviceInfo> ListDepzDevices(
            IEnumerable<string>? ports = null, bool matchUsb = true, TimeSpan? timeout = null, IRegistry? registry = null)
        {
            var enumerated = ListPorts(registry).ToDictionary(p => p.Port, StringComparer.OrdinalIgnoreCase);
            IEnumerable<SerialPortInfo> candidates = ports is null
                ? enumerated.Values
                : ports.Select(p => enumerated.TryGetValue(p, out SerialPortInfo pi) ? pi : new SerialPortInfo(p, null, null, null));
            if (matchUsb)
                candidates = candidates.Where(p => UsbIds.IsKnownDepzUsb(p.Vid, p.Pid));
            var found = new List<DeviceInfo>();
            foreach (SerialPortInfo pi in SerialPorts.BySerial(candidates))
            {
                DeviceInfo? info = ProbePort(pi.Port, timeout);
                // Not a `with`: the init setters' modreq differs between Depz.Sensor's two builds.
                if (info != null)
                    found.Add(new DeviceInfo(info.Port, info.Mode, info.SensorType, info.SoftwareName, info.FwVersion,
                        info.DeviceName, info.SerialNumber, pi.Vid, pi.Pid, pi.UsbSerial));
            }
            return found;
        }

        /// <summary>Open the DEPZ device with the smallest USB serial, or the one whose USB serial is
        /// <paramref name="serial"/>, as its sensor class (contract 02 §4.3).</summary>
        /// <exception cref="NoDepzDeviceException">No candidate port.</exception>
        /// <exception cref="DeviceLostException">The candidate's port cannot be opened.</exception>
        public static DepzDevice OpenDevice(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null)
        {
            IReadOnlyList<SerialPortInfo> candidates = ListDepzPorts(registry);
            if (serial != null)
            {
                SerialPortInfo match = candidates.FirstOrDefault(p => p.UsbSerial == serial)
                    ?? throw new NoDepzDeviceException($"no DEPZ device with USB serial '{serial}'");
                return Open(match, timeout);
            }
            if (candidates.Count == 0)
                throw new NoDepzDeviceException(NoCandidate(registry));
            return Open(candidates[0], timeout);
        }

        /// <summary>Open the <paramref name="index"/>-th DEPZ candidate, candidates sorted by USB serial.</summary>
        public static DepzDevice OpenDevice(int index, TimeSpan? timeout = null, IRegistry? registry = null)
        {
            IReadOnlyList<SerialPortInfo> candidates = ListDepzPorts(registry);
            if (index < 0 || index >= candidates.Count)
                throw new NoDepzDeviceException($"no DEPZ candidate at index {index} ({candidates.Count} candidate(s) found)");
            return Open(candidates[index], timeout);
        }

        /// <summary>Open exactly <paramref name="port"/> as its sensor class (also a port whose USB id is not
        /// a DEPZ one: an unprogrammed unit). The registry supplies the port's USB id, where it has one.</summary>
        public static DepzDevice OpenPort(string port, TimeSpan? timeout = null, IRegistry? registry = null)
        {
            if (string.IsNullOrEmpty(port))
                throw new ArgumentException("port name is empty", nameof(port));
            SerialPortInfo pi = ListPorts(registry).FirstOrDefault(p => string.Equals(p.Port, port, StringComparison.OrdinalIgnoreCase))
                ?? new SerialPortInfo(port, null, null, null);
            return Open(pi, timeout);
        }

        /// <summary>Open the port of a <see cref="ListDepzDevices"/> result as the class its probe answers resolve to.</summary>
        public static DepzDevice OpenDevice(DeviceInfo info, TimeSpan? timeout = null)
        {
            if (info is null)
                throw new ArgumentNullException(nameof(info));
            return Open(new SerialPortInfo(info.Port, info.UsbVid, info.UsbPid, info.UsbSerial), timeout);
        }

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is an SR04.</summary>
        public static Sr04Device OpenSr04(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Sr04Device>(OpenDevice(serial, timeout, registry), "an SR04");

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is a VL53L4CD.</summary>
        public static Vl53l4cdDevice OpenVl53l4cd(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Vl53l4cdDevice>(OpenDevice(serial, timeout, registry), "a VL53L4CD");

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is a
        /// VL53L8CX or VL53L8CH (<see cref="Vl53l8chDevice"/> derives from <see cref="Vl53l8cxDevice"/>).</summary>
        public static Vl53l8cxDevice OpenVl53l8(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Vl53l8cxDevice>(OpenDevice(serial, timeout, registry), "a VL53L8CX / VL53L8CH");

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is a
        /// VL53L5CX, VL53L7CX or VL53L7CH (all derive from <see cref="Vl53l7cxDevice"/>).</summary>
        public static Vl53l7cxDevice OpenVl53l7(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Vl53l7cxDevice>(OpenDevice(serial, timeout, registry), "a VL53L5CX / VL53L7CX / VL53L7CH");

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is a BNO055.</summary>
        public static Bno055Device OpenBno055(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Bno055Device>(OpenDevice(serial, timeout, registry), "a BNO055");

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is a BNO085 / BNO086.</summary>
        public static Bno086Device OpenBno086(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Bno086Device>(OpenDevice(serial, timeout, registry), "a BNO085 / BNO086");

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is a
        /// board of the VL53L 1D family (every class derives from <see cref="Vl53lxDevice"/>).</summary>
        public static Vl53lxDevice OpenVl53lx(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Vl53lxDevice>(OpenDevice(serial, timeout, registry), "a VL53L 1D-family board");

        /// <summary><see cref="OpenDevice(string?, TimeSpan?, IRegistry?)"/>, and <see cref="WrongTypeException"/> unless it is a TMF8829.</summary>
        public static Tmf8829Device OpenTmf8829(string? serial = null, TimeSpan? timeout = null, IRegistry? registry = null) =>
            DepzDevices.Expect<Tmf8829Device>(OpenDevice(serial, timeout, registry), "a TMF8829");

        // One connection: the probe runs on it and the class continues on it (contract 02 §4.4).
        private static DepzDevice Open(SerialPortInfo pi, TimeSpan? timeout) =>
            DepzDevices.OpenOnLink(new WindowsSerialLink(pi.Port), pi.Vid, pi.Pid, timeout);

        private static string NoCandidate(IRegistry? registry) =>
            registry != null || WindowsRegistry.IsAvailable
                ? "no DEPZ device found (no serial port has a known DEPZ USB id). "
                  + "Pass a port name to OpenPort to open an unprogrammed unit."
                : $"port discovery of the Unity module needs Windows (this is {Environment.OSVersion.Platform}); "
                  + "open the port by name with OpenPort.";
    }
}
