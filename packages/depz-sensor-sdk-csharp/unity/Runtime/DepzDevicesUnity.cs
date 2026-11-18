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
    /// open calls over the Windows registry (COM ports with their USB ids) and
    /// <see cref="WindowsSerialLink"/>. Selection follows contract 02 §4.3 (only
    /// ports with a known DEPZ USB id, ordered by USB serial) and the class comes
    /// from the probe, run by <see cref="DepzDevices.OpenOnLink"/> on the one
    /// connection the device then keeps. Every open call returns the class the
    /// probe resolves; cast to the one you expect. Off Windows there is no
    /// registry: discovery finds nothing and <see cref="OpenPort(string, TimeSpan?)"/> is the way in.
    /// </summary>
    public static class DepzDevicesUnity
    {
        private static readonly IRegistry Windows = new WindowsRegistry();

        /// <summary>Every serial port present, with USB ids where the registry gives them.</summary>
        public static IReadOnlyList<SerialPortInfo> ListPorts() => ListPorts(Windows);

        // The discovering calls over a given registry: the tests run them over a fake.
        internal static IReadOnlyList<SerialPortInfo> ListPorts(IRegistry registry) => WindowsSerialPorts.List(registry);

        /// <summary>The ports with a known DEPZ USB id, sorted by USB serial (none last), then by port
        /// (contract 02 §4.3): the discovery candidates, index 0 is what <see cref="OpenDevice(int, TimeSpan?)"/> opens.</summary>
        public static IReadOnlyList<SerialPortInfo> ListDepzPorts() => ListDepzPorts(Windows);

        internal static IReadOnlyList<SerialPortInfo> ListDepzPorts(IRegistry registry) =>
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
            IEnumerable<string>? ports = null, bool matchUsb = true, TimeSpan? timeout = null) =>
            ListDepzDevices(Windows, ports, matchUsb, timeout);

        internal static IReadOnlyList<DeviceInfo> ListDepzDevices(
            IRegistry registry, IEnumerable<string>? ports = null, bool matchUsb = true, TimeSpan? timeout = null)
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

        /// <summary>Open the <paramref name="index"/>-th DEPZ candidate as its sensor class, candidates
        /// sorted by USB serial (contract 02 §4.3); the default is the smallest USB serial.</summary>
        /// <exception cref="NoDepzDeviceException">No candidate port, none at <paramref name="index"/>, or its port
        /// cannot be opened (the message says so, <see cref="Exception.InnerException"/> says why; contract 07 §3).</exception>
        public static DepzDevice OpenDevice(int index = 0, TimeSpan? timeout = null) => OpenDevice(Windows, index, timeout);

        internal static DepzDevice OpenDevice(IRegistry registry, int index = 0, TimeSpan? timeout = null)
        {
            IReadOnlyList<SerialPortInfo> candidates = ListDepzPorts(registry);
            if (candidates.Count == 0)
                throw new NoDepzDeviceException(NoCandidate(registry));
            if (index < 0 || index >= candidates.Count)
                throw new NoDepzDeviceException($"no DEPZ candidate at index {index} ({candidates.Count} candidate(s) found)");
            return Open(candidates[index], timeout);
        }

        /// <summary>Open the DEPZ device whose USB serial is <paramref name="serial"/> as its sensor class
        /// (contract 02 §4.3).</summary>
        /// <exception cref="NoDepzDeviceException">No candidate port has that USB serial, or its port cannot be
        /// opened (the message says so, <see cref="Exception.InnerException"/> says why; contract 07 §3).</exception>
        public static DepzDevice OpenDeviceBySerial(string serial, TimeSpan? timeout = null) =>
            OpenDeviceBySerial(Windows, serial, timeout);

        internal static DepzDevice OpenDeviceBySerial(IRegistry registry, string serial, TimeSpan? timeout = null)
        {
            if (serial is null)
                throw new ArgumentNullException(nameof(serial));
            SerialPortInfo match = ListDepzPorts(registry).FirstOrDefault(p => p.UsbSerial == serial)
                ?? throw new NoDepzDeviceException($"no DEPZ device with USB serial '{serial}'");
            return Open(match, timeout);
        }

        /// <summary>Open exactly <paramref name="port"/> as its sensor class (also a port whose USB id is not
        /// a DEPZ one: an unprogrammed unit). The registry supplies the port's USB id, where it has one.</summary>
        /// <exception cref="NoDepzDeviceException">Nothing DEPZ answers, or the port cannot be opened (the message says
        /// so, <see cref="Exception.InnerException"/> says why; contract 07 §3).</exception>
        public static DepzDevice OpenPort(string port, TimeSpan? timeout = null) => OpenPort(Windows, port, timeout);

        internal static DepzDevice OpenPort(IRegistry registry, string port, TimeSpan? timeout = null)
        {
            if (string.IsNullOrEmpty(port))
                throw new ArgumentException("port name is empty", nameof(port));
            SerialPortInfo pi = ListPorts(registry).FirstOrDefault(p => string.Equals(p.Port, port, StringComparison.OrdinalIgnoreCase))
                ?? new SerialPortInfo(port, null, null, null);
            return Open(pi, timeout);
        }

        /// <summary>Open the port of a <see cref="ListDepzDevices(IEnumerable{string}?, bool, TimeSpan?)"/> result as the class its probe answers resolve to.</summary>
        public static DepzDevice OpenDevice(DeviceInfo info, TimeSpan? timeout = null)
        {
            if (info is null)
                throw new ArgumentNullException(nameof(info));
            // As DepzDevices.OpenDevice(DeviceInfo) and the reference: the port was found, an open that fails is its loss.
            return DepzDevices.OpenOnLink(new WindowsSerialLink(info.Port), info.UsbVid, info.UsbPid, timeout);
        }

        // One connection: the probe runs on it and the class continues on it (contract 02 §4.4).
        private static DepzDevice Open(SerialPortInfo pi, TimeSpan? timeout)
        {
            WindowsSerialLink link;
            try
            {
                link = new WindowsSerialLink(pi.Port);
            }
            catch (DeviceLostException e)
            {
                throw new NoDepzDeviceException(
                    $"no DEPZ device answered on {pi.Port}: could not open the port ({e.Message})", e);
            }
            return DepzDevices.OpenOnLink(link, pi.Vid, pi.Pid, timeout);
        }

        private static string NoCandidate(IRegistry registry) =>
            !ReferenceEquals(registry, Windows) || WindowsRegistry.IsAvailable
                ? "no DEPZ device found (no serial port has a known DEPZ USB id). "
                  + "Pass a port name to OpenPort to open an unprogrammed unit."
                : $"port discovery of the Unity module needs Windows (this is {Environment.OSVersion.Platform}); "
                  + "open the port by name with OpenPort.";
    }
}
