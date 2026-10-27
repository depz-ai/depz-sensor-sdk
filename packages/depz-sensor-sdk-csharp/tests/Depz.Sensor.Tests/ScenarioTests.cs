using System.Collections;
using System.Globalization;
using System.Reflection;
using System.Text;
using Depz.Sensor.Devices;
using Depz.Sensor.Json;
using Depz.Sensor.Protocol;
using Depz.Sensor.Usb;
using Xunit;

namespace Depz.Sensor.Tests;

// Contract 15 device scenarios: the C# runner. Each contracts/scenarios/<id>.json
// names a real-hardware capture and the API calls that made it. The capture is
// replayed causally and strict-tx (every byte the SDK writes must equal the
// recorded tx stream); the device is opened as `probe` says; each step's result
// is made canonical (§3) and matched against `expect`. Python, TS, C and C++ run
// the same files. Classes without a C# device layer yet stay in
// VECTOR_COVERAGE_EXCEPTIONS.json (second round, one step per sensor).
public class ScenarioTests
{
    private static readonly TimeSpan RequestTimeout = TimeSpan.FromSeconds(2); // as every runner
    private static readonly TimeSpan TakeTimeout = TimeSpan.FromSeconds(5);    // per streamed item

    // Contract 07 §1 layer 3: canonical class → C# class, for the classes ported so far.
    private static readonly Dictionary<string, Type> Classes = new()
    {
        ["sr04"] = typeof(Sr04Device),
        ["vl53l4cd"] = typeof(Vl53l4cdDevice),
        ["vl53l8cx"] = typeof(Vl53l8cxDevice),
        ["vl53l8ch"] = typeof(Vl53l8chDevice),
        ["vl53l5cx"] = typeof(Vl53l5cxDevice),
        ["vl53l7cx"] = typeof(Vl53l7cxDevice),
        ["vl53l7ch"] = typeof(Vl53l7chDevice),
        ["bno055"] = typeof(Bno055Device),
        ["bno086"] = typeof(Bno086Device),
        ["vl53lx"] = typeof(Vl53lxDevice),
        ["vl53l0x"] = typeof(Vl53l0xDevice),
        ["vl53l1cx"] = typeof(Vl53l1cxDevice),
        ["vl53l1cb"] = typeof(Vl53l1cbDevice),
        ["vl53l3cx"] = typeof(Vl53l3cxDevice),
        ["vl53l4cx"] = typeof(Vl53l4cxDevice),
        ["tmf8829"] = typeof(Tmf8829Device),
    };

    // Record arguments (§2): the C# type a record argument becomes → its builder.
    private static readonly Dictionary<Type, Func<Dictionary<string, object?>, object>> RecordArgs = new()
    {
        [typeof(Bno055.Bno055Units)] = r => new Bno055.Bno055Units(
            (bool)r["accel_mg"]!, (bool)r["gyro_rps"]!, (bool)r["euler_rad"]!, (bool)r["temp_f"]!, (bool)r["android"]!),
        [typeof(Vl53l8.Vl53l8CnhSetup)] = r =>
        {
            var setup = new Vl53l8.Vl53l8CnhSetup();
            setup.InitConfig(Int(r["start_bin"]), Int(r["num_bins"]), Int(r["sub_sample"]));
            var m = ((List<object?>)r["agg_map"]!).Select(Int).ToArray();
            setup.CreateAggMap(Int(r["resolution"]), m[0], m[1], m[2], m[3], m[4], m[5]);
            return setup;
        },
    };

    private static int Int(object? v) => Convert.ToInt32(v, CultureInfo.InvariantCulture);

    // Types that are plain tuples in Python (canonical form: an array of their fields, in order).
    private static readonly HashSet<Type> TupleTypes = new()
    {
        typeof(Bno055.Bno055Vec3), typeof(Bno055.Bno055Euler), typeof(Bno055.Bno055Quat),
    };

    // The alias table (§2): canonical names C# spells otherwise than PascalCase(name), per class.
    private static readonly Dictionary<(Type, string), string> Aliases = new();

