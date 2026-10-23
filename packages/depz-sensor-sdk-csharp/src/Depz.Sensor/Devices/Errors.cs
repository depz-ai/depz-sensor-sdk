using Depz.Sensor.Protocol;

namespace Depz.Sensor.Devices;

/// <summary>Base of every live-layer error (contract 07). Arguments the SDK refuses before
/// sending anything throw <see cref="ArgumentException"/> instead.</summary>
public class DepzException : Exception
{
    public DepzException(string message) : base(message) { }
    public DepzException(string message, Exception inner) : base(message, inner) { }
}

/// <summary>No reply to a request within its timeout (default 200 ms).</summary>
public class DepzTimeoutException : DepzException
{
    /// <summary>The request opcode that went unanswered.</summary>
    public int Cmd { get; }

    public DepzTimeoutException(int cmd, TimeSpan timeout)
        : base($"no reply to cmd 0x{cmd:X2} within {timeout.TotalSeconds:0.000}s") => Cmd = cmd;

    public DepzTimeoutException(string message) : base(message) { }
}

/// <summary>The device answered a request with a non-OK RPT_STATUS (contract 02 §3).</summary>
public class StatusException : DepzException
{
    public int Cmd { get; }
    public int Status { get; }

    /// <summary>The contract 02 §3 name of <see cref="Status"/>, e.g. <c>ERR_INVALID_PARAM</c>.</summary>
    public string StatusName { get; }

    public StatusException(int cmd, int status)
        : this(cmd, status, $"cmd 0x{cmd:X2} failed: {NameOf(status)} (0x{status:X2})") { }

    protected StatusException(int cmd, int status, string message) : base(message)
    {
        Cmd = cmd;
        Status = status;
        StatusName = NameOf(status);
    }

    internal static string NameOf(int status) => status switch
    {
        (int)Protocol.Status.Ok => "OK",
        (int)Protocol.Status.Error => "ERROR",
        (int)Protocol.Status.ErrInvalidCmd => "ERR_INVALID_CMD",
        (int)Protocol.Status.ErrPayloadFormat => "ERR_PAYLOAD_FORMAT",
        (int)Protocol.Status.ErrInvalidParam => "ERR_INVALID_PARAM",
        (int)Protocol.Status.ErrPayloadCrc => "ERR_PAYLOAD_CRC",
        (int)Protocol.Status.ErrBusy => "ERR_BUSY",
        (int)Protocol.Status.ErrCmdNotSupported => "ERR_CMD_NOT_SUPPORTED",
        (int)Protocol.Status.ErrNotInitialized => "ERR_NOT_INITIALIZED",
        (int)Protocol.Status.ErrHardwareFault => "ERR_HARDWARE_FAULT",
        _ => $"STATUS_0x{status:X2}",
    };
}

/// <summary>ERR_BUSY from the device, or a request whose opcode is already in flight.</summary>
public sealed class BusyException : StatusException
{
    public BusyException(int cmd)
        : base(cmd, (int)Protocol.Status.ErrBusy, $"cmd 0x{cmd:X2}: busy (ERR_BUSY or already in flight)") { }
}

/// <summary>The connection went away: unplugged, closed, or the OS refused I/O.</summary>
public class DeviceLostException : DepzException
{
    public DeviceLostException(string message) : base(message) { }
    public DeviceLostException(string message, Exception inner) : base(message, inner) { }
}

/// <summary>I/O on a link that is already closed.</summary>
public sealed class LinkClosedException : DeviceLostException
{
    public LinkClosedException(string message) : base(message) { }
}

/// <summary>Discovery found no DEPZ device to open.</summary>
public sealed class NoDepzDeviceException : DepzException
{
    public NoDepzDeviceException(string message) : base(message) { }
    public NoDepzDeviceException(string message, Exception inner) : base(message, inner) { }
}

/// <summary>The device is in bootloader mode: use the contract 06 client, not a sensor class.</summary>
public sealed class BootloaderModeException : DepzException
{
    public BootloaderModeException()
        : base("device is in bootloader mode; use the bootloader client (contract 06)") { }
}

/// <summary>The opened device is not the sensor class the caller asked for.</summary>
public sealed class WrongTypeException : DepzException
{
    public WrongTypeException(string message) : base(message) { }
}

/// <summary>A <see cref="ReplayLink"/> in strict mode saw the SDK write bytes the capture does not hold.</summary>
public sealed class ReplayMismatchException : DepzException
{
    public ReplayMismatchException(string message) : base(message) { }
}
