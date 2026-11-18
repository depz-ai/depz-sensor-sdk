package ai.depz.sensor.android

import ai.depz.sensor.devices.DeviceLostException
import ai.depz.sensor.devices.SensorStream
import ai.depz.sensor.devices.Unsubscribe
import android.os.Handler
import android.os.Looper
import java.util.concurrent.Executor
import java.util.concurrent.RejectedExecutionException
import java.util.concurrent.atomic.AtomicInteger
import java.util.function.Predicate
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch

/**
 * The handle of a call that answers through a callback — [DepzAndroid.requestPermission],
 * [DepzAndroid.open], [DepzSensor.record], [DepzSensor.close], a stream of a [DepzSensor]
 * subscribed with a [StreamListener].
 */
fun interface Cancellable {
    /**
     * Stops the call: its callback or listener is not called from here on (a call already running
     * on its executor finishes). A stream's subscription is closed, a permission request stops
     * listening for the answer, a board being opened is closed. Idempotent; any thread.
     */
    fun cancel()
}

/**
 * The outcome of a call that waits — [DepzAndroid.requestPermission], [DepzAndroid.open],
 * [DepzSensor.record], [DepzSensor.close], [DepzRecording.close] — for a program without
 * coroutines: exactly one of the two methods is called, once, on the call's executor — or neither
 * after [Cancellable.cancel].
 */
interface DepzCallback<T> {
    /** The call succeeded with [value]. */
    fun onResult(value: T)

    /** The call failed with [error]: what the `suspend` variant of the call throws. */
    fun onError(error: Exception)
}

/**
 * The receiver of a [DepzSensor] stream for a program without coroutines. Every method is called
 * on the subscription's executor, one call at a time, in the stream's order: [onItem] for each
 * item, then at most one of [onError] and [onEnd], after which nothing more. After
 * [Cancellable.cancel] none is called.
 *
 * Only [onItem] has to be written, so a Java lambda is a listener:
 * `sensor.measurements(m -> show(m))`.
 */
fun interface StreamListener<T> {
    /** The next item of the stream. */
    fun onItem(item: T)

    /**
     * The board is lost (unplugged, a USB error) and the stream is over: [error] is the Java SDK's
     * loss, its `cause` what the USB link reported, as [DepzSensor.lost] then tells too. The sensor
     * still has to be closed.
     */
    fun onError(error: DeviceLostException) {}

    /** The stream is over because the program closed the sensor. */
    fun onEnd() {}
}

/**
 * The main thread as an [Executor]: where the callbacks and listeners run when the call names no
 * executor.
 */
internal object MainExecutor : Executor {
    private val handler by lazy { Handler(Looper.getMainLooper()) }

    override fun execute(command: Runnable) {
        if (!handler.post(command)) throw RejectedExecutionException("the main looper is quitting")
    }
}

/**
 * Runs [block] in this scope — on the calling thread up to its first suspension — and hands its
 * outcome to [callback] on [executor]. The result of a call cancelled too late to stop it, or one
 * the executor refuses, goes to [discard]: nobody else will ever hold it.
 */
internal fun <T> CoroutineScope.answer(
    callback: DepzCallback<T>,
    executor: Executor,
    discard: (T) -> Unit = {},
    block: suspend () -> T,
): Cancellable {
    val state = AtomicInteger(PENDING)
    val job = launch(start = CoroutineStart.UNDISPATCHED) {
        val outcome = try {
            Result.success(block())
        } catch (e: CancellationException) {
            throw e
        } catch (e: Exception) {
            Result.failure(e)
        }
        try {
            executor.execute {
                if (state.compareAndSet(PENDING, ANSWERED)) {
                    outcome.fold(callback::onResult) { callback.onError(it as Exception) }
                } else {
                    outcome.onSuccess(discard)
                }
            }
        } catch (e: RejectedExecutionException) {
            // The app shut its executor down: it awaits no answer.
            outcome.onSuccess(discard)
        }
    }
    return Cancellable { if (state.compareAndSet(PENDING, CANCELLED)) job.cancel() }
}

/** Where the callback variants of the suspending calls wait: for the user, a board, a file. */
internal val CALLS = CoroutineScope(SupervisorJob() + Dispatchers.Default)

private const val PENDING = 0
private const val ANSWERED = 1
private const val CANCELLED = 2

/**
 * A Java SDK stream delivered to [listener] on [executor]: subscribed when this returns
 * (`SensorStream.on(callback, where, onEnd)`), so nothing emitted after that is missed.
 *
 * The device's reader thread only puts an item into a buffer of [capacity] items — the oldest
 * dropped when the listener is slower than the sensor — and asks the executor for a delivery
 * unless one is already asked for. A delivery hands over what the buffer held when it began, so
 * a busy stream leaves the executor's thread (the main thread, by default) room for other work.
 */
internal fun <T : Any> SensorStream<T>.deliver(
    listener: StreamListener<T>,
    executor: Executor,
    capacity: Int,
    where: Predicate<T>? = null,
): Cancellable = Delivery(this, listener, executor, capacity, where)

private class Delivery<T : Any>(
    stream: SensorStream<T>,
    private val listener: StreamListener<T>,
    private val executor: Executor,
    private val capacity: Int,
    where: Predicate<T>?,
) : Cancellable {
    private val lock = Any()
    private val buffer = ArrayDeque<T>()   // guarded by lock
    private var ended = false              // guarded by lock: the stream ended; `loss` tells how
    private var loss: DeviceLostException? = null
    private var scheduled = false          // guarded by lock: a delivery is asked for or running
    @Volatile private var cancelled = false

    // Null while on() runs: a stream that already ended calls end() from inside it.
    @Volatile private var subscription: Unsubscribe? = null

    init {
        require(capacity > 0) { "capacity $capacity: at least 1" }
        subscription = stream.on({ item -> offer(item) }, where) { lost -> end(lost) }
        if (cancelled) subscription?.close() // the executor refused that very end
    }

    private fun offer(item: T) {
        synchronized(lock) {
            if (buffer.size == capacity) buffer.removeFirst()
            buffer.addLast(item)
        }
        schedule()
    }

    private fun end(lost: DeviceLostException?) {
        synchronized(lock) {
            ended = true
            loss = lost
        }
        schedule()
    }

    private fun schedule() {
        synchronized(lock) {
            if (scheduled || cancelled) return
            scheduled = true
        }
        try {
            executor.execute(::run)
        } catch (e: RejectedExecutionException) {
            // Nothing can be delivered any more. Thrown on to the Java SDK's guard of the reader
            // thread, which counts and logs it.
            cancel()
            throw e
        }
    }

    /** One delivery, on the executor: the items buffered by now, then the end if it came. */
    private fun run() {
        var batch: List<T> = emptyList()
        var last = false
        synchronized(lock) {
            batch = buffer.toList()
            buffer.clear()
            last = ended
        }
        try {
            for (item in batch) {
                if (cancelled) return
                listener.onItem(item)
            }
            if (last && !cancelled) {
                cancelled = true // the stream is over: nothing more, whatever comes
                loss?.let(listener::onError) ?: listener.onEnd()
            }
        } catch (e: Throwable) {
            // A listener that throws gets nothing more; its executor's thread gets the exception.
            cancel()
            throw e
        } finally {
            val more = synchronized(lock) {
                scheduled = false
                !cancelled && (buffer.isNotEmpty() || (ended && !last))
            }
            if (more) schedule()
        }
    }

    override fun cancel() {
        cancelled = true
        subscription?.close()
        synchronized(lock) { buffer.clear() }
    }
}
