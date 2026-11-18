package ai.depz.sensor.android

import ai.depz.sensor.devices.Bno055Device
import ai.depz.sensor.devices.Bno055Sample
import ai.depz.sensor.devices.Bno086Device
import ai.depz.sensor.devices.DepzDevice
import ai.depz.sensor.devices.DeviceEvent
import ai.depz.sensor.devices.DeviceLostException
import ai.depz.sensor.devices.Sr04Device
import ai.depz.sensor.devices.Sr04Measurement
import ai.depz.sensor.devices.Tmf8829AppliedConfig
import ai.depz.sensor.devices.Tmf8829Device
import ai.depz.sensor.devices.Vl53l4Measurement
import ai.depz.sensor.devices.Vl53l4cdDevice
import ai.depz.sensor.devices.Vl53l7chDevice
import ai.depz.sensor.devices.Vl53l8chDevice
import ai.depz.sensor.devices.Vl53l8cxDevice
import ai.depz.sensor.devices.Vl53lxDevice
import ai.depz.sensor.devices.Vl53lxMeasurement
import ai.depz.sensor.sensors.bno055.Bno055Regs
import ai.depz.sensor.sensors.bno086.Reports
import ai.depz.sensor.sensors.bno086.Sh2Hub
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder
import ai.depz.sensor.sensors.tmf8829.Tmf8829Config
import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame
import java.io.OutputStream
import java.time.Duration
import java.util.concurrent.Executor
import java.util.function.Predicate
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext

/**
 * An open DEPZ sensor board, as Kotlin sees it: what [DepzAndroid.open] returns. One subclass per
 * sensor family — [Sr04Sensor], [Vl53l4cdSensor], [Vl53lxSensor], [Vl53l8Sensor], [Bno055Sensor],
 * [Bno086Sensor], [Tmf8829Sensor], and [GenericSensor] for a board the SDK has no class for — so a
 * `when (sensor)` over them is exhaustive and each has its own typed stream and its own setup:
 *
 * ```kotlin
 * DepzAndroid.open(context, board).use { sensor ->
 *     when (sensor) {
 *         is Vl53l8Sensor -> {
 *             sensor.init()
 *             sensor.configure(resolution = 64, rangingFrequencyHz = 15)
 *             sensor.start()
 *             sensor.frames.collect { frame -> … }   // ends when the sensor is closed or lost
 *         }
 *         …
 *     }
 * }
 * ```
 *
 * Every class is a thin layer over the Java SDK's device class of the family ([device]): the
 * protocol, the drivers and the decoding are the Java SDK's, and so are the measurement types.
 *
 * **Threading.** The Java SDK's calls block (a request waits for the board's reply); here every
 * one is a `suspend` function that runs it on [Dispatchers.IO], so all of them may be called from
 * the main thread. The streams are [Flow]s collected in the collector's own context. Reading a
 * property of this class never blocks. What this class does not wrap is reached with [io]:
 * `sensor.io { device.offsetMm = 5 }`. Calling a method of [device] directly is for a background
 * thread only.
 *
 * **Cancellation.** A cancelled `suspend` call does not interrupt the request under way — a board
 * is never left with half a command — so the call returns (with `CancellationException`) once
 * that request ended, within its timeouts. Cancelling the collection of a stream closes its
 * subscription at once; the sensor goes on measuring until its `stop()`.
 *
 * **Streams.** Each stream is a cold [Flow]: a collection subscribes when it starts and gets the
 * items from then on, so start collecting before the sensor's `start()` to see the first ones.
 * Several collections may run at once. Each has a buffer of [DEFAULT_BUFFER] items between the
 * device's reader thread and the collector; when the collector is slower than the sensor the
 * oldest item is dropped, never the reader held. Another size: `.buffer(n,
 * BufferOverflow.DROP_OLDEST)` on the flow. A stream ends in one of two ways:
 *  - the program's own [close] → the flow completes normally (a collection started after it
 *    completes at once);
 *  - the board is lost (unplugged, a USB error) → the flow fails with the Java SDK's
 *    [DeviceLostException], whose `cause` is what the USB link reported; the same exception is
 *    then in [lost], and every later call fails with it. The lost sensor still has to be closed.
 *    (Where kotlinx.coroutines' stack-trace recovery is on — a JVM with assertions enabled, as in
 *    unit tests; not on Android by default — an exception that crosses coroutines is a copy of
 *    the same class whose `cause` is the original: there the link's report is one `cause` deeper.)
 *
 * **Errors** are the Java SDK's exceptions (`DepzException` and its subclasses:
 * `DepzTimeoutException`, `StatusException`, `InvalidStateException`, `DeviceLostException`…),
 * thrown by the `suspend` call that met them.
 *
 * **Recording.** [record] writes the sensor's measurements into a dataset file as the desktop
 * SDKs do ([DepzRecording]); the bytes of the link itself are captured from [DepzAndroid.open].
 *
 * **Cleanup.** [close] is idempotent and returns at once; [closeAndWait] also waits until the USB
 * interface is released — wait for it before the same board is opened again. `use { }` works.
 *
 * **From Java**, without coroutines. The `suspend` functions and the [Flow]s are Kotlin's and are
 * not visible from Java. A Java program gets the streams through a [StreamListener] and gives the
 * commands to the Java SDK's own [device], whose methods block, on a background thread of its:
 *
 * ```java
 * Sr04Sensor sr04 = (Sr04Sensor) sensor;
 * Cancellable subscription = sr04.measurements(new StreamListener<Sr04Measurement>() {
 *     @Override public void onItem(Sr04Measurement m) { … }       // main thread
 *     @Override public void onError(DeviceLostException e) { … }  // the board is lost
 *     @Override public void onEnd() { … }                         // the sensor was closed
 * });
 * background.execute(() -> sr04.getDevice().start());             // a Java SDK call: it blocks
 * …
 * subscription.cancel();
 * sensor.close();                                                 // returns at once
 * ```
 *
 * `sensor.close(callback)` answers once the USB interface is released: what to wait for before
 * the same board is opened again.
 *
 * A listener's methods are called on the executor the subscription names — the main thread when
 * it names none — one at a time and in order; a lambda is a listener of the items alone
 * (`sr04.measurements(m -> show(m))`). The subscription is in place when the call returns, with
 * the same buffer between the reader thread and the executor ([DEFAULT_BUFFER] items, drop-oldest;
 * another size is the call's last argument); a listener that throws is unsubscribed and its exception left to the executor's
 * thread. [Cancellable.cancel] closes the subscription: no call after it.
 */
sealed class DepzSensor(private val dispatcher: CoroutineDispatcher) : AutoCloseable {
    /**
     * The Java SDK's device this sensor wraps, for everything this class has no function for. Its
     * methods block: call them inside [io], or on a background thread.
     */
    abstract val device: DepzDevice

    /**
     * The board's diagnostic events (the Java SDK's `DepzDevice.events()`): unsolicited statuses,
     * sequence gaps, CRC errors, text, the disconnect. Ends as every stream of this class does.
     */
    @get:JvmSynthetic
    val events: Flow<DeviceEvent> get() = device.events().asFlow(DEFAULT_BUFFER)

    /**
     * [events] for a program without coroutines ("From Java" above): subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun events(
        listener: StreamListener<DeviceEvent>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.events().deliver(listener, executor, capacity)

    /** True once the sensor closed, by [close] or by a loss ([lost] tells which). */
    val isClosed: Boolean get() = device.isClosed

    /** The loss that ended this sensor (its `cause` is what the link reported), or `null`. */
    val lost: DeviceLostException? get() = device.lost()

    private val closing: Deferred<Unit> by lazy { CoroutineScope(dispatcher).async { device.close() } }