    private static readonly string ScenarioDir = Path.Combine(
        Path.GetDirectoryName(Vectors.RecordingPath("x"))!, "..", "..", "scenarios");

    // Scenarios of a ported class whose driver is not ported yet (they stay in the coverage exceptions).
    private static readonly Dictionary<string, string> Pending = new()
    {
    };

    public static IEnumerable<object[]> Files() =>
        Directory.GetFiles(ScenarioDir, "*.json")
            .Select(p => (Path: p, Sc: Load(p)))
            .Where(x => Classes.ContainsKey((string)x.Sc["class"]!) && !Pending.ContainsKey((string)x.Sc["id"]!))
            .Select(x => new object[] { Path.GetFileName(x.Path) })
            .OrderBy(x => (string)x[0], StringComparer.Ordinal);

    private static Dictionary<string, object?> Load(string path) =>
        (Dictionary<string, object?>)JsonText.Parse(File.ReadAllText(path, Encoding.UTF8))!;

    [Theory]
    [MemberData(nameof(Files))]
    public void Scenario(string name)
    {
        var sc = Load(Path.Combine(ScenarioDir, name));
        Assert.Equal("depz.scenario/1", sc["schema"]);
        string id = (string)sc["id"]!;
        Assert.Equal($"{id}.json", name);

        // No drops: each queue holds everything its scenario takes, plus what streams on after.
        var steps = ((List<object?>)sc["steps"]!).Cast<Dictionary<string, object?>>().ToList();
        var sizes = new Dictionary<string, int>();
        foreach (var s in steps.Where(s => s.ContainsKey("take")))
            sizes[(string)s["take"]!] = sizes.GetValueOrDefault((string)s["take"]!) + (int)(long)s["count"]!;

        var link = new ReplayLink(Vectors.RecordingPath((string)sc["recording"]!), strictTx: true);
        DepzDevice dev = Open(sc, link);
        var subs = new Dictionary<string, object>();
        try
        {
            Assert.True(dev.GetType() == Classes[(string)sc["class"]!],
                $"resolved {dev.GetType().Name}, not {sc["class"]}");
            for (int i = 0; i < steps.Count; i++)
            {
                try
                {
                    RunStep(dev, steps[i], subs, sizes);
                }
                catch (Exception e)
                {
                    throw new Xunit.Sdk.XunitException($"{id} step {i} {Describe(steps[i])}: {Unwrap(e).GetType().Name}: {Unwrap(e).Message}");
                }
            }
            foreach (var kv in subs)
            {
                long dropped = (long)kv.Value.GetType().GetProperty("DroppedCount")!.GetValue(kv.Value)!;
                Assert.True(dropped == 0, $"{kv.Key}: {dropped} items dropped");
            }
            if ((sc.GetValueOrDefault("end") as string ?? "open") == "exhausted")
            {
                var sw = System.Diagnostics.Stopwatch.StartNew();
                while (!(link.RxExhausted && link.TxRemaining == 0) && sw.Elapsed < TimeSpan.FromSeconds(2))
                    Thread.Sleep(10);
                Assert.True(link.RxExhausted && link.TxRemaining == 0,
                    $"{id}: capture not exhausted: {link.TxRemaining} tx bytes never written, {link.RxRemaining} rx chunks never delivered");
            }
        }
        finally
        {
            dev.Close();
        }
        Vectors.Seen($"scenarios/{id}");
        Vectors.Seen($"recordings/{sc["recording"]}");
    }

    // ── opening (§1 probe) ───────────────────────────────────────────────────

