package ai.depz.sensor.android

import ai.depz.sensor.dataset.SessionRecorder
import ai.depz.sensor.devices.Unsubscribe
import java.io.OutputStream
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.Executor
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.async
import kotlinx.coroutines.withContext

/**
 * A recording under way: the measurements of one or several open sensors written, decoded and on
 * one host timeline, into a dataset — the `.depzdata` file of the desktop SDKs (contract 09),
 * which every DEPZ SDK reads and plays (`DatasetReader`, `DatasetPlayer`) and the web viewer
 * opens. [DepzSensor.record] starts one for a sensor, [start] for several:
 *
 * ```kotlin
 * val recording = sensor.record(File(context.filesDir, "run.depzdata").outputStream())
 * …                                  // the sensor streams; every item is a record of the file
 * recording.closeAndWait()           // the file is complete
 * ```
 *
 * The recorder is the Java SDK's `SessionRecorder` and the file is what it writes on desktop: the
 * recording hooks each sensor's stream beside the app's own collections and listeners, and never
 * configures, starts or stops a sensor — the app does. The output is any [OutputStream] the app
 * opened: a file of its storage, `contentResolver.openOutputStream(uri)` for a document the user
 * picked (the Storage Access Framework), a `GZIPOutputStream` over either for a `.depzdata.gz`.
 * The recording owns the stream from the call that starts it and closes it — when the recording
 * ends, and also when it could not start.
 *
 * **How it ends.** By [close] (or [closeAndWait]); by itself once every recorded sensor is closed
 * or lost — of several sensors, the ones still open go on being recorded; and a start that is
 * cancelled leaves nothing recording. However it ends, the file is closed and holds every record
 * written until then.
 *
 * A record is written on the sensor's reader thread, so a slow stream holds the sensor's other
 * subscribers: record into a file, not into a network socket.
 */
