package ai.depz.sensor.android.sample

import ai.depz.sensor.devices.Vl53l8cxDevice
import java.util.Locale

/** One setting on a board's card: a labelled choice. [selected] is written by the main thread only. */
internal class Setting<T>(val label: String, val choices: List<Pair<String, T>>, selected: T) {
    @Volatile var selected: Int = choices.indexOfFirst { it.second == selected }.also {
        require(it >= 0) { "$label: $selected is not one of the choices" }
    }

    val value: T get() = choices[selected].second
}

/** [standard] and [current] — what the sensor holds or the launch asked for — in order, each once. */
internal fun <T : Comparable<T>> withCurrent(standard: List<T>, current: T): List<T> =
    (standard + current).distinct().sorted()

/**
 * The frame rates the card offers for the VL53L5/L7/L8, [wanted] (the launch's) among them. A rate
 * above the 8×8 limit is marked: the app caps the request at the limit of the chosen zones.
 */
internal fun multizoneRates(wanted: Int): List<Pair<String, Int>> {
    val limit8x8 = Vl53l8cxDevice.maxRangingFrequencyHz(64)
    return withCurrent(listOf(Vl53l8cxDevice.MIN_RANGING_FREQUENCY_HZ, 5, 10, 15, 30, 60), wanted)
        .map { hz -> (if (hz > limit8x8) "$hz Hz (4×4)" else "$hz Hz") to hz }
}

/** `"50 ms"`, `"33.3 ms"`. */
internal fun millisText(micros: Long): String =
    if (micros % 1000 == 0L) "${micros / 1000} ms" else String.format(Locale.ROOT, "%.1f ms", micros / 1000.0)