    private static DepzDevice Open(Dictionary<string, object?> sc, ReplayLink link)
    {
        int? vid = null, pid = null;
        if (sc.GetValueOrDefault("usb_model") is string model)
        {
            var hit = UsbIds.DepzPidModel.Where(kv => kv.Value == model).Select(kv => (int?)kv.Key).FirstOrDefault()
                ?? throw new InvalidDataException($"usb_model '{model}' is not in the USB-id table");
            (vid, pid) = (UsbIds.DepzUsbVid, hit);
        }
        string probe = sc.GetValueOrDefault("probe") as string ?? "canonical";
        if (probe == "canonical")
        {
            Assert.False(sc.ContainsKey("identity"), "`identity` is for legacy captures only");
            return DepzDevices.OpenOnLink(link, vid, pid, RequestTimeout);
        }
        Assert.Equal("legacy", probe);
        // The probe the SDKs made before contract 02 §4.4: GET_NAME_ACTIVE_SOFTWARE, plus
        // GET_DEVICE_NAME for the L5/L7 and 1D families only; the class attached in place.
        var dev = new DepzDevice(link, RequestTimeout);
        try
        {
            string software = dev.GetSoftwareName();
            var ident = IdentityParser.ParseSoftwareName(software);
            string deviceName = "";
            if (ident.SensorType == SensorType.Vl53l7 || ident.SensorType == SensorType.Vl53lx)
            {
                try { deviceName = dev.GetDeviceName(); }
                catch (DepzException) { deviceName = ""; }
            }
            if (sc.GetValueOrDefault("identity") is Dictionary<string, object?> want)
            {
                string? m = Mismatch(Canon(ident), want, "identity");
                if (m != null)
                    throw new Xunit.Sdk.XunitException(m);
            }
            return DepzDevices.Attach(dev, Discovery.ResolveSensorClass(software, deviceName, vid, pid));
        }
        catch
        {
            dev.Close();
            throw;
        }
    }

    // ── steps (§2) ───────────────────────────────────────────────────────────

    private static void RunStep(DepzDevice dev, Dictionary<string, object?> step,
        Dictionary<string, object> subs, Dictionary<string, int> sizes)
    {
        if (step.TryGetValue("call", out object? call))
        {
            var args = step.GetValueOrDefault("args") as List<object?> ?? new List<object?>();
            if (step.TryGetValue("error", out object? kind))
            {
                object? got;
                try
                {
                    got = Call(dev, (string)call!, args);
                }
                catch (Exception e)
                {
                    var inner = Unwrap(e);
                    string? k = ErrorKind(inner);
                    Assert.True(k == (string)kind!,
                        $"expected error {kind}, got {k ?? "unmapped"} ({inner.GetType().Name}: {inner.Message})");
                    return;
                }
                throw new Xunit.Sdk.XunitException($"expected error {kind}, succeeded with {JsonText.Serialize(Canon(got))}");
            }
            object? result = Call(dev, (string)call!, args);
            if (step.TryGetValue("expect", out object? want))
                Check(Mismatch(Canon(result), want, ""));
        }
        else if (step.TryGetValue("subscribe", out object? stream))
        {
            string name = (string)step["as"]!;
            int capacity = sizes.GetValueOrDefault(name) + 4096;
            subs[name] = Subscribe(dev, (string)stream!, capacity);
        }
        else if (step.TryGetValue("take", out object? subName))
        {
            object sub = subs[(string)subName!];
            var want = (List<object?>)step["expect"]!;
            Assert.Equal((long)step["count"]!, (long)want.Count);
            var take = sub.GetType().GetMethod("Take")!;
            for (int i = 0; i < want.Count; i++)
            {
                object? item;
                try
                {
                    item = take.Invoke(sub, new object[] { TakeTimeout });
                }
                catch (TargetInvocationException e) when (e.InnerException is DepzTimeoutException)
                {
                    throw new Xunit.Sdk.XunitException($"got {i} of {want.Count} items, then nothing for {TakeTimeout.TotalSeconds} s");
                }
                Check(Mismatch(Canon(item), want[i], $"item[{i}]"));
            }
        }
        else if (step.TryGetValue("get", out object? prop))
        {
            string member = Member(dev.GetType(), (string)prop!);
            object? value = dev.GetType().GetProperty(member) is PropertyInfo pi
                ? pi.GetValue(dev)
                : Call(dev, (string)prop!, new List<object?>());
            Check(Mismatch(Canon(value), step["expect"], (string)prop!));
        }
        else
        {
            throw new Xunit.Sdk.XunitException($"unknown step {JsonText.Serialize(step)}");
        }
    }

