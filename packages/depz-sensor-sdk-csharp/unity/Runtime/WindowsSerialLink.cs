#nullable enable
using System;
using System.IO;
using System.IO.Ports;
using Depz.Sensor.Devices;

namespace Depz.Sensor.Unity
{
    /// <summary>
    /// A serial port as an <see cref="ILink"/> over the System.IO.Ports Unity's
    /// .NET Framework API level provides (Mono's, on Windows): 115200 8N1, DTR
    /// and RTS raised on open as the reference SDK's pyserial does (a CDC device
    /// may hold its output until the host asserts DTR). One read blocks the port
    /// for at most <see cref="MaxReadMs"/> and <see cref="Close"/> waits for the
    /// read in flight before releasing the port, because Mono's port must not be
    /// closed under a blocked read. Errors map as <c>SerialPortLink</c>'s: a port
    /// that cannot be opened or goes away is a <see cref="DeviceLostException"/>,
    /// I/O after Close a <see cref="LinkClosedException"/>.
    /// </summary>
    public sealed class WindowsSerialLink : ILink, IDisposable
    {
        /// <summary>The longest one <see cref="Read"/> holds the port, so <see cref="Close"/> never waits longer.</summary>
        public const int MaxReadMs = 100;

        private readonly SerialPort _port;
        private readonly byte[] _buf = new byte[16384];
        private readonly object _io = new object();
        private volatile bool _closed;

        /// <summary>Open <paramref name="port"/> (<c>COM5</c>; a <c>/dev/tty…</c> path where Mono has ports).</summary>
        /// <exception cref="DeviceLostException">The port does not exist, is held by another program, or is denied.</exception>
        public WindowsSerialLink(string port, int baudRate = 115200)
        {
            if (string.IsNullOrEmpty(port))
                throw new ArgumentException("port name is empty", nameof(port));
            _port = new SerialPort(port, baudRate, Parity.None, 8, StopBits.One)
            {
                DtrEnable = true,
                RtsEnable = true,
                ReadTimeout = 50,
                WriteTimeout = 1000,
            };
            try
            {
                _port.Open();
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is InvalidOperationException)
            {
                throw new DeviceLostException($"cannot open {port}: {e.Message}", e);
            }
            Name = port;
        }

        public string Name { get; }

        public bool IsClosed => _closed || !_port.IsOpen;

        public byte[] Read(TimeSpan timeout)
        {
            if (_closed)
                throw new LinkClosedException($"{Name} closed");
            int ms = (int)Math.Min(Math.Max(1.0, timeout.TotalMilliseconds), MaxReadMs);
            lock (_io)
            {
                if (_closed)
                    throw new LinkClosedException($"{Name} closed");
                try
                {
                    _port.ReadTimeout = ms;
                    int n = _port.Read(_buf, 0, _buf.Length);
                    if (n <= 0)
                        return Array.Empty<byte>();
                    var data = new byte[n];
                    Array.Copy(_buf, data, n);
                    return data;
                }
                catch (TimeoutException)
                {
                    return Array.Empty<byte>();
                }
                catch (Exception e) when (e is IOException || e is InvalidOperationException || e is ObjectDisposedException
                                          || e is UnauthorizedAccessException)
                {
                    if (_closed)
                        throw new LinkClosedException($"{Name} closed");
                    throw new DeviceLostException($"{Name}: {e.Message}", e);
                }
            }
        }

        public void Write(byte[] data)
        {
            if (data is null)
                throw new ArgumentNullException(nameof(data));
            if (_closed)
                throw new LinkClosedException($"{Name} closed");
            try
            {
                _port.Write(data, 0, data.Length);
            }
            catch (Exception e) when (e is IOException || e is InvalidOperationException || e is ObjectDisposedException
                                      || e is UnauthorizedAccessException || e is TimeoutException)
            {
                if (_closed)
                    throw new LinkClosedException($"{Name} closed");
                throw new DeviceLostException($"{Name}: {e.Message}", e);
            }
        }

        public void Dispose() => Close();

        public void Close()
        {
            if (_closed)
                return;
            _closed = true;
            lock (_io) // the read in flight finishes first (at most MaxReadMs)
            {
                try
                {
                    _port.Close();
                }
                catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is InvalidOperationException)
                {
                    // already gone (unplugged)
                }
            }
        }
    }
}
