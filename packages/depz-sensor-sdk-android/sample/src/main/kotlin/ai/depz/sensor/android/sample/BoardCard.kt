package ai.depz.sensor.android.sample

import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.LinearLayout
import android.widget.Spinner
import android.widget.TextView

/**
 * One board on the screen: its name and USB identity, the Connect / Disconnect button, the state
 * (an error in red), the Record button and where the recording goes, the settings of its family
 * and the live view — a distance as a number, the
 * zones as a [HeatmapView], an IMU's orientation and acceleration as text.
 */
internal class BoardCard(context: Context) : LinearLayout(context) {
    /** What the button does next. */
    enum class Action { CONNECT, DISCONNECT, CLOSING }

    private val density = resources.displayMetrics.density
    private val title = text(18f).apply { setTypeface(typeface, Typeface.BOLD) }
    private val identity = text(12f)
    private val button = Button(context)
    private val state = text(14f)
    private val record = Button(context)
    private val recorded = text(12f)
    private val settings = LinearLayout(context).apply { gravity = Gravity.CENTER_VERTICAL }
    private val number = text(40f)
    private val heatmap = HeatmapView(context)
    private val lines = text(15f).apply { typeface = Typeface.MONOSPACE }
    private val note = text(12f)
    private val stateColors = state.textColors

    /** Called with the action the button showed when it was pressed. */
    var onAction: (Action) -> Unit = {}

    private var action = Action.CONNECT

    /** Called when the Record / Stop recording button is pressed. */
    var onRecord: () -> Unit = {}

    init {
        orientation = VERTICAL
        val pad = dp(12)
        setPadding(pad, pad, pad, pad)
        background = GradientDrawable().apply {
            cornerRadius = 8 * density
            setColor(0x22808080) // a tint that reads on the light and the dark theme
        }
        val head = LinearLayout(context).apply { gravity = Gravity.CENTER_VERTICAL }
        val names = LinearLayout(context).apply { orientation = VERTICAL }
        names.addView(title)
        names.addView(identity)
        head.addView(names, LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f))
        head.addView(button)
        addView(head)
        addView(state)
        val recording = LinearLayout(context).apply { gravity = Gravity.CENTER_VERTICAL }
        recording.addView(record)
        recording.addView(recorded, LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f).apply { marginStart = dp(8) })
        addView(recording)
        addView(settings)
        addView(number)
        addView(heatmap, LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply { topMargin = dp(8) })
        addView(lines)
        addView(note)
        button.setOnClickListener { onAction(action) }
        record.setOnClickListener { onRecord() }
        showRecord(offered = false, recording = false)
        setAction(Action.CONNECT)
        clearLive()
    }

    fun setNames(title: String, identity: String) {
        this.title.text = title
        this.identity.text = identity
    }

    fun setAction(action: Action) {
        this.action = action
        button.isEnabled = action != Action.CLOSING
        button.text = when (action) {
            Action.CONNECT -> "Connect"
            Action.DISCONNECT -> "Disconnect"
            Action.CLOSING -> "Closing…"
        }
    }

    /** The state line; [error] paints it red. */
    fun setState(text: String, error: Boolean) {
        if (state.text.toString() != text) state.text = text
        if (error) state.setTextColor(Color.rgb(211, 47, 47)) else state.setTextColor(stateColors)
    }

    /**
     * The Record button: [offered] while the board streams, and saying Stop while it is
     * [recording]; null leaves the button disabled — a recording is being started or saved.
     */
    fun showRecord(offered: Boolean, recording: Boolean?) {
        record.visibility = if (offered) VISIBLE else GONE
        record.isEnabled = recording != null
        if (recording != null) record.text = if (recording) "Stop recording" else "Record"
    }

    /** Where the recording goes or went (the file's path), or what failed; empty hides the line. */
    fun setRecorded(text: String) {
        if (recorded.text.toString() != text) recorded.text = text
        recorded.visibility = if (text.isEmpty()) GONE else VISIBLE
    }

    /** The family's settings as drop-down lists; [onChange] after the user picked another choice. */
    fun showSettings(list: List<Setting<*>>, onChange: () -> Unit) {
        settings.removeAllViews()
        for (setting in list) {
            settings.addView(text(14f).apply { text = setting.label })
            val spinner = Spinner(context)
            spinner.adapter = ArrayAdapter(context, android.R.layout.simple_spinner_item, setting.choices.map { it.first })
                .apply { setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item) }
            spinner.setSelection(setting.selected)
            spinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                    if (position == setting.selected) return // the list's own first call
                    setting.selected = position
                    onChange()
                }

                override fun onNothingSelected(parent: AdapterView<*>?) = Unit
            }
            settings.addView(spinner, LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT).apply { marginEnd = dp(12) })
        }
        settings.visibility = if (list.isEmpty()) GONE else VISIBLE
    }

    /** The settings can be changed only while the sensor streams. */
    fun enableSettings(enabled: Boolean) {
        for (i in 0 until settings.childCount) settings.getChildAt(i).isEnabled = enabled
    }

    fun show(reading: Reading) {
        when (reading) {
            is Reading.Distance -> {
                number.text = distanceText(reading.mm)
                note.text = reading.note
                only(number)
            }
            is Reading.Grid -> {
                heatmap.show(reading)
                note.text = reading.note
                only(heatmap)
            }
            is Reading.Imu -> {
                lines.text = "Orientation\n${reading.orientation}\nAcceleration\n${reading.acceleration}"
                only(lines)
            }
        }
    }

    /** Hides the settings and the live view: the board is not connected. */
    fun clearLive() {
        settings.removeAllViews()
        settings.visibility = GONE
        only(null)
    }

    private fun only(shown: View?) {
        for (view in listOf(number, heatmap, lines)) view.visibility = if (view === shown) VISIBLE else GONE
        note.visibility = if (shown == null || shown === lines) GONE else VISIBLE
    }

    private fun text(sizeSp: Float) = TextView(context).apply { textSize = sizeSp }

    private fun dp(value: Int) = (value * density).toInt()
}
