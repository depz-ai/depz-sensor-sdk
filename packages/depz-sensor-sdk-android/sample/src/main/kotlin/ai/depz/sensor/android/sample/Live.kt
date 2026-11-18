package ai.depz.sensor.android.sample

import ai.depz.sensor.android.Bno055Sensor
import ai.depz.sensor.android.Bno086Sensor
import ai.depz.sensor.android.DepzSensor
import ai.depz.sensor.android.GenericSensor
import ai.depz.sensor.android.Sr04Sensor
import ai.depz.sensor.android.Tmf8829Sensor
import ai.depz.sensor.android.Vl53l4cdSensor
import ai.depz.sensor.android.Vl53l8Sensor
import ai.depz.sensor.android.Vl53lxSensor
import ai.depz.sensor.android.io
import ai.depz.sensor.devices.Vl53l8cxDevice
import ai.depz.sensor.sensors.bno086.Reports
import ai.depz.sensor.sensors.tmf8829.Tmf8829Config
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.map

private const val XSHUT_RESET = 2

/** What the launch asked for (the `am start` extras, see [MainActivity]). */
internal class Launch(
    /** VL53L5/L7/L8: 16 (4×4) or 64 (8×8) zones. */
    val zones: Int,
    /** VL53L5/L7/L8: frames per second, capped at the limit of the zones. */
    val hz: Int,
    /** Pulse XSHUT of a VL53L4CD / 1D ToF sensor before its init. */
    val resetFirst: Boolean,
    /** Stop the sensor before the board is closed. */
    val stopOnClose: Boolean,
    /** Record every board from the moment it streams. */
    val record: Boolean,
)

/**
 * An open sensor as a card runs it: what each family needs between `open` and its first item, the
 * settings the card offers, and the stream as [Reading]s. Every call suspends on the SDK and is
 * safe on the main thread.
 */
internal abstract class Live {
    /** The settings of the card, known after [prepare]. */
    var settings: List<Setting<*>> = emptyList()
        protected set

    /** Once after opening: the sensor's init, and the [settings] with what the sensor holds. */
    open suspend fun prepare() {}

    /** Brings the sensor to the [settings] and starts it; the sensor is not streaming. */
    abstract suspend fun start()

    abstract suspend fun stop()

    /** The stream the card shows and counts. */
    abstract val readings: Flow<Reading>

    /** What the family says about the sensor as it runs now, for the card and the log. */
    open val detail: () -> String = { "" }
}

internal fun liveOf(sensor: DepzSensor, launch: Launch, log: (String) -> Unit): Live = when (sensor) {
    is Sr04Sensor -> Sr04Live(sensor)
    is Vl53l4cdSensor -> Vl53l4cdLive(sensor, launch)
    is Vl53lxSensor -> Vl53lxLive(sensor, launch)
    is Vl53l8Sensor -> Vl53l8Live(sensor, launch, log)
    is Bno055Sensor -> Bno055Live(sensor)
    is Bno086Sensor -> Bno086Live(sensor)
    is Tmf8829Sensor -> Tmf8829Live(sensor)
    is GenericSensor -> NoLive(sensor)
}

/**
 * A board the SDK has no sensor class for: nothing to configure, nothing to show. Its readings
 * are none, and end as every family's — failing with the loss when the board is unplugged — so
 * the card says so: the board's events are what tells.
 */
private class NoLive(private val sensor: GenericSensor) : Live() {
    override suspend fun start() {}
    override suspend fun stop() {}
    override val readings: Flow<Reading> get() = flow { sensor.events.collect { } }
}

private class Sr04Live(private val sensor: Sr04Sensor) : Live() {
    private lateinit var period: Setting<Long>
    private var applied = 0L

    override suspend fun prepare() {
        applied = sensor.io { device.samplePeriodUs } // the board keeps the period it was given
        period = Setting(
            "Period",
            withCurrent(listOf(50_000L, 100_000L, 200_000L, 500_000L, 1_000_000L), applied).map { millisText(it) to it },
            applied,
        )
        settings = listOf(period)
    }

    override suspend fun start() {
        if (period.value != applied) {
            sensor.configure(samplePeriodUs = period.value)
            applied = period.value
        }
        sensor.start()
    }

    override suspend fun stop() = sensor.stop()

    override val readings get() = sensor.measurements.map(::reading)
}

private class Vl53l4cdLive(private val sensor: Vl53l4cdSensor, private val launch: Launch) : Live() {
    private lateinit var budget: Setting<Int>
    private var applied = 0

    override suspend fun prepare() {
        if (launch.resetFirst) sensor.io { device.xshut(XSHUT_RESET) }
        sensor.init()
        applied = sensor.io { device.rangeTiming.timingBudgetMs() }
        budget = Setting("Budget", withCurrent(listOf(10, 20, 33, 50, 100, 200), applied).map { "$it ms" to it }, applied)
        settings = listOf(budget)
    }

    override suspend fun start() {
        if (budget.value != applied) {
            sensor.configure(timingBudgetMs = budget.value)
            applied = budget.value
        }
        sensor.start()
    }

    override suspend fun stop() = sensor.stop()

    override val readings get() = sensor.measurements.map(::reading)
}

