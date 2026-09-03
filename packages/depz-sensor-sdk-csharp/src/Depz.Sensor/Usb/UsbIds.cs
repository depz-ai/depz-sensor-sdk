namespace Depz.Sensor.Usb;

/// <summary>
/// DEPZ USB identity table (contracts/02_COMMON_COMMANDS.md §4).
///
/// Used by discovery to pick the right serial port without poking unrelated
/// devices; the protocol probe (GET_NAME_ACTIVE_SOFTWARE) remains the source of
/// truth for what a device actually is. The PID→model map is an informational
/// hint only.
/// </summary>
public static class UsbIds
{
    /// <summary>Production VID shared by every DEPZ sensor.</summary>
    public const int DepzUsbVid = 0x1BCF; // 7119

    public const int PidSr04 = 0xEC78;    // 60536 — HC-SR04 ultrasonic
    public const int PidVl53l8Ch = 0xED40; // 60736 — VL53L8CH ToF
    public const int PidVl53l8Cx = 0xED4B; // 60747 — VL53L8CX ToF (hw-verified)
    public const int PidVl53l4Cd = 0xED45; // 60741 — VL53L4CD single-zone ToF
    public const int PidBno086 = 0xEE08;  // 60936 — BNO086 IMU

    /// <summary>PID → sensor-model hint. Informational: the protocol probe is authoritative.</summary>
    public static readonly IReadOnlyDictionary<int, string> DepzPidModel = new Dictionary<int, string>
    {
        [PidSr04] = "sr04",
        [PidVl53l8Ch] = "vl53l8ch",
        [0xED41] = "vl53l0x",  // 60737
        [0xED42] = "vl53l1cb", // 60738
        [0xED43] = "vl53l1cx", // 60739
        [0xED44] = "vl53l3cx", // 60740
        [PidVl53l4Cd] = "vl53l4cd",
        [0xED46] = "vl53l4cx", // 60742
        [0xED47] = "vl53l4ed", // 60743
        [0xED48] = "vl53l5cx", // 60744
        [0xED49] = "vl53l7cx", // 60745
        [0xED4A] = "vl53l7ch", // 60746
        [PidVl53l8Cx] = "vl53l8cx",
        [PidBno086] = "bno086",
        [0xEE09] = "bno085",   // 60937
        [0xEE0A] = "bno055",   // 60938
    };

    /// <summary>
    /// Inclusive PID range: any PID here under <see cref="DepzUsbVid"/> is a
    /// candidate DEPZ sensor even if not individually mapped (the whole reserved
    /// sensor block 60536..65535).
    /// </summary>
    public const int DepzPidRangeLo = 60536;
    public const int DepzPidRangeHi = 65535;

    /// <summary>Dev / unprogrammed default: STMicroelectronics VID/PID.</summary>
    public const int DevUsbVid = 0x0483; // 1155
    public const int DevUsbPid = 0x56DC; // 22236

    /// <summary>True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.</summary>
    public static bool IsKnownDepzUsb(int? vid, int? pid)
    {
        if (vid is null || pid is null)
            return false;
        if (vid == DevUsbVid && pid == DevUsbPid)
            return true;
        if (vid != DepzUsbVid)
            return false;
        if (DepzPidModel.ContainsKey(pid.Value))
            return true;
        return pid >= DepzPidRangeLo && pid <= DepzPidRangeHi;
    }

    /// <summary>Best-guess model name for a (vid, pid), or null. Informational only.</summary>
    public static string? UsbModelHint(int? vid, int? pid)
    {
        if (vid is null || pid is null)
            return null;
        if (vid == DevUsbVid && pid == DevUsbPid)
            return "dev";
        if (vid == DepzUsbVid)
            return DepzPidModel.TryGetValue(pid.Value, out var m) ? m : null;
        return null;
    }

    /// <summary>A discovered serial port with its USB iSerial (null/empty when absent).</summary>
    public sealed record PortCandidate(string Port, string? Serial);

    /// <summary>
    /// Deterministic discovery order (contract 02 §4): sort candidate ports by
    /// USB iSerial ascending (ordinal), ports with no serial (null or empty)
    /// last, tie-broken by port path (ordinal). Stable, non-mutating.
    /// </summary>
    public static IReadOnlyList<PortCandidate> SerialOrdering(IEnumerable<PortCandidate> ports) =>
        ports
            .OrderBy(p => string.IsNullOrEmpty(p.Serial) ? 1 : 0)
            .ThenBy(p => p.Serial ?? string.Empty, StringComparer.Ordinal)
            .ThenBy(p => p.Port, StringComparer.Ordinal)
            .ToList();
}