    /**
     * Closes the sensor: the streams complete, the calls under way fail, the USB interface is
     * released. Returns at once — the work is done on [Dispatchers.IO] — so it may be called from
     * the main thread; calling it again does nothing. A sensor that streams is not stopped first:
     * call its `stop()` before, where the board should fall silent.
     */
    final override fun close() {
        closing.start()
    }

    /**
     * [close], and suspends until the sensor is closed: the reader thread ended and the USB
     * interface released, so the board can be opened again. Throws what closing the link threw,
     * if anything; the sensor is closed all the same.
     */
    @JvmSynthetic
    suspend fun closeAndWait() = closing.await()

    /**
     * [closeAndWait] for a program without coroutines: closes the sensor and returns at once, and
     * [callback] gets on [executor] `onResult(null)` once the sensor is closed — the reader thread
     * ended and the USB interface released, so the board can be opened again — or `onError` with
     * what closing the link threw; the sensor is closed all the same. [isClosed] is no such
     * signal: it is true from the moment the closing began.
     *
     * @param executor where [callback] is called; the main thread when omitted
     * @return the handle that gives the answer up ([Cancellable.cancel]): [callback] is not
     *   called; the sensor closes all the same
     */
    @JvmOverloads
    fun close(callback: DepzCallback<Void?>, executor: Executor = MainExecutor): Cancellable {
        close()
        return CALLS.answer(callback, executor) {
            closing.await()
            null
        }
    }

    /**
     * Starts recording this sensor's measurements into [output] as a dataset — the `.depzdata`
     * file of the desktop SDKs — and returns the [DepzRecording] that runs: close it to end the
     * recording; closing the sensor, or losing it, ends it too. [DepzRecording.start] has the
     * rules and the parameters, and records several sensors into one file.
     *
     * The sensor is not touched beyond a clock synchronisation: start it before or after.
     * [output] is the recording's from this call on, closed also when the recording could not
     * start or the call was cancelled.
     */
    @JvmSynthetic
    suspend fun record(
        output: OutputStream,
        note: String = "",
        vl53l8Layers: Boolean = false,
        temperatureHz: Double? = DepzRecording.DEFAULT_TEMPERATURE_HZ,
    ): DepzRecording = DepzRecording.start(output, listOf(this), note, vl53l8Layers, temperatureHz, dispatcher)

    /**
     * [record] for a program without coroutines: returns at once, and [callback] gets on
     * [executor] `onResult` with the recording that runs, which the program then closes
     * ([DepzRecording.close]), or `onError` with what the `suspend` variant throws.
     *
     * @param executor where [callback] is called; the main thread when omitted
     * @return the handle that gives the recording up ([Cancellable.cancel]): [callback] is not
     *   called, nothing records and [output] is closed
     */
    @JvmOverloads
    fun record(
        output: OutputStream,
        callback: DepzCallback<DepzRecording>,
        executor: Executor = MainExecutor,
        note: String = "",
        vl53l8Layers: Boolean = false,
        temperatureHz: Double? = DepzRecording.DEFAULT_TEMPERATURE_HZ,
    ): Cancellable = CALLS.answer(callback, executor, discard = { it.close() }) {
        DepzRecording.start(output, listOf(this), note, vl53l8Layers, temperatureHz, dispatcher)
    }

    internal suspend fun <R> onIo(block: () -> R): R = withContext(dispatcher) { block() }

    companion object {
        /** The items a stream's collection buffers before it drops the oldest: 64. */
        const val DEFAULT_BUFFER: Int = 64

        /**
         * [device] — a Java SDK device opened on any link (a `ReplayLink` over a recorded
         * session, a link of the app's own) — as the [DepzSensor] of its family. The sensor owns
         * the device from here: closing the sensor closes it.
         */
        @JvmStatic
        fun of(device: DepzDevice): DepzSensor = of(device, Dispatchers.IO)

        internal fun of(device: DepzDevice, dispatcher: CoroutineDispatcher): DepzSensor = when (device) {
            is Sr04Device -> Sr04Sensor(device, dispatcher)
            is Vl53l4cdDevice -> Vl53l4cdSensor(device, dispatcher)
            is Vl53lxDevice -> Vl53lxSensor(device, dispatcher)
            is Vl53l8cxDevice -> Vl53l8Sensor(device, dispatcher)
            is Bno055Device -> Bno055Sensor(device, dispatcher)
            is Bno086Device -> Bno086Sensor(device, dispatcher)
            is Tmf8829Device -> Tmf8829Sensor(device, dispatcher)
            else -> GenericSensor(device, dispatcher)
        }
    }
}