/** VL53L0X, L1CX, L1CB, L3CX, L4CX: the budgets come from the driver the board's product binds. */
private class Vl53lxLive(private val sensor: Vl53lxSensor, private val launch: Launch) : Live() {
    private lateinit var budget: Setting<Int>

    override suspend fun prepare() {
        if (launch.resetFirst) sensor.io { device.xshut(XSHUT_RESET) }
        sensor.init()
        val (choices, default) = sensor.io {
            val only = device.budgetChoices()
            val range = device.budgetRange()
            // 50 ms is the SDK's default configuration; a product with a fixed set gets its nearest.
            val default = device.snapBudget(50)
            val offered = only.ifEmpty { listOf(20, 33, 50, 100, 200, 500).filter { it in range[0]..range[1] } }
            withCurrent(offered, default) to default
        }
        budget = Setting("Budget", choices.map { "$it ms" to it }, default)
        settings = listOf(budget)
    }

    override suspend fun start() {
        sensor.configure(budgetMs = budget.value, interMs = 0) // continuous
        sensor.start()
    }

    override suspend fun stop() = sensor.stop()

    override val readings get() = sensor.measurements.map(::reading)
}

/** VL53L5CX, L7CX, L7CH, L8CX, L8CH. */
private class Vl53l8Live(
    private val sensor: Vl53l8Sensor,
    launch: Launch,
    private val log: (String) -> Unit,
) : Live() {
    private val zones = Setting("Zones", listOf("8×8" to 64, "4×4" to 16), if (launch.zones == 16) 16 else 64)
    private val rate = Setting("Rate", multizoneRates(launch.hz), launch.hz)
    @Volatile private var running = ""

    override suspend fun prepare() {
        sensor.init() // downloads the sensor's firmware: some seconds
        settings = listOf(zones, rate)
    }

    override suspend fun start() {
        // After init the sensor ranges at 1 Hz; the rate asked for is capped at the zones' limit.
        sensor.configure(zones.value, rate.value.coerceAtMost(Vl53l8cxDevice.maxRangingFrequencyHz(zones.value)))
        val (nowZones, nowHz) = sensor.io { device.resolution to device.rangingFrequencyHz }
        log("asked zones=${zones.value} hz=${rate.value}, sensor says zones=$nowZones hz=$nowHz")
        running = "$nowZones zones at $nowHz Hz"
        sensor.start()
    }

    override suspend fun stop() = sensor.stop()

    override val readings get() = sensor.frames.map(::reading)

    // The frames the board sent that the app never got: plain reads of counters, no request.
    override val detail = {
        val device = sensor.device
        "$running · discards=${device.reassemblerDiscards()} parseErrors=${device.frames().parseErrors()}"
    }
}

private class Bno055Live(private val sensor: Bno055Sensor) : Live() {
    // The sensor's fusion runs at 100 Hz: the fastest stream worth asking for, and the SDK's default.
    private val rate = Setting("Rate", listOf(100, 50, 20, 10).map { "$it Hz" to it }, 100)

    override suspend fun prepare() {
        sensor.configure()
        settings = listOf(rate)
    }

    override suspend fun start() = sensor.start(periodMs = 1000 / rate.value)

    override suspend fun stop() = sensor.stop()

    override val readings get() = sensor.samples.map(::reading)
}

/** BNO085 / BNO086: the rotation vector and the accelerometer, both at the card's rate. */
private class Bno086Live(private val sensor: Bno086Sensor) : Live() {
    private val rate = Setting("Rate", listOf(200, 100, 50, 10).map { "$it Hz" to it }, 50)
    @Volatile private var granted = ""

    override suspend fun prepare() {
        settings = listOf(rate)
    }

    override suspend fun start() {
        val hz = rate.value.toDouble()
        val rotation = sensor.enable(Reports.ROTATION_VECTOR, hz)
        sensor.enable(Reports.ACCELEROMETER, hz)
        // the hub rounds the rate to its own grid
        granted = if (rotation.intervalUs() > 0) "hub grants ${1_000_000 / rotation.intervalUs()} Hz" else ""
    }

    override suspend fun stop() {
        sensor.disable(Reports.ROTATION_VECTOR)
        sensor.disable(Reports.ACCELEROMETER)
    }

    /** One reading per rotation vector — the card's rate counts those — with the latest acceleration. */
    override val readings: Flow<Reading>
        get() = flow {
            var acceleration: Reports.Acceleration? = null
            sensor.reports.collect { report ->
                when {
                    report is Reports.Acceleration && report.sensorId() == Reports.ACCELEROMETER -> acceleration = report
                    report is Reports.RotationVector && report.sensorId() == Reports.ROTATION_VECTOR ->
                        emit(reading(report, acceleration))
                }
            }
        }

    override val detail = { granted }
}

private class Tmf8829Live(private val sensor: Tmf8829Sensor) : Live() {
    private val preset = Setting("Preset", Tmf8829Config.PRESETS.keys.map { it to it }, "8x8")

    override suspend fun prepare() {
        sensor.init() // downloads the sensor's firmware: some seconds
        settings = listOf(preset)
    }

    override suspend fun start() {
        sensor.configure(preset.value)
        sensor.start()
    }

    override suspend fun stop() = sensor.stop()

    override val readings get() = sensor.measurements.map(::reading)
}
