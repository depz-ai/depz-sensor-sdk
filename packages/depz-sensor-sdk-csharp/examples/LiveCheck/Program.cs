// Live check on real boards: every DEPZ board in /dev/serial/by-id (or the ports given) is opened,
// set streaming the usual way and read for N seconds; per board the rate and one plausibility value
// (the centre distance of a ToF sensor, |gravity| of a BNO055). The same check runs in the Python,
// Rust and Java SDKs, so the numbers can be compared board by board.
//
//     dotnet run --project examples/LiveCheck -- [seconds] [port ...]

using System.Diagnostics;
using System.Globalization;
using Depz.Sensor;
using Depz.Sensor.Bno055;
using Depz.Sensor.Devices;

double secs = args.Length > 0 ? double.Parse(args[0], CultureInfo.InvariantCulture) : 10.0;
var ports = args.Skip(1).ToList();
if (ports.Count == 0)
    ports = Directory.GetFiles("/dev/serial/by-id").OrderBy(p => p, StringComparer.Ordinal).ToList();

int failed = 0;
foreach (string port in ports)
{
    string name = Path.GetFileName(port);
    var clock = Stopwatch.StartNew();
    try
    {
        using DepzDevice dev = DepzDevices.OpenPort(port);
        string sw = dev.GetSoftwareName();
        var (count, values) = Check(dev, secs);
        double? med = Median(values);
        string medText = med is double m ? string.Create(CultureInfo.InvariantCulture, $"{m:F1} {Unit(dev)}") : "—";
        Console.WriteLine(string.Create(CultureInfo.InvariantCulture,
            $"ok    {name,-70} {sw,-22} {count / secs,6:F1}/s  median {medText}  ({clock.Elapsed.TotalSeconds:F1} s)"));
    }
    catch (Exception e) when (e is DepzException or InvalidOperationException or IOException or UnauthorizedAccessException)
    {
        failed++;
        Console.WriteLine($"FAIL  {name,-70} {e.GetType().Name}: {e.Message}");
    }
}
return failed > 0 ? 1 : 0;

static (int Count, List<double> Values) Drain<T>(Subscription<T> sub, double secs, Func<T, double?> f)
{
    var end = Stopwatch.StartNew();
    int n = 0;
    var vals = new List<double>();
    while (end.Elapsed.TotalSeconds < secs)
    {
        try
        {
            T item = sub.Take(TimeSpan.FromMilliseconds(200));
            n++;
            if (f(item) is double v)
                vals.Add(v);
        }
        catch (DepzTimeoutException)
        {
            // nothing in 200 ms: the rate shows it
        }
    }
    return (n, vals);
}

static double? Median(List<double> v)
{
    if (v.Count == 0)
        return null;
    var s = v.OrderBy(x => x).ToList();
    return s[s.Count / 2];
}

// The four centre zones of an 8×8 frame.
static double? Centre(int[] d) => d.Length >= 64 ? (d[27] + d[28] + d[35] + d[36]) / 4.0 : null;

static string Unit(DepzDevice dev) => dev switch
{
    Bno055Device => "|g| m/s²",
    Bno086Device => "reports",
    _ => "mm",
};

static (int Count, List<double> Values) Check(DepzDevice dev, double secs)
{
    switch (dev)
    {
        case Sr04Device d:
        {
            var sub = d.Measurements(256);
            d.Start();
            var r = Drain(sub, secs, m => m.DistanceMm);
            d.Stop();
            return r;
        }
        case Vl53l4cdDevice d:
        {
            d.Init();
            var sub = d.Measurements(256);
            d.StartRanging();
            var r = Drain(sub, secs, m => m.RangeStatus == 0 ? m.DistanceMm : null);
            d.StopRanging();
            return r;
        }
        case Vl53l8cxDevice d:
        {
            d.Init();
            d.SetResolution(64);
            d.SetRangingFrequencyHz(15);
            var sub = d.Frames(64);
            d.StartRanging();
            var r = Drain(sub, secs, f => Centre(f.DistanceMm));
            d.StopRanging();
            return r;
        }
        case Vl53lxDevice d:
        {
            d.Init();
            var sub = d.Measurements(256);
            d.StartRanging();
            var r = Drain(sub, secs, m => m.Status == 0 ? m.DistanceMm : null);
            d.StopRanging();
            return r;
        }
        case Tmf8829Device d:
        {
            d.Init();
            var sub = d.Measurements(64);
            d.Start();
            var r = Drain(sub, secs, m => m.DistanceMm is { } z ? Median(z.Where(x => x > 0).ToList()) : null);
            d.Stop();
            return r;
        }
        case Bno055Device d:
        {
            d.Configure(Bno055OprMode.Ndof);
            var sub = d.Samples(1024);
            d.StartStream(10);
            return Drain(sub, secs, s => s.Gravity is { } g ? Math.Sqrt(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]) : null);
        }
        case Bno086Device d:
        {
            var sub = d.Reports(capacity: 4096);
            d.Enable(0x05, 100.0); // rotation vector
            d.Enable(0x01, 100.0); // accelerometer
            return Drain(sub, secs, _ => null);
        }
        default:
            return (0, new List<double>());
    }
}