    private static void Check(string? mismatch)
    {
        if (mismatch != null)
            throw new Xunit.Sdk.XunitException(mismatch);
    }

    private static string Member(Type type, string canonical)
    {
        for (var t = type; t != null; t = t.BaseType)
            if (Aliases.TryGetValue((t, canonical), out string? alias))
                return alias;
        return string.Concat(canonical.Split('_').Select(w => w.Length == 0 ? w : char.ToUpperInvariant(w[0]) + w.Substring(1)));
    }

    // A stream's subscribe method, its queue sized by name (as Python passes maxsize=); every other
    // parameter (a sensor filter) at its default.
    private static object Subscribe(DepzDevice dev, string canonical, int capacity)
    {
        string name = Member(dev.GetType(), canonical);
        var method = dev.GetType().GetMethods(BindingFlags.Public | BindingFlags.Instance)
            .SingleOrDefault(m => m.Name == name && m.GetParameters().Any(p => p.Name == "capacity"))
            ?? throw new Xunit.Sdk.XunitException($"{dev.GetType().Name} has no {name}(capacity): add '{canonical}' to the alias table");
        var args = method.GetParameters().Select(p => p.Name == "capacity" ? capacity : Type.Missing).ToArray();
        return method.Invoke(dev, BindingFlags.OptionalParamBinding, null, args, CultureInfo.InvariantCulture)!;
    }

    // Positional JSON arguments onto the method's parameters; omitted trailing ones take their defaults.
    private static object? Call(DepzDevice dev, string canonical, List<object?> args)
    {
        string name = Member(dev.GetType(), canonical);
        var methods = dev.GetType().GetMethods(BindingFlags.Public | BindingFlags.Instance)
            .Where(m => m.Name == name && !m.IsGenericMethod)
            .Where(m => m.GetParameters().Length >= args.Count
                        && m.GetParameters().Skip(args.Count).All(p => p.IsOptional))
            .ToList();
        if (methods.Count == 0)
            throw new Xunit.Sdk.XunitException($"{dev.GetType().Name} has no {name}({args.Count} args): add '{canonical}' to the alias table");
        var method = methods[0];
        var ps = method.GetParameters();
        var call = new object?[ps.Length];
        for (int i = 0; i < ps.Length; i++)
            call[i] = i < args.Count ? Arg(args[i], ps[i].ParameterType, canonical, i) : Type.Missing;
        return method.Invoke(dev, BindingFlags.OptionalParamBinding, null, call, CultureInfo.InvariantCulture);
    }

    private static object? Arg(object? a, Type t, string call, int pos)
    {
        Type target = Nullable.GetUnderlyingType(t) ?? t;
        if (a is Dictionary<string, object?> rec)
            return RecordArgs.TryGetValue(target, out var build)
                ? build(rec)
                : throw new Xunit.Sdk.XunitException($"{call} argument {pos} is a record with no builder for {target.Name}");
        if (a is null)
            return null;
        if (target.IsEnum)
            return Enum.ToObject(target, Convert.ToInt64(a, CultureInfo.InvariantCulture));
        if (a is List<object?> items && target.IsGenericType && target.FullName!.StartsWith("System.ValueTuple", StringComparison.Ordinal))
        {
            var fields = target.GetGenericArguments();
            if (fields.Length != items.Count)
                throw new Xunit.Sdk.XunitException($"{call} argument {pos}: {items.Count} items for a {fields.Length}-tuple");
            return Activator.CreateInstance(target, items.Select((x, k) => Arg(x, fields[k], call, pos)).ToArray());
        }
        if (target == typeof(TimeSpan))
            throw new Xunit.Sdk.XunitException($"{call} argument {pos}: scenarios carry no timeouts");
        return Convert.ChangeType(a, target, CultureInfo.InvariantCulture);
    }

    // §4: the canonical kind of a C# exception, null for anything else.
    private static string? ErrorKind(Exception e) => e switch
    {
        DepzTimeoutException => "timeout",
        StatusException s => s.Status == (int)Status.ErrCmdNotSupported ? "not_supported" : "device_error",
        InvalidReplyException => "invalid_reply",
        ArgumentException => "invalid_argument",
        NotSupportedException => "not_supported",
        _ => null,
    };