/**
 * Runs [block] — calls of the Java SDK's [DepzSensor.device], which block — on
 * [Dispatchers.IO] and returns its result: `val offset = sensor.io { device.offsetMm }`.
 * Cancelling does not interrupt the block ([DepzSensor], "Cancellation").
 */
@JvmSynthetic
suspend fun <S : DepzSensor, R> S.io(block: S.() -> R): R = onIo { block() }

/** A board the SDK has no sensor class for: its [events], and the common commands through [io]. */
class GenericSensor internal constructor(
    override val device: DepzDevice,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher)

/** HC-SR04 ultrasonic ranger (`Sr04Device`). */
class Sr04Sensor internal constructor(
    override val device: Sr04Device,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher) {
    /** The measurements of the loop ([start]). */
    @get:JvmSynthetic
    val measurements: Flow<Sr04Measurement> get() = device.measurements().asFlow(DEFAULT_BUFFER)

    /**
     * [measurements] for a program without coroutines ([DepzSensor], "From Java"): subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun measurements(
        listener: StreamListener<Sr04Measurement>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.measurements().deliver(listener, executor, capacity)

    /**
     * Sets what is not `null`: the minimum interval between measurement starts
     * (`setSamplePeriodUs`) and the post-echo settle pause (`setEchoDecayUs`, which the board
     * clamps to its range).
     */
    @JvmSynthetic
    suspend fun configure(samplePeriodUs: Long? = null, echoDecayUs: Int? = null): Unit = onIo {
        samplePeriodUs?.let { device.setSamplePeriodUs(it) }
        echoDecayUs?.let { device.setEchoDecayUs(it) }
    }

    /** Starts the measurement loop (idempotent). */
    @JvmSynthetic
    suspend fun start(): Unit = onIo { device.start() }

    /** Stops the measurement loop (idempotent). */
    @JvmSynthetic
    suspend fun stop(): Unit = onIo { device.stop() }

    /** One measurement, outside the loop; [timeout] `null` is the Java SDK's 1 s. */
    @JvmSynthetic
    suspend fun measureOnce(timeout: Duration? = null): Sr04Measurement = onIo { device.measureOnce(timeout) }
}

/** VL53L4CD single-zone ToF sensor (`Vl53l4cdDevice`). */
class Vl53l4cdSensor internal constructor(
    override val device: Vl53l4cdDevice,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher) {
    /** The ranging measurements, one per frame while the sensor ranges ([start]). */
    @get:JvmSynthetic
    val measurements: Flow<Vl53l4Measurement> get() = device.measurements().asFlow(DEFAULT_BUFFER)

    /**
     * [measurements] for a program without coroutines ([DepzSensor], "From Java"): subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun measurements(
        listener: StreamListener<Vl53l4Measurement>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.measurements().deliver(listener, executor, capacity)

    /** Initialises the sensor (the driver's default configuration); first after opening. */
    @JvmSynthetic
    suspend fun init(): Unit = onIo { device.init() }

    /**
     * The timing budget of one measurement and the pause between two ([interMeasurementMs] 0:
     * continuous) — `setRangeTiming`. After [init], not while ranging.
     */
    @JvmSynthetic
    suspend fun configure(timingBudgetMs: Int, interMeasurementMs: Long = 0): Unit =
        onIo { device.setRangeTiming(timingBudgetMs, interMeasurementMs) }

    /** Starts ranging (`startRanging`). */
    @JvmSynthetic
    suspend fun start(): Unit = onIo { device.startRanging() }

    /** Stops ranging (`stopRanging`; nothing when it does not range). */
    @JvmSynthetic
    suspend fun stop(): Unit = onIo { device.stopRanging() }

    /** One polled measurement, not while ranging; [timeout] `null` is the Java SDK's 1 s. */
    @JvmSynthetic
    suspend fun measureOnce(timeout: Duration? = null): Vl53l4Measurement = onIo { device.measureOnce(timeout) }
}