class DepzRecording internal constructor(
    private val recorder: SessionRecorder,
    sensors: List<DepzSensor>,
    dispatcher: CoroutineDispatcher,
) : AutoCloseable {
    private val closed = AtomicBoolean(false)
    private val open = AtomicInteger(sensors.size)
    private val watches = CopyOnWriteArrayList<Unsubscribe>()
    private val closing: Deferred<Long> by lazy {
        CoroutineScope(dispatcher).async {
            watches.forEach { it.close() }
            recorder.stop() // unhooks the streams, then flushes and closes the output
            recorder.recordsWritten()
        }
    }

    init {
        // The end of a device's events is the end of the device, closed or lost; it is told on the
        // reader thread, or here and now for a sensor that ended while the recording started.
        for (sensor in sensors) {
            watches += sensor.device.events().on({ }, null) { if (open.decrementAndGet() == 0) close() }
        }
    }

    /** The records written so far; after the end, the records of the file. */
    val recordsWritten: Long get() = recorder.recordsWritten()

    /** True once the recording ended: by [close], or because every recorded sensor is closed or lost. */
    val isClosed: Boolean get() = closed.get()

    /**
     * Ends the recording: nothing more is written, the output is flushed and closed. Returns at
     * once — the work is done on [Dispatchers.IO] — so it may be called from the main thread;
     * calling it again does nothing. The sensors go on streaming.
     */
    override fun close() {
        closed.set(true)
        closing.start()
    }

    /**
     * [close], and suspends until the output is closed: the file is complete and can be read,
     * shared or uploaded.
     *
     * @return the records of the file
     * @throws java.io.UncheckedIOException the output could not be flushed or closed: the file may
     *   be cut short. The recording is ended all the same.
     */
    @JvmSynthetic
    suspend fun closeAndWait(): Long {
        close()
        return closing.await()
    }

    /**
     * [closeAndWait] for a program without coroutines: returns at once, and [callback] gets on
     * [executor] `onResult` with the records of the file once the output is closed, or `onError`
     * when it could not be flushed or closed.
     *
     * @param executor where [callback] is called; the main thread when omitted
     * @return the handle that gives the answer up ([Cancellable.cancel]): [callback] is not
     *   called; the recording ends all the same
     */
    @JvmOverloads
    fun close(callback: DepzCallback<Long>, executor: Executor = MainExecutor): Cancellable {
        close()
        return CALLS.answer(callback, executor) { closing.await() }
    }

    companion object {
        /**
         * Starts recording [sensors] into [output] as one dataset on one host timeline: each
         * sensor's clock is synchronised with this device's (the Java SDK's `syncTime`, five
         * exchanges with the board, while it streams or not), the file's header is written, and
         * from then on every item of every sensor's stream is a record — `sr04`, `vl53l4`,
         * `vl53lx`, `vl53l8` (and `vl53l8_cnh`), `bno055`, `bno086`, `tmf8829` — under the device
         * id `d0`, `d1`, … in the order of [sensors]. A [GenericSensor] has no stream to record.
         *
         * Suspends until the recording runs. On a failure — a sensor already closed or lost, a
         * board that does not answer, an output that cannot be written — and on cancellation,
         * nothing records and [output] is closed.
         *
         * @param output where the dataset goes; the recording closes it ([DepzRecording])
         * @param note free text for the file's header
         * @param vl53l8Layers the records of a VL53L5/L7/L8 also carry the signal, ambient, sigma
         *   and reflectance of every zone
         * @param temperatureHz about this many `temperature` records per sensor and second — the
         *   board's MCU thermometer, asked on a thread of the recorder, and the sensor's die where
         *   its frames carry one; `null` for none
         * @throws IllegalArgumentException [sensors] is empty
         */
        @JvmSynthetic
        suspend fun start(
            output: OutputStream,
            sensors: List<DepzSensor>,
            note: String = "",
            vl53l8Layers: Boolean = false,
            temperatureHz: Double? = DEFAULT_TEMPERATURE_HZ,
        ): DepzRecording = start(output, sensors, note, vl53l8Layers, temperatureHz, Dispatchers.IO)

        /**
         * [start] for a program without coroutines: returns at once — the boards are asked on a
         * background thread — and [callback] gets on [executor] `onResult` with the recording
         * that runs, which the program then closes, or `onError` with what the `suspend` variant
         * throws.
         *
         * @param executor where [callback] is called; the main thread when omitted
         * @return the handle that gives the recording up ([Cancellable.cancel]): [callback] is not
         *   called, nothing records and [output] is closed
         */
        @JvmStatic
        @JvmOverloads
        fun start(
            output: OutputStream,
            sensors: List<DepzSensor>,
            callback: DepzCallback<DepzRecording>,
            executor: Executor = MainExecutor,
            note: String = "",
            vl53l8Layers: Boolean = false,
            temperatureHz: Double? = DEFAULT_TEMPERATURE_HZ,
        ): Cancellable = CALLS.answer(callback, executor, discard = { it.close() }) {
            start(output, sensors, note, vl53l8Layers, temperatureHz, Dispatchers.IO)
        }

        /** The `temperature` records per sensor and second when the call names no rate: 1. */
        const val DEFAULT_TEMPERATURE_HZ: Double = 1.0

        internal suspend fun start(
            output: OutputStream,
            sensors: List<DepzSensor>,
            note: String,
            vl53l8Layers: Boolean,
            temperatureHz: Double?,
            dispatcher: CoroutineDispatcher,
        ): DepzRecording {
            // The recorder owns the output from here: its close() closes it, started or not.
            val recorder = SessionRecorder(output, note, vl53l8Layers, temperatureHz)
            try {
                withContext(dispatcher) {
                    require(sensors.isNotEmpty()) { "no sensor to record" }
                    for (sensor in sensors) recorder.add(sensor.device)
                    recorder.start()
                }
            } catch (e: Throwable) {
                // Failed, or cancelled — also when the recorder had started meanwhile: nobody gets it.
                withContext(NonCancellable + dispatcher) {
                    try {
                        recorder.close()
                    } catch (closeFailure: Exception) {
                        e.addSuppressed(closeFailure)
                    }
                }
                throw e
            }
            return DepzRecording(recorder, sensors, dispatcher)
        }
    }
}
