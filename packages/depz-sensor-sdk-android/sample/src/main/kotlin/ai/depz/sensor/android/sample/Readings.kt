package ai.depz.sensor.android.sample

import ai.depz.sensor.devices.Bno055Sample
import ai.depz.sensor.devices.Sr04Measurement
import ai.depz.sensor.devices.Vl53l4Measurement
import ai.depz.sensor.devices.Vl53lxMeasurement
import ai.depz.sensor.sensors.bno086.Reports
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame
import java.util.Locale
import kotlin.math.roundToInt

/**
 * What a board's card draws for the sensor's latest item. Presentation only: the values are the
 * Java SDK's, already decoded; here they are picked and laid out for the screen.
 */
internal sealed interface Reading {
    /** One distance as a number. [mm] `null`: the sensor has no distance to show, [note] says why. */
    data class Distance(val mm: Double?, val note: String) : Reading

    /**
     * The zones of a multizone sensor, row-major as the SDK gives them: [mm] holds `rows × cols`
     * distances, `NaN` for a zone without a valid one.
     */
    class Grid(val rows: Int, val cols: Int, val mm: DoubleArray, val note: String) : Reading

    /** An IMU's orientation and acceleration, each a line of text (`"—"` until the first value). */
    data class Imu(val orientation: String, val acceleration: String) : Reading
}

/** A zone of the VL53L5/L7/L8 holds a distance when its target status is 5 or 9 — the web viewer's rule. */
internal fun zoneValid(targetStatus: Int): Boolean = targetStatus == 5 || targetStatus == 9

internal fun reading(m: Sr04Measurement): Reading =
    Reading.Distance(m.distanceMm(), if (m.valid()) "echo ${m.echoTimeUs()} µs" else "no echo")

internal fun reading(m: Vl53l4Measurement): Reading =
    Reading.Distance(if (m.valid()) m.distanceMm().toDouble() else null, m.statusText())

/** The distance the SDK calls plottable (the first such target of a histogram product), else none. */
internal fun reading(m: Vl53lxMeasurement): Reading =
    Reading.Distance(m.plotDistances().firstOrNull()?.toDouble(), m.statusText())

internal fun reading(frame: Vl53l8Frame): Reading.Grid {
    val zones = frame.resolution()
    val side = if (zones == 16) 4 else 8
    val mm = DoubleArray(zones) { i ->
        if (zoneValid(frame.targetStatus()[i])) frame.distanceMm()[i].toDouble() else Double.NaN
    }
    val valid = mm.count { !it.isNaN() }
    return Reading.Grid(side, side, mm, "$valid of $zones zones valid · sensor ${frame.siliconTempDegc()} °C")
}

/**
 * The first peak of every pixel; a distance of 0 is "no target" on the TMF8829 (as the web viewer
 * shows it). A format without distances gives an empty grid.
 */
internal fun reading(m: MeasurementBuilder.Measurement): Reading.Grid {
    val distances = m.distanceMm() ?: return Reading.Grid(0, 0, DoubleArray(0), "this format carries no distances")
    val rows = m.results().rows()
    val cols = m.results().cols()
    val mm = DoubleArray(rows * cols) { i -> distances[i].takeIf { it > 0.0 } ?: Double.NaN }
    val targets = mm.count { !it.isNaN() }
    return Reading.Grid(rows, cols, mm, "$targets of ${rows * cols} pixels with a target · ${cols}×$rows")
}

/** The quaternion and the Euler angles the sensor's fusion gives, and its acceleration, in the units in effect. */
internal fun reading(s: Bno055Sample): Reading {
    val angle = if (s.units().eulerRad()) "rad" else "°"
    val accel = if (s.units().accelMg()) "mg" else "m/s²"
    val orientation = listOfNotNull(
        s.euler()?.let { "heading ${num(it[0], 1)} roll ${num(it[1], 1)} pitch ${num(it[2], 1)} $angle" },
        s.quaternion()?.let { quaternionText(it[0], it[1], it[2], it[3]) },
    ).joinToString("\n")
    return Reading.Imu(orientation.ifEmpty { "—" }, s.accel()?.let { xyzText(it[0], it[1], it[2], accel) } ?: "—")
}

/** The BNO086's latest rotation vector and accelerometer report (either still `null` at the start). */
internal fun reading(rotation: Reports.RotationVector?, acceleration: Reports.Acceleration?): Reading = Reading.Imu(
    rotation?.let { quaternionText(it.real(), it.i(), it.j(), it.k()) } ?: "—",
    acceleration?.let { xyzText(it.x(), it.y(), it.z(), "m/s²") } ?: "—",
)

internal fun quaternionText(w: Double, x: Double, y: Double, z: Double): String =
    "q  w ${num(w, 3)}  x ${num(x, 3)}  y ${num(y, 3)}  z ${num(z, 3)}"

internal fun xyzText(x: Double, y: Double, z: Double, unit: String): String =
    "x ${num(x, 2)}  y ${num(y, 2)}  z ${num(z, 2)} $unit"

private fun num(value: Double, decimals: Int): String = String.format(Locale.ROOT, "%+.${decimals}f", value)

/** `"1234 mm"`, or `"—"` without a distance. */
internal fun distanceText(mm: Double?): String = if (mm == null) "—" else String.format(Locale.ROOT, "%.0f mm", mm)

/** The distance the heatmap's scale ends at: farther zones take the far colour. */
internal const val HEAT_FAR_MM = 4000.0

/**
 * The heatmap's colour (opaque ARGB) of a distance: red at 0, through yellow, green and cyan to
 * blue at [farMm] and beyond.
 */
internal fun heatColor(mm: Double, farMm: Double = HEAT_FAR_MM): Int {
    val sector = (mm / farMm).coerceIn(0.0, 1.0) * 4.0 // red → yellow → green → cyan → blue
    val index = minOf(sector.toInt(), 3)
    val up = (255 * (sector - index)).roundToInt()
    val down = 255 - up
    val (r, g, b) = when (index) {
        0 -> Triple(255, up, 0)
        1 -> Triple(down, 255, 0)
        2 -> Triple(0, 255, up)
        else -> Triple(0, down, 255)
    }
    return (0xFF shl 24) or (r shl 16) or (g shl 8) or b
}
