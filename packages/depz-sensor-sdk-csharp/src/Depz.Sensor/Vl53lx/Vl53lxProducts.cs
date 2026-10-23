using System.Text.RegularExpressions;

namespace Depz.Sensor.Vl53lx;

/// <summary>
/// The three kinds of driver ST ships for the 1D family, in the order a UI
/// should list them (contract 12 §1). What separates them is where the ranging
/// arithmetic runs.
/// </summary>
public enum Vl53lxDriverKind
{
    /// <summary>Ultra Lite Driver — the die computes the distance.</summary>
    Uld,

    /// <summary>Ultra Low Power — the ULD shape with power saving (VL53L3CX only).</summary>
    Ulp,

    /// <summary>ST's Bare Driver — the die hands over 24 photon bins, the host finds up to four targets.</summary>
    Histogram,
}

/// <summary>
/// Sensor class an <c>APP_VL53L0_4</c> board resolves to (contract 12 §1).
/// <see cref="Vl53lx"/> is the generic class that takes the product at init —
/// also what a VL53L4CD board on this firmware gets.
/// </summary>
public enum Vl53lxClass
{
    Vl53lx,
    Vl53l0x,
    Vl53l1cx,
    Vl53l1cb,
    Vl53l3cx,
    Vl53l4cx,
}

/// <summary>
/// One row of the product table: what is true of the part itself, plus the
/// bridge parameters its default driver runs with (register-address width,
/// interrupt-release steps, bus ceiling). The model id is a cross-check only:
/// L1CX/L1CB share 0xEACC, L4CD/L4CX share 0xEBAA.
/// </summary>
public sealed record Vl53lxProduct(
    string Name,
    int UsbPid,
    ushort ModelId,
    int ReachMm,
    IReadOnlyList<Vl53lxDriverKind> DriverKinds,
    Vl53lxDriverKind DefaultDriver,
    int AddrWidth,
    IReadOnlyList<Vl53lxClearStep> ClearSteps,
    int MaxKhz);

/// <summary>Product table and board-name / class resolution (contract 12 §1).</summary>
public static class Vl53lxProducts
{
    private static readonly Vl53lxClearStep[] DieClear = { new(0x0086, 0x01) };
    private static readonly Vl53lxClearStep[] L0xClear = { new(0x0B, 0x01), new(0x0B, 0x00) };

    /// <summary>Every product of the family, in the order a UI should list them.</summary>
    public static readonly IReadOnlyList<Vl53lxProduct> All = new Vl53lxProduct[]
    {
        new("VL53L0X", 0xED41, 0x00EE, 2000, new[] { Vl53lxDriverKind.Uld },
            Vl53lxDriverKind.Uld, 1, L0xClear, 400),
        new("VL53L1CX", 0xED43, 0xEACC, 4000, new[] { Vl53lxDriverKind.Uld, Vl53lxDriverKind.Histogram },
            Vl53lxDriverKind.Uld, 2, DieClear, 1000),
        new("VL53L1CB", 0xED42, 0xEACC, 8000, new[] { Vl53lxDriverKind.Uld, Vl53lxDriverKind.Histogram },
            Vl53lxDriverKind.Uld, 2, DieClear, 1000),
        new("VL53L3CX", 0xED44, 0xEAAA, 3000, new[] { Vl53lxDriverKind.Ulp, Vl53lxDriverKind.Histogram },
            Vl53lxDriverKind.Ulp, 2, DieClear, 1000),
        new("VL53L4CD", 0xED45, 0xEBAA, 1300, new[] { Vl53lxDriverKind.Uld, Vl53lxDriverKind.Histogram },
            Vl53lxDriverKind.Uld, 2, DieClear, 1000),
        new("VL53L4CX", 0xED46, 0xEBAA, 6000, new[] { Vl53lxDriverKind.Histogram },
            Vl53lxDriverKind.Histogram, 2, DieClear, 1000),
    };

    private static readonly Regex NameRe = new(@"VL53L(\d[A-Z0-9]*)", RegexOptions.Compiled);

    /// <summary>The table row for a product name (exact, upper case), or null.</summary>
    public static Vl53lxProduct? Find(string? name)
    {
        foreach (var p in All)
            if (p.Name == name)
                return p;
        return null;
    }

    /// <summary>
    /// <c>"ToF Sensor VL53L4CD USB v2.1"</c> → <c>"VL53L4CD"</c>: the first
    /// <c>VL53L&lt;digit&gt;&lt;part&gt;</c> match, case-insensitive. Null if the
    /// name carries no family product (an unstamped board, an unknown part).
    /// </summary>
    public static string? ProductFromBoardName(string? name)
    {
        if (string.IsNullOrEmpty(name))
            return null;
        Match m = NameRe.Match(name.ToUpperInvariant());
        if (!m.Success)
            return null;
        string product = "VL53L" + m.Groups[1].Value;
        return Find(product) is null ? null : product;
    }

