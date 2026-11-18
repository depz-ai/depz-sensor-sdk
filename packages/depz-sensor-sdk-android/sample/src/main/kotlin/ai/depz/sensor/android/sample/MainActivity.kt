package ai.depz.sensor.android.sample

import ai.depz.sensor.android.DepzAndroid
import ai.depz.sensor.android.DepzRecording
import ai.depz.sensor.android.DepzSensor
import ai.depz.sensor.android.DepzUsbDevice
import ai.depz.sensor.android.GenericSensor
import ai.depz.sensor.android.UsbPermissionDeniedException
import ai.depz.sensor.devices.DeviceLostException
import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.os.SystemClock
import android.util.Log
import android.view.Gravity
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.io.File
import java.time.Duration
import java.time.LocalDateTime
import java.util.concurrent.atomic.AtomicLong
import kotlin.math.roundToLong
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

private const val TAG = "DEPZ"
private const val DRAW_MS = 100L
private const val RATE_MS = 1000L
private const val LOG_MS = 5000L

/** One attached board: its card, and its connection while it has one. */
private class Board(var device: DepzUsbDevice, val card: BoardCard) {
    /** The board's name in the log (`RATE` lines) and on the card. */
    val title: String get() = device.productName ?: device.deviceName

    /** The connection: permission, opening, streaming, closing. Not active: the board is not connected. */
    var job: Job? = null
    @Volatile var status = "not connected"
    @Volatile var error = false
    @Volatile var live: Live? = null

    /** The latest item of the stream, written by its collector. */
    @Volatile var latest: Reading? = null
    val count = AtomicLong()
    var countBefore = 0L
    var rate = 0L

    /** The user picked another choice of a setting: apply it. */
    val changes = Channel<Unit>(Channel.CONFLATED)

    /** The open sensor while the board streams: what the Record button records. */
    var sensor: DepzSensor? = null

    /** The recording under way and its file. */
    var recording: DepzRecording? = null
    var recordingFile: File? = null

    /** A recording is being started or saved: the Record button waits. */
    var recordBusy = false

    /** The line under the Record button once nothing records: where the last file is, or what failed. */
    var recorded = ""
}

/**
 * The sample app of the DEPZ Android SDK: the attached DEPZ boards as a list of cards. Each card
 * connects its board (the USB permission is asked first), offers the settings of its sensor
 * family and shows the measurements live — a distance, the zones of a multizone ToF sensor as a
 * heatmap, an IMU's orientation and acceleration — and the errors: a refused permission, a lost
 * board. Record on a card writes the board's measurements into a dataset file (`.depzdata`, the
 * format of the desktop SDKs and the web viewer) in the app's files directory and shows its path
 * (`DepzSensor.record`). Several boards run at once, and the list follows the boards plugged in and unplugged
 * while the app runs (`DepzAndroid.devices`). All of it is the SDK's Kotlin API called from the main
 * thread: `DepzAndroid.discover` / `open`, the `suspend` functions of the sensor and its `Flow`.
 *
 * At launch every attached board is connected. For runs from adb the launch takes extras, and the
 * log (tag `DEPZ`) gets a `RATE <board> | <state> | n=<items> | <items>/s | <detail>` line per
 * board every 5 s:
 *
 *     adb shell am start -n ai.depz.sensor.android.sample/.MainActivity \
 *         [--ez connect false]   # leave the boards to their Connect buttons
 *         [--ei zones 16|64]     # VL53L5/L7/L8: 4×4 or 8×8 (default 64)
 *         [--ei hz N]            # VL53L5/L7/L8: frames per second (default 15)
 *         [--ez reset true]      # VL53L4CD / 1D ToF: pulse XSHUT before the init
 *         [--ez nostop true]     # close the boards without stopping their sensors
 *         [--ez record true]     # record every board from the moment it streams (the log tells the files)
 *         [--ez finish true]     # close the running app
 */