/**
 * The single-zone ToF family on the 1D bridge — VL53L0X, L1CX, L1CB, L3CX, L4CX
 * (`Vl53lxDevice` and its product classes).
 */
class Vl53lxSensor internal constructor(
    override val device: Vl53lxDevice,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher) {
    /** The ranging measurements while the sensor ranges ([start]). */
    @get:JvmSynthetic
    val measurements: Flow<Vl53lxMeasurement> get() = device.measurements().asFlow(DEFAULT_BUFFER)

    /**
     * [measurements] for a program without coroutines ([DepzSensor], "From Java"): subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun measurements(
        listener: StreamListener<Vl53lxMeasurement>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.measurements().deliver(listener, executor, capacity)

    /**
     * Binds the driver and initialises the sensor; first after opening. [driver] `null`: the
     * product's first driver kind (`device.driverKinds()` lists them); [product] `null`: the one
     * the board names.
     */
    @JvmSynthetic
    suspend fun init(driver: String? = null, product: String? = null): Unit = onIo { device.init(driver, product) }

    /** The Java SDK's default ranging configuration (`configure()`). After [init], not while ranging. */
    @JvmSynthetic
    suspend fun configure(): Unit = onIo { device.configure() }

    /**
     * Re-initialises the sensor and applies a ranging configuration (`Vl53lxDevice.configure`):
     * the timing budget, the pause between measurements ([interMs] 0: continuous), the distance
     * [mode] (`device.modes()`), a stored offset / crosstalk calibration, a signal threshold —
     * each `null` leaves the sensor's. After [init], not while ranging.
     */
    @JvmSynthetic
    suspend fun configure(
        budgetMs: Int,
        interMs: Int,
        mode: String? = null,
        offsetMm: Int? = null,
        xtalkKcps: Int? = null,
        signalKcps: Int? = null,
    ): Unit = onIo { device.configure(budgetMs, interMs, mode, offsetMm, xtalkKcps, signalKcps) }

    /** Starts ranging (`startRanging`). */
    @JvmSynthetic
    suspend fun start(): Unit = onIo { device.startRanging() }

    /** Stops ranging (`stopRanging`). */
    @JvmSynthetic
    suspend fun stop(): Unit = onIo { device.stopRanging() }

    /** One polled measurement, not while ranging; [timeout] `null` is the Java SDK's default. */
    @JvmSynthetic
    suspend fun measureOnce(timeout: Duration? = null): Vl53lxMeasurement = onIo { device.measureOnce(timeout) }
}

/**
 * The multizone ToF family — VL53L5CX, L7CX, L7CH, L8CX, L8CH (`Vl53l8cxDevice` and its
 * subclasses; [device] is the board's own class).
 */