    /// <summary>
    /// Pick the class (normative order): the production USB PID model
    /// (<see cref="Usb.UsbIds.UsbModelHint"/>) if it is a family product, else
    /// the product the device name carries, else the generic
    /// <see cref="Vl53lxClass.Vl53lx"/>. VL53L4CD maps to the generic class on
    /// this firmware (the <c>vl53l4cd</c> class belongs to <c>APP_VL53L4</c>).
    /// </summary>
    public static Vl53lxClass ResolveClass(string? usbModel, string? deviceName)
    {
        string? product = string.IsNullOrEmpty(usbModel) ? null : usbModel.ToUpperInvariant();
        if (Find(product) is null)
            product = ProductFromBoardName(deviceName);
        return product switch
        {
            "VL53L0X" => Vl53lxClass.Vl53l0x,
            "VL53L1CX" => Vl53lxClass.Vl53l1cx,
            "VL53L1CB" => Vl53lxClass.Vl53l1cb,
            "VL53L3CX" => Vl53lxClass.Vl53l3cx,
            "VL53L4CX" => Vl53lxClass.Vl53l4cx,
            _ => Vl53lxClass.Vl53lx,
        };
    }

    /// <summary>
    /// Cross-check the id the sensor answered against the product asked for.
    /// True only says "not something else entirely".
    /// </summary>
    public static bool ModelIdOk(string product, int value) => Find(product)?.ModelId == value;

    /// <summary>The driver kinds in the order a UI lists them (uld/registry.py DRIVER_KINDS).</summary>
    public static readonly IReadOnlyList<string> DriverKindNames = new[] { "uld", "ulp", "histogram" };

    private const string HistCaveat =
        "the histogram driver has no calibrations and no detection thresholds - the light drivers are the ones with those";

    // Which (product, kind) pairs exist and what to tell the user about each; a missing pair is absent on purpose.
    private static readonly Dictionary<string, Dictionary<string, (Func<Vl53lxPlatform, string, Vl53lxDriver> Create, string? Caveat)>> Drivers = new()
    {
        ["VL53L0X"] = new()
        {
            ["uld"] = ((p, n) => new Vl53l0xUldDriver(p, n), "no detection thresholds; there is no ROI on this die at all"),
        },
        ["VL53L1CX"] = new()
        {
            ["uld"] = ((p, n) => new Vl53l1UldDriver(p, n), null),
            ["histogram"] = ((p, n) => new Vl53lxHistogramDriver(p, n), HistCaveat),
        },
        ["VL53L1CB"] = new()
        {
            ["uld"] = ((p, n) => new Vl53l1UldDriver(p, n), null),
            ["histogram"] = ((p, n) => new Vl53lxHistogramDriver(p, n), HistCaveat),
        },
        ["VL53L3CX"] = new()
        {
            ["ulp"] = ((p, n) => new Vl53l3UlpDriver(p, n), "single-target ranging only - the histogram driver gives several targets instead"),
            ["histogram"] = ((p, n) => new Vl53lxHistogramDriver(p, n), HistCaveat),
        },
        ["VL53L4CD"] = new()
        {
            ["uld"] = ((p, n) => new Vl53l4UldDriver(p, n), null),
            ["histogram"] = ((p, n) => new Vl53lxHistogramDriver(p, n), HistCaveat),
        },
        ["VL53L4CX"] = new()
        {
            ["histogram"] = ((p, n) => new Vl53lxHistogramDriver(p, n), HistCaveat),
        },
    };

    /// <summary>The driver kinds <paramref name="product"/> has, in <see cref="DriverKindNames"/> order.</summary>
    /// <exception cref="NotSupportedException">No such product.</exception>
    public static IReadOnlyList<string> DriverKinds(string product)
    {
        var have = Row(product);
        return DriverKindNames.Where(have.ContainsKey).ToList();
    }

    /// <summary>The driver for one (product, kind) pair, and its caveat or null.</summary>
    /// <exception cref="NotSupportedException">A pair the table has no row for.</exception>
    public static (Vl53lxDriver Driver, string? Caveat) CreateDriver(string product, string kind, Vl53lxPlatform platform)
    {
        var row = Row(product);
        if (!row.TryGetValue(kind, out var entry))
            throw new NotSupportedException($"{product} has no '{kind}' driver - it has {string.Join(", ", DriverKinds(product))}");
        return (entry.Create(platform, product), entry.Caveat);
    }

    private static Dictionary<string, (Func<Vl53lxPlatform, string, Vl53lxDriver> Create, string? Caveat)> Row(string product) =>
        Drivers.TryGetValue(product, out var row)
            ? row
            : throw new NotSupportedException($"no such product: {product} - served: {string.Join(", ", All.Select(p => p.Name))}");
}
