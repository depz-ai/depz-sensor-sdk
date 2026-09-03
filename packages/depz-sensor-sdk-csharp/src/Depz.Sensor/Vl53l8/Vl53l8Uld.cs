namespace Depz.Sensor.Vl53l8;

/// <summary>
/// Placeholder for the live VL53L8CX ULD init/config driver (sensor-firmware
/// download + DCI register-bridge configuration).
///
/// OUT OF SCOPE for this SDK layer: that driver is a 1:1 register-sequence port
/// that only means anything against real silicon over the CDC link, and cannot
/// be verified from golden vectors. It is deliberately stubbed. The verifiable
/// decode layer lives in <see cref="Vl53l8FrameDecoder"/> (results frames) and
/// <see cref="Vl53l8Advanced"/> / <see cref="MotionConfig"/> (DCI payload
/// codecs), which are covered by the golden vectors.
/// </summary>
public static class Vl53l8Uld
{
    /// <summary>Not implemented: requires live hardware (see class remarks).</summary>
    public static void Init() =>
        throw new NotSupportedException(
            "Live VL53L8CX ULD init (firmware download + register-bridge config) is " +
            "hardware-dependent and out of scope for the decode SDK. Use " +
            "Vl53l8FrameDecoder / Vl53l8Advanced for the verifiable codec layer.");
}
