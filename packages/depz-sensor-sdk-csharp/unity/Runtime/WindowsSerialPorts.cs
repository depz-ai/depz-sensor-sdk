#nullable enable
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Depz.Sensor.Devices;

namespace Depz.Sensor.Unity
{
    /// <summary>
    /// Windows serial-port enumeration from the registry, with USB ids. The
    /// ports present now are the values of <c>HKLM\HARDWARE\DEVICEMAP\SERIALCOMM</c>;
    /// which USB device owns which port is under
    /// <c>HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_xxxx&amp;PID_xxxx[&amp;MI_nn]\&lt;instance&gt;\Device Parameters\PortName</c>,
    /// where a plain device's instance id is its USB serial (a device without one
    /// gets a generated id with <c>&amp;</c> in it: no serial). Pure reads over
    /// <see cref="IRegistry"/>, so it is testable without Windows.
    /// </summary>
    public static class WindowsSerialPorts
    {
        /// <summary>The key whose values are the serial ports present now (name: driver device, data: <c>COMn</c>).</summary>
        public const string SerialCommKey = @"HARDWARE\DEVICEMAP\SERIALCOMM";

        /// <summary>The USB device tree: <c>VID_xxxx&amp;PID_xxxx[&amp;MI_nn]\&lt;instance&gt;</c> keys.</summary>
        public const string UsbEnumKey = @"SYSTEM\CurrentControlSet\Enum\USB";

        /// <summary>Every COM port present now, with the USB ids of the device that owns it where the
        /// registry says so (null ids for a non-USB port, or one several devices claim). Sorted by port name.</summary>
        public static IReadOnlyList<SerialPortInfo> List(IRegistry registry)
        {
            if (registry is null)
                throw new ArgumentNullException(nameof(registry));
            HashSet<string> present = PresentPorts(registry);
            var claims = new Dictionary<string, List<Claim>>(StringComparer.OrdinalIgnoreCase);
            foreach (string hw in registry.SubKeyNames(UsbEnumKey) ?? Array.Empty<string>())
            {
                int? vid = HwField(hw, "VID_"), pid = HwField(hw, "PID_");
                if (vid is null || pid is null)
                    continue; // ROOT_HUB and the like
                int mi = hw.IndexOf("&MI_", StringComparison.OrdinalIgnoreCase);
                string hwPath = UsbEnumKey + "\\" + hw;
                foreach (string inst in registry.SubKeyNames(hwPath) ?? Array.Empty<string>())
                {
                    string instPath = hwPath + "\\" + inst;
                    string? port = registry.GetString(instPath + "\\Device Parameters", "PortName");
                    // No PortName: not a serial device. A port not present now: a stale entry, the
                    // port this device had the last time it was plugged in (Windows keeps them all).
                    if (port is null || !present.Contains(port))
                        continue;
                    // The volatile Control subkey exists only while the device is attached.
                    bool attached = registry.SubKeyNames(instPath + "\\Control") != null;
                    string? serial = UsbSerial(mi < 0 ? inst : ParentSerial(registry, hw.Substring(0, mi), inst));
                    if (!claims.TryGetValue(port, out List<Claim> list))
                        claims[port] = list = new List<Claim>();
                    list.Add(new Claim(vid.Value, pid.Value, serial, attached));
                }
            }
            var result = new List<SerialPortInfo>();
            foreach (string port in present.OrderBy(p => p, StringComparer.Ordinal))
                result.Add(claims.TryGetValue(port, out List<Claim> list)
                    ? Resolve(port, list)
                    : new SerialPortInfo(port, null, null, null));
            return result;
        }

        private static HashSet<string> PresentPorts(IRegistry registry)
        {
            var set = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string name in registry.ValueNames(SerialCommKey) ?? Array.Empty<string>())
            {
                string? port = registry.GetString(SerialCommKey, name);
                if (!string.IsNullOrEmpty(port))
                    set.Add(port!);
            }
            return set;
        }

        // Several entries can name one port: Windows reuses COM numbers, and an entry outlives its
        // device being unplugged. The attached entries win; a single survivor gives its ids, several
        // of one model give the model without a serial, several models give nothing.
        private static SerialPortInfo Resolve(string port, List<Claim> claims)
        {
            List<Claim> live = claims.Where(c => c.Attached).ToList();
            if (live.Count == 0)
                live = claims;
            if (live.Count == 1)
                return new SerialPortInfo(port, live[0].Vid, live[0].Pid, live[0].Serial);
            if (!live.All(c => c.Vid == live[0].Vid && c.Pid == live[0].Pid))
                return new SerialPortInfo(port, null, null, null);
            bool oneSerial = live.All(c => c.Serial == live[0].Serial);
            return new SerialPortInfo(port, live[0].Vid, live[0].Pid, oneSerial ? live[0].Serial : null);
        }

        // An interface of a composite device (…&MI_nn) carries a generated instance id
        // ("7&2a1b3c4d&0&0000"); the USB serial is the instance id of the parent device, whose
        // ParentIdPrefix ("7&2a1b3c4d&0") the interface id starts with.
        private static string? ParentSerial(IRegistry registry, string parentHw, string inst)
        {
            string parentPath = UsbEnumKey + "\\" + parentHw;
            IReadOnlyList<string> parents = registry.SubKeyNames(parentPath) ?? Array.Empty<string>();
            foreach (string parent in parents)
            {
                string? prefix = registry.GetString(parentPath + "\\" + parent, "ParentIdPrefix");
                if (!string.IsNullOrEmpty(prefix) && inst.StartsWith(prefix + "&", StringComparison.OrdinalIgnoreCase))
                    return parent;
            }
            return parents.Count == 1 ? parents[0] : null;
        }

        // Windows generates the instance id of a device without a USB serial ("5&2d4e6f8a&0&1");
        // '&' never comes from a device, whose serial Windows copies as is.
        private static string? UsbSerial(string? inst) =>
            string.IsNullOrEmpty(inst) || inst!.Contains('&') ? null : inst;

        private static int? HwField(string hw, string tag)
        {
            int i = hw.IndexOf(tag, StringComparison.OrdinalIgnoreCase);
            if (i < 0 || hw.Length < i + tag.Length + 4)
                return null;
            return int.TryParse(hw.AsSpan(i + tag.Length, 4), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out int v)
                ? v
                : (int?)null;
        }

        private sealed class Claim
        {
            public readonly int Vid;
            public readonly int Pid;
            public readonly string? Serial;
            public readonly bool Attached;

            public Claim(int vid, int pid, string? serial, bool attached)
            {
                Vid = vid;
                Pid = pid;
                Serial = serial;
                Attached = attached;
            }
        }
    }
}