class MainActivity : Activity() {
    private lateinit var cards: LinearLayout
    private lateinit var summary: TextView
    /** Everything the screen runs, on the main thread: the SDK's `suspend` functions do not block it. */
    private var scope: CoroutineScope? = null
    private lateinit var options: Launch
    /** The attached boards by Android's device name, in the order of discovery. */
    private val boards = LinkedHashMap<String, Board>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // `am start … --ez finish true` when the app is not running: nothing to close, don't start.
        if (intent.getBooleanExtra("finish", false)) {
            finish()
            return
        }
        options = Launch(
            zones = intent.getIntExtra("zones", 64),
            hz = intent.getIntExtra("hz", 15),
            resetFirst = intent.getBooleanExtra("reset", false),
            stopOnClose = !intent.getBooleanExtra("nostop", false),
            record = intent.getBooleanExtra("record", false),
        )
        setContentView(screen())
        // The main thread as a dispatcher, from Android's own executor: kotlinx-coroutines-core has
        // no Dispatchers.Main without the kotlinx-coroutines-android artifact, which the app does not need.
        val scope = CoroutineScope(SupervisorJob() + mainExecutor.asCoroutineDispatcher()).also { this.scope = it }
        rescan()
        Log.i(TAG, "found ${boards.size} DEPZ device(s)")
        // Every board asks at once; the SDK shows the permission dialogs one after another.
        if (intent.getBooleanExtra("connect", true)) boards.values.forEach(::connect)
        scope.launch { refresh() }
        // Hot-plug: a board plugged in while the app runs gets its card, an unplugged one loses it.
        scope.launch {
            DepzAndroid.devices(this@MainActivity).collect { attached ->
                Log.i(TAG, "USB: ${attached.size} DEPZ device(s) attached")
                rescan()
            }
        }
    }

    /** `am start … --ez finish true` closes the app the way a user's exit does. */
    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        if (intent.getBooleanExtra("finish", false)) finish()
    }

    private fun screen(): ScrollView {
        val pad = dp(12)
        summary = TextView(this).apply { textSize = 16f }
        val rescan = Button(this).apply {
            text = "Rescan"
            setOnClickListener { rescan() }
        }
        val head = LinearLayout(this).apply { gravity = Gravity.CENTER_VERTICAL }
        head.addView(summary, LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f))
        head.addView(rescan)
        cards = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        val column = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
        }
        column.addView(head)
        column.addView(cards)
        return ScrollView(this).apply {
            fitsSystemWindows = true // Android 15+ draws the app under the system bars
            addView(column)
        }
    }

    /**
     * Brings the list to the boards attached now: a new board gets a card, a board that is gone
     * and not connected loses its card, and a board whose permission was granted meanwhile shows
     * its serial number.
     */
    private fun rescan() {
        val attached = DepzAndroid.discover(this).associateBy { it.deviceName }
        boards.entries.removeAll { (name, board) ->
            (name !in attached && board.job?.isActive != true).also { gone -> if (gone) cards.removeView(board.card) }
        }
        for ((name, device) in attached) {
            val board = boards.getOrPut(name) {
                val card = BoardCard(this)
                Board(device, card).also { board ->
                    card.onAction = { action ->
                        if (action == BoardCard.Action.CONNECT) connect(board) else disconnect(board)
                    }
                    card.onRecord = { scope?.launch { if (board.recording == null) record(board) else save(board) } }
                    cards.addView(card, LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT,
                    ).apply { topMargin = dp(12) })
                }
            }
            board.device = device
            board.card.setNames(board.title, listOfNotNull(
                device.serial?.let { "serial $it" },
                "USB %04X:%04X".format(device.vendorId, device.productId),
                device.modelHint,
            ).joinToString(" · "))
        }
        summary.text = when (boards.size) {
            0 -> "No DEPZ sensor connected"
            1 -> "1 DEPZ board"
            else -> "${boards.size} DEPZ boards"
        }
    }

    private fun connect(board: Board) {
        val scope = this.scope ?: return
        if (board.job?.isActive == true) return
        board.card.setAction(BoardCard.Action.DISCONNECT)
        board.error = false
        board.job = scope.launch {
            try {
                // Not before the connection an earlier activity left closing released the board.
                connections.hold(
                    board.device.deviceName,
                    waiting = { board.status = "waiting for the previous connection to close…" },
                ) { session(board) }
            } catch (e: CancellationException) {
                board.status = "not connected" // Disconnect, or the app is closing
                throw e
            } catch (e: UsbPermissionDeniedException) {
                fail(board, "USB permission denied")
            } catch (e: DeviceLostException) {
                fail(board, "disconnected: ${e.cause?.message ?: e.message}")
                Log.i(TAG, "${board.title}: ${board.status}")
            } catch (e: Exception) {
                Log.e(TAG, "${board.title}: failed", e)
                fail(board, "error: ${e.message ?: e}")
            } finally {
                board.live = null
                board.card.clearLive()
                board.card.setAction(BoardCard.Action.CONNECT)
            }
        }
    }

    /** Ends the board's connection: its sensor is stopped and closed before the button offers Connect again. */
    private fun disconnect(board: Board) {
        board.card.setAction(BoardCard.Action.CLOSING)
        board.status = "closing…"
        board.job?.cancel()
    }

    private fun fail(board: Board, what: String) {
        board.status = what
        board.error = true
    }

    /** One connection of [board], from the permission to the loss or the cancellation that ends it. */
    private suspend fun session(board: Board) {
        board.status = if (DepzAndroid.hasPermission(this, board.device)) "opening…" else "waiting for USB permission"
        val sensor = DepzAndroid.open(this, board.device, Duration.ofSeconds(1))
        var live: Live? = null
        try {
            rescan() // with the permission the board's serial number is readable
            val name = sensor.device.javaClass.simpleName
            board.status = "starting $name…"
            Log.i(TAG, "${board.title}: opened as $name")
            live = liveOf(sensor, options) { Log.i(TAG, "${board.title}: $it") }
            live.prepare()
            board.card.showSettings(live.settings) { board.changes.trySend(Unit) }
            board.changes.tryReceive() // a choice made on an earlier connection is already in the settings
            board.latest = null
            coroutineScope {
                // The stream is collected off the main thread; the screen draws its latest item.
                launch(Dispatchers.Default) {
                    live.readings.collect { reading ->
                        board.count.incrementAndGet()
                        board.latest = reading
                    }
                }
                live.start()
                board.live = live
                board.sensor = sensor
                board.status = if (sensor is GenericSensor) "no stream for $name" else name
                Log.i(TAG, "${board.title}: streaming")
                if (options.record) record(board)
                // Until the board is lost (the stream fails) or the connection is cancelled.
                for (change in board.changes) {
                    board.card.enableSettings(false)
                    board.status = "applying the settings…"
                    live.stop()
                    live.start()
                    board.status = name
                    board.card.enableSettings(true)
                }
            }
        } finally {
            withContext(NonCancellable) { close(board, sensor, live) }
        }
    }

    /**
     * Starts recording [board] into a new file of the app's files directory. The sensor is not
     * touched: it streams on, and every item from here on is a record of the file.
     */
    private suspend fun record(board: Board) {
        val sensor = board.sensor ?: return
        if (board.recording != null || board.recordBusy) return
        board.recordBusy = true
        try {
            val file = File(filesDir, recordingFileName(board.title, board.device.serial ?: board.device.deviceName, LocalDateTime.now()))
            // No file is left of a recording that did not start (a failure, a cancellation).
            board.recording = recordInto(file) { output ->
                sensor.record(output, note = "DEPZ Android sample · ${board.title}")
            }
            board.recordingFile = file
            Log.i(TAG, "${board.title}: recording to ${file.path}")
        } catch (e: CancellationException) {
            throw e
        } catch (e: Exception) {
            Log.e(TAG, "${board.title}: recording failed", e)
            board.recorded = "recording failed: ${e.message ?: e}"
        } finally {
            board.recordBusy = false
        }
    }

    /**
     * Ends the recording of [board], if one runs, and says where the file is. Also after the
     * board was closed or lost: the recording ended with it, and its file is complete.
     */
    private suspend fun save(board: Board) {
        val recording = board.recording ?: return
        if (board.recordBusy) return
        board.recordBusy = true
        val path = board.recordingFile?.path
        try {
            val records = recording.closeAndWait()
            board.recorded = "saved $records records: $path"
            Log.i(TAG, "${board.title}: recorded $records records to $path")
        } catch (e: Exception) {
            Log.e(TAG, "${board.title}: saving $path failed", e)
            board.recorded = "saving $path failed: ${e.message ?: e}"
        } finally {
            board.recording = null
            board.recordingFile = null
            board.recordBusy = false
        }
    }

    /**
     * Stops the sensor — unless the board is lost or the launch said `nostop` — saves its
     * recording and releases the board.
     */
    private suspend fun close(board: Board, sensor: DepzSensor, live: Live?) {
        board.sensor = null
        if (live != null && options.stopOnClose && sensor.lost == null) {
            runCatching { live.stop() }.onFailure { Log.e(TAG, "${board.title}: stop failed", it) }
        }
        save(board)
        runCatching { sensor.closeAndWait() }.onFailure { Log.e(TAG, "${board.title}: close failed", it) }
    }

    /** Draws the latest items ten times a second, counts the rates every second, logs them every five. */
    private suspend fun refresh() {
        var ratesAt = SystemClock.elapsedRealtime()
        // The first lines a second after the launch: a script granting the USB permission reads them.
        var loggedAt = ratesAt - (LOG_MS - RATE_MS)
        while (true) {
            val now = SystemClock.elapsedRealtime()
            if (now - ratesAt >= RATE_MS) {
                for (board in boards.values) {
                    val n = board.count.get()
                    board.rate = ((n - board.countBefore) * 1000.0 / (now - ratesAt)).roundToLong()
                    board.countBefore = n
                }
                ratesAt = now
            }
            for (board in boards.values) {
                val detail = board.live?.detail?.invoke().orEmpty()
                val counted = if (board.live != null) "  ·  ${board.count.get()}  ·  ${board.rate}/s" else ""
                board.card.setState(board.status + counted + if (detail.isEmpty()) "" else "\n$detail", board.error)
                if (board.live != null) board.latest?.let(board.card::show)
                val recording = board.recording
                board.card.showRecord(
                    offered = board.sensor != null || recording != null,
                    recording = if (board.recordBusy) null else recording != null,
                )
                board.card.setRecorded(
                    if (recording != null) "${recording.recordsWritten} records → ${board.recordingFile?.path}" else board.recorded,
                )
            }
            if (now - loggedAt >= LOG_MS) {
                loggedAt = now
                for (board in boards.values) {
                    Log.i(TAG, "RATE ${board.title} | ${board.status} | n=${board.count.get()} | ${board.rate}/s | " +
                        board.live?.detail?.invoke().orEmpty())
                }
            }
            delay(DRAW_MS)
        }
    }

    private fun dp(value: Int) = (value * resources.displayMetrics.density).toInt()

    override fun onDestroy() {
        // Every connection is cancelled; each still stops and closes its board (session's finally).
        scope?.cancel()
        super.onDestroy()
    }

    private companion object {
        /** Of the process, not of the activity: the next activity's connections wait for this one's. */
        val connections = Connections()
    }
}