    private static Exception Unwrap(Exception e) =>
        e is TargetInvocationException { InnerException: { } inner } ? Unwrap(inner) : e;

    private static string Describe(Dictionary<string, object?> step)
    {
        if (step.TryGetValue("call", out object? c))
            return $"{c}({string.Join(", ", (step.GetValueOrDefault("args") as List<object?> ?? new List<object?>()).Select(JsonText.Serialize))})";
        if (step.TryGetValue("subscribe", out object? s))
            return $"subscribe {s} as {step["as"]}";
        if (step.TryGetValue("take", out object? t))
            return $"take {step["count"]} from {t}";
        return $"get {step.GetValueOrDefault("get")}";
    }

    // ── matching (§3) ────────────────────────────────────────────────────────

    /// <summary>A result as canonical JSON: records keyed by the Python field names (snake_case of
    /// the C# property), enums as integers, bytes as hex. C# holds both Python <c>bytes</c> and small
    /// numeric arrays (zone statuses, uint8 arrays) in <c>byte[]</c>: a property is a byte string
    /// when its name says raw data (<c>…Raw</c>, <c>Data</c>), else an array of numbers.</summary>
    internal static object? Canon(object? v) => Canon(v, bytesAsHex: true);

    private static bool IsByteString(string property) =>
        property == "Data" || property.EndsWith("Raw", StringComparison.Ordinal);

    private static object? Canon(object? v, bool bytesAsHex)
    {
        switch (v)
        {
            case null: return null;
            case bool or string: return v;
            case SensorType st: return st.ToString().ToLowerInvariant();
            case Enum e: return Convert.ToInt64(e, CultureInfo.InvariantCulture);
            case ulong u: return u <= long.MaxValue ? (object)(long)u : u;
            case sbyte or byte or short or ushort or int or uint or long: return Convert.ToInt64(v, CultureInfo.InvariantCulture);
            case float or double or decimal: return Convert.ToDouble(v, CultureInfo.InvariantCulture);
            case byte[] b when bytesAsHex: return BitConverter.ToString(b).Replace("-", "").ToLowerInvariant();
            case Bno086.BnoReport r: return Canon(r.Fields()); // the Python field names, kept by the SDK
            case Vl53lx.Bare.HistogramBinData h: return Canon(h.Fields());
            case IDictionary d:
            {
                var o = new Dictionary<string, object?>();
                foreach (DictionaryEntry kv in d)
                    o[Convert.ToString(kv.Key, CultureInfo.InvariantCulture)!] = Canon(kv.Value);
                return o;
            }
            case IEnumerable list: return list.Cast<object?>().Select(Canon).ToList();
        }
        var t = v.GetType();
        if (t.IsGenericType && t.FullName!.StartsWith("System.ValueTuple", StringComparison.Ordinal))
            return t.GetFields().Select(f => Canon(f.GetValue(v))).ToList();
        if (TupleTypes.Contains(t))
            return t.GetConstructors().OrderByDescending(c => c.GetParameters().Length).First().GetParameters()
                .Select(p => Canon(t.GetProperty(p.Name!)!.GetValue(v))).ToList();
        var obj = new Dictionary<string, object?>();
        foreach (var p in t.GetProperties(BindingFlags.Public | BindingFlags.Instance))
        {
            if (p.GetIndexParameters().Length > 0 || p.Name == "EqualityContract")
                continue;
            obj[Snake(p.Name)] = Canon(p.GetValue(v), bytesAsHex: IsByteString(p.Name));
        }
        return obj;
    }

    internal static string Snake(string pascal)
    {
        var sb = new StringBuilder();
        for (int i = 0; i < pascal.Length; i++)
        {
            char c = pascal[i];
            if (char.IsUpper(c))
            {
                bool prevLowerOrDigit = i > 0 && (char.IsLower(pascal[i - 1]) || char.IsDigit(pascal[i - 1]));
                bool acronymEnd = i > 0 && char.IsUpper(pascal[i - 1]) && i + 1 < pascal.Length && char.IsLower(pascal[i + 1]);
                if (prevLowerOrDigit || acronymEnd)
                    sb.Append('_');
                sb.Append(char.ToLowerInvariant(c));
            }
            else
            {
                sb.Append(c);
            }
        }
        return sb.ToString();
    }

