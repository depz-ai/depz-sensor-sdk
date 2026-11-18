package ai.depz.sensor.android

import ai.depz.sensor.devices.SensorStream
import java.util.function.Predicate
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.callbackFlow

/**
 * A Java SDK stream as a cold [Flow]: every collection is a subscription of its own
 * (`SensorStream.on(callback, where, onEnd)`), closed when the collection ends for any reason.
 *
 * The device's reader thread only puts an item into the collection's buffer — [capacity] items,
 * the oldest dropped when the collector is slower than the sensor — so a slow collector never
 * holds the reader. The flow completes normally after the program's own `close()` and fails with
 * the device's `DeviceLostException` (its `cause` is what the link threw) after a loss; the items
 * still in the buffer are delivered first.
 */
internal fun <T : Any> SensorStream<T>.asFlow(capacity: Int, where: Predicate<T>? = null): Flow<T> =
    callbackFlow {
        val subscription = on({ trySend(it) }, where) { loss -> close(loss) }
        awaitClose { subscription.close() }
    }.buffer(capacity, BufferOverflow.DROP_OLDEST)
