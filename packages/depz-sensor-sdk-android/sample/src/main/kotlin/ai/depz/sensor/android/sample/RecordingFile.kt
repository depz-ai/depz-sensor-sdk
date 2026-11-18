package ai.depz.sensor.android.sample

import java.io.File
import java.io.OutputStream
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
import java.util.concurrent.atomic.AtomicReference
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext

private val STAMP = DateTimeFormatter.ofPattern("yyyyMMdd-HHmmss")

/**
 * The name of a board's recording started [at]: the board's name and what tells it from another
 * board of its model ([id]: the USB serial, or Android's device name), as a file name, then the
 * time — `DEPZ_VL53L8CX-3577-20261110-231501.depzdata`.
 */
internal fun recordingFileName(board: String, id: String, at: LocalDateTime): String =
    "${fileNamePart(board)}-${fileNamePart(id.substringAfterLast('/'))}-${STAMP.format(at)}.depzdata"

/** [text] with every run of characters a file name should not hold as one `_`, and no dot or `_` at its ends. */
private fun fileNamePart(text: String): String =
    text.replace(Regex("[^A-Za-z0-9.]+"), "_").trim('_', '.').ifEmpty { "board" }

/**
 * Opens [file] and hands its stream to [start], which starts the recording into it and whose
 * result this returns: the stream is the recording's from then on. A recording that did not
 * start — [start] failed, or the call was cancelled, be it while the file was being opened —
 * leaves nothing behind: the stream is closed and the file, empty or cut short, is deleted.
 * Opening and deleting are disk work, done off the caller's thread.
 */
internal suspend fun <T> recordInto(
    file: File,
    open: (File) -> OutputStream = { it.outputStream() },
    start: suspend (OutputStream) -> T,
): T {
    // Set on the disk thread, also when the caller was cancelled meanwhile and never gets the stream.
    val output = AtomicReference<OutputStream>()
    try {
        withContext(Dispatchers.IO) { output.set(open(file)) }
        return start(output.get())
    } catch (e: Throwable) {
        withContext(NonCancellable + Dispatchers.IO) {
            runCatching { output.get()?.close() }
            file.delete()
        }
        throw e
    }
}