    /// <summary>§3 matching; null when <paramref name="got"/> matches <paramref name="want"/>.</summary>
    internal static string? Mismatch(object? got, object? want, string path)
    {
        string at = path.Length == 0 ? "result" : path;
        switch (want)
        {
            case Dictionary<string, object?> w:
                if (got is not Dictionary<string, object?> g)
                    return $"{at}: expected an object, got {JsonText.Serialize(got)}";
                foreach (var kv in w)
                {
                    if (!g.TryGetValue(kv.Key, out object? gv))
                        return $"{at}.{kv.Key}: missing (fields: {string.Join(", ", g.Keys)})";
                    string? m = Mismatch(gv, kv.Value, $"{path}.{kv.Key}");
                    if (m != null)
                        return m;
                }
                return null;
            case List<object?> wl:
                if (got is not List<object?> gl)
                    return $"{at}: expected an array, got {JsonText.Serialize(got)}";
                if (gl.Count != wl.Count)
                    return $"{at}: expected {wl.Count} elements, got {gl.Count}";
                for (int i = 0; i < wl.Count; i++)
                {
                    string? m = Mismatch(gl[i], wl[i], $"{path}[{i}]");
                    if (m != null)
                        return m;
                }
                return null;
            case double wd:
                if (got is long or ulong or double)
                {
                    double gd = Convert.ToDouble(got, CultureInfo.InvariantCulture);
                    if (Math.Abs(gd - wd) <= Math.Max(1e-9 * Math.Abs(wd), 1e-12))
                        return null;
                }
                return $"{at}: expected {JsonText.Serialize(wd)}, got {JsonText.Serialize(got)}";
            case long wl2 when got is double gd2:
                // An integer expectation matches a floating-point result of the same value: a binding may
                // keep a field as double where Python has an int (TS has one number type for both).
                return gd2 == wl2 ? null : $"{at}: expected {wl2}, got {JsonText.Serialize(gd2)}";
            default:
                // integers, strings, booleans, null: exact, and a bool never equals an integer
                if (got?.GetType() == want?.GetType() && Equals(got, want))
                    return null;
                return $"{at}: expected {JsonText.Serialize(want)}, got {JsonText.Serialize(got)}";
        }
    }
}

public class ScenarioRunnerSelfTests
{
    [Theory]
    [InlineData("TimestampUs", "timestamp_us")]
    [InlineData("EchoTimeUs", "echo_time_us")]
    [InlineData("SiliconTempDegc", "silicon_temp_degc")]
    [InlineData("NbTargetDetected", "nb_target_detected")]
    [InlineData("Vl53l8Model", "vl53l8_model")]
    [InlineData("CnhRaw", "cnh_raw")]
    [InlineData("USBSerial", "usb_serial")]
    public void SnakeCase(string pascal, string snake) => Assert.Equal(snake, ScenarioTests.Snake(pascal));

    [Fact]
    public void MatchingFollowsContract15()
    {
        var got = ScenarioTests.Canon(new Sr04Measurement(5, 6, "loop"));
        Assert.Null(ScenarioTests.Mismatch(got, JsonText.Parse("{\"timestamp_us\":5,\"source\":\"loop\"}"), ""));
        // An integer result matches a floating-point expectation within tolerance.
        Assert.Null(ScenarioTests.Mismatch(got, JsonText.Parse("{\"timestamp_us\":5.0}"), ""));
        Assert.NotNull(ScenarioTests.Mismatch(got, JsonText.Parse("{\"nope\":1}"), ""));
        Assert.NotNull(ScenarioTests.Mismatch(true, 1L, ""));
        Assert.NotNull(ScenarioTests.Mismatch(new List<object?> { 1L }, JsonText.Parse("[1,2]"), ""));
    }
}
