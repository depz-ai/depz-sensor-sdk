package ai.depz.sensor.android.sample

import java.util.concurrent.ConcurrentHashMap
import kotlinx.coroutines.sync.Mutex

/**
 * The turns of the connections of each board, for the whole process. A connection outlives the
 * activity that started it by the time it takes to stop and close its board, and Android may
 * create the next activity at once — a configuration change the manifest does not take (the dark
 * theme, the font size), a launch right after Back. The new activity's connection of a board
 * waits here until the earlier one released the board: opened before that, the board is still
 * held and the opening fails.
 */
internal class Connections {
    private val turns = ConcurrentHashMap<String, Mutex>()

    /**
     * Runs [connection] — from the opening of the board to its release — as the connection of the
     * board Android names [deviceName], once every earlier connection of that board ended.
     * [waiting] is called first when there is one to wait for. Cancelling while it waits gives
     * the turn up.
     */
    suspend fun <T> hold(deviceName: String, waiting: () -> Unit = {}, connection: suspend () -> T): T {
        val turn = turns.computeIfAbsent(deviceName) { Mutex() }
        if (!turn.tryLock()) {
            waiting()
            turn.lock()
        }
        try {
            return connection()
        } finally {
            turn.unlock()
        }
    }
}