class Vl53l8Sensor internal constructor(
    override val device: Vl53l8cxDevice,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher) {
    /** The frames while the sensor ranges ([start]): 4×4 or 8×8 zones each. */
    @get:JvmSynthetic
    val frames: Flow<Vl53l8Frame> get() = device.frames().asFlow(DEFAULT_BUFFER)

    /**
     * [frames] for a program without coroutines ([DepzSensor], "From Java"): subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun frames(
        listener: StreamListener<Vl53l8Frame>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.frames().deliver(listener, executor, capacity)

    /**
     * Downloads the sensor's firmware and its default configuration — some seconds; first after
     * opening. [progress] gets the phase texts, [writeProgress] (done, total) the big firmware
     * writes, both on a background thread.
     */
    @JvmSynthetic
    suspend fun init(
        progress: ((String) -> Unit)? = null,
        writeProgress: ((done: Int, total: Int) -> Unit)? = null,
    ): Unit = onIo { device.init(progress?.let { p -> { p(it) } }, writeProgress?.let { p -> { done, total -> p(done, total) } }) }

    /**
     * Sets what is not `null`, the resolution first (the frequency's limit depends on it): the
     * zones (16 or 64, `setResolution`) and the frames per second (`setRangingFrequencyHz`; the
     * limit is `Vl53l8cxDevice.maxRangingFrequencyHz(zones)`). After [init], not while ranging.
     */
    @JvmSynthetic
    suspend fun configure(resolution: Int? = null, rangingFrequencyHz: Int? = null): Unit = onIo {
        resolution?.let { device.setResolution(it) }
        rangingFrequencyHz?.let { device.setRangingFrequencyHz(it) }
    }

    /**
     * Arms CNH (the per-zone histograms of the VL53L7CH / L8CH) for the next [start]: every frame
     * then carries `cnhRaw`. After [init], not while ranging.
     *
     * @throws UnsupportedOperationException the board is not a CH sensor
     */
    @JvmSynthetic
    suspend fun configureCnh(setup: Vl53l8CnhSetup): Unit = onIo {
        when (val dev = device) {
            is Vl53l8chDevice -> dev.configureCnh(setup)
            is Vl53l7chDevice -> dev.configureCnh(setup)
            else -> throw UnsupportedOperationException("${dev.javaClass.simpleName} has no CNH: a VL53L7CH or VL53L8CH does")
        }
    }

    /** Starts ranging (`startRanging`). */
    @JvmSynthetic
    suspend fun start(): Unit = onIo { device.startRanging() }

    /** Stops ranging (`stopRanging`). */
    @JvmSynthetic
    suspend fun stop(): Unit = onIo { device.stopRanging() }
}

/** BNO055 IMU (`Bno055Device`). */
class Bno055Sensor internal constructor(
    override val device: Bno055Device,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher) {
    /** The samples of the stream ([start]), decoded with the units in effect. */
    @get:JvmSynthetic
    val samples: Flow<Bno055Sample> get() = device.samples().asFlow(DEFAULT_BUFFER)

    /**
     * [samples] for a program without coroutines ([DepzSensor], "From Java"): subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun samples(
        listener: StreamListener<Bno055Sample>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.samples().deliver(listener, executor, capacity)

    /** The Java SDK's default setup (`configure()`): the NDOF fusion mode, the power-on units. */
    @JvmSynthetic
    suspend fun configure(): Unit = onIo { device.configure() }

    /**
     * The session setup (`Bno055Device.configure`): the operation [mode], the [units] (`null`: the
     * power-on ones), an axis remap and a calibration profile (`null`: the sensor's).
     */
    @JvmSynthetic
    suspend fun configure(
        mode: Bno055Regs.OprMode,
        units: Bno055Regs.Units? = null,
        axisRemap: Bno055Regs.AxisRemap? = null,
        calibration: Bno055Regs.CalibrationProfile? = null,
    ): Unit = onIo { device.configure(mode, units, axisRemap, calibration) }

    /** Starts the stream as the Java SDK's `startStream()` does: the full block every 10 ms. */
    @JvmSynthetic
    suspend fun start(): Unit = onIo { device.startStream() }

    /**
     * Starts the stream (`startStream`): the register [block] (`{address, length}`; `null`: the
     * full block) read every [periodMs] and pushed. Replaces a running stream.
     */
    @JvmSynthetic
    suspend fun start(periodMs: Int, block: IntArray? = null): Unit = onIo { device.startStream(periodMs, block) }

    /** Stops the stream (`stopStream`). */
    @JvmSynthetic
    suspend fun stop(): Unit = onIo { device.stopStream() }
}

/** BNO085 / BNO086 IMU (`Bno086Device`): its reports are enabled one sensor at a time. */
class Bno086Sensor internal constructor(
    override val device: Bno086Device,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher) {
    /** The reports of every enabled sensor: one of the `Reports` records each. */
    @get:JvmSynthetic
    val reports: Flow<Reports.Report> get() = device.reports().asFlow(DEFAULT_BUFFER)

    /**
     * The reports of one sensor only ([sensorId]: `Reports.ROTATION_VECTOR`, `Reports.ACCELEROMETER`…),
     * filtered before the buffer: another sensor's reports take no room in it.
     */
    @JvmSynthetic
    fun reports(sensorId: Int): Flow<Reports.Report> =
        device.reports().asFlow(DEFAULT_BUFFER, Predicate { it.sensorId() == sensorId })

    /**
     * [reports] for a program without coroutines ([DepzSensor], "From Java"): subscribed when this
     * returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun reports(
        listener: StreamListener<Reports.Report>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.reports().deliver(listener, executor, capacity)

    /**
     * The reports of one sensor only ([sensorId]), for a program without coroutines: filtered
     * before the buffer, subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun reports(
        sensorId: Int, listener: StreamListener<Reports.Report>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.reports().deliver(listener, executor, capacity, Predicate { it.sensorId() == sensorId })

    /**
     * Enables the reports of [sensor] at [hz] (`Bno086Device.enable`) and returns the rate the
     * hub granted, read back: it rounds to its own grid.
     */
    @JvmSynthetic
    suspend fun enable(sensor: Int, hz: Double): Sh2Hub.FeatureResponse = onIo { device.enable(sensor, hz) }

    /** Disables the reports of [sensor]. */
    @JvmSynthetic
    suspend fun disable(sensor: Int): Unit = onIo { device.disable(sensor) }
}

/** TMF8829 multizone ToF sensor (`Tmf8829Device`). */
class Tmf8829Sensor internal constructor(
    override val device: Tmf8829Device,
    dispatcher: CoroutineDispatcher,
) : DepzSensor(dispatcher) {
    /** The measurements while the sensor measures ([start]). */
    @get:JvmSynthetic
    val measurements: Flow<MeasurementBuilder.Measurement> get() = device.measurements().asFlow(DEFAULT_BUFFER)

    /**
     * [measurements] for a program without coroutines ([DepzSensor], "From Java"): subscribed when this returns.
     *
     * @param executor where [listener] is called; the main thread when omitted
     * @param capacity the items buffered for [listener] before the oldest is dropped
     * @return the handle that closes the subscription
     */
    @JvmOverloads
    fun measurements(
        listener: StreamListener<MeasurementBuilder.Measurement>,
        executor: Executor = MainExecutor,
        capacity: Int = DEFAULT_BUFFER,
    ): Cancellable = device.measurements().deliver(listener, executor, capacity)

    /**
     * Downloads the sensor's firmware and starts it — some seconds; first after opening. Stops a
     * running stream. [progress] (done, total) tracks the download, on a background thread.
     */
    @JvmSynthetic
    suspend fun init(progress: ((done: Int, total: Int) -> Unit)? = null): Tmf8829Device.InitResult =
        onIo { device.init(progress?.let { p -> { done, total -> p(done, total) } }) }

    /** The Java SDK's default configuration (`configure()`): the 8x8 preset at its default period. */
    @JvmSynthetic
    suspend fun configure(): Tmf8829AppliedConfig = onIo { device.configure() }

    /**
     * Loads [preset] (`Tmf8829Config.PRESETS`) with a measurement every [periodMs] and returns
     * the configuration the sensor holds. After [init], not while measuring.
     */
    @JvmSynthetic
    suspend fun configure(preset: String, periodMs: Int = Tmf8829Config.DEFAULT_PERIOD_MS): Tmf8829AppliedConfig =
        onIo { device.configure(preset, periodMs) }

    /** Starts measuring; configures the default preset first when [configure] was not called. */
    @JvmSynthetic
    suspend fun start(): Unit = onIo { device.start() }

    /** Stops measuring. */
    @JvmSynthetic
    suspend fun stop(): Unit = onIo { device.stop() }
}
