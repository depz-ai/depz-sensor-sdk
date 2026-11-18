package ai.depz.sensor.android.sample

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.view.View
import kotlin.math.min

/**
 * The zones of a multizone ToF sensor as a grid of coloured cells: the distance as a colour
 * ([heatColor]: near red, far blue) and, where a cell is large enough, as a number in mm. A zone
 * without a valid distance is a grey cell with a cross.
 */
internal class HeatmapView(context: Context) : View(context) {
    private var grid: Reading.Grid? = null
    private val density = resources.displayMetrics.density
    private val fill = Paint()
    private val cross = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.rgb(150, 150, 150)
        strokeWidth = density
    }
    private val label = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        textAlign = Paint.Align.CENTER
        color = Color.BLACK
    }

    fun show(grid: Reading.Grid) {
        val reshaped = this.grid?.let { it.rows != grid.rows || it.cols != grid.cols } ?: true
        this.grid = grid
        if (reshaped) requestLayout()
        invalidate()
    }

    /** As wide as the card lets it, up to 360 dp; as high as the grid's shape makes it. */
    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = min(MeasureSpec.getSize(widthMeasureSpec), (360 * density).toInt())
        val shape = grid?.takeIf { it.cols > 0 }
        setMeasuredDimension(width, if (shape == null) 0 else width * shape.rows / shape.cols)
    }

    override fun onDraw(canvas: Canvas) {
        val grid = this.grid ?: return
        if (grid.cols == 0) return
        val cell = width.toFloat() / grid.cols
        val gap = if (cell > 6 * density) density else 0f
        label.textSize = cell * 0.32f
        val numbers = label.textSize >= 8 * density // smaller than that nobody reads
        val baseline = (label.descent() + label.ascent()) / 2
        for (i in grid.mm.indices) {
            val left = (i % grid.cols) * cell
            val top = (i / grid.cols) * cell
            val mm = grid.mm[i]
            fill.color = if (mm.isNaN()) INVALID else heatColor(mm)
            canvas.drawRect(left + gap, top + gap, left + cell - gap, top + cell - gap, fill)
            if (mm.isNaN()) {
                val inset = cell * 0.35f
                canvas.drawLine(left + inset, top + inset, left + cell - inset, top + cell - inset, cross)
                canvas.drawLine(left + cell - inset, top + inset, left + inset, top + cell - inset, cross)
            } else if (numbers) {
                canvas.drawText(mm.toInt().toString(), left + cell / 2, top + cell / 2 - baseline, label)
            }
        }
    }

    private companion object {
        val INVALID = Color.rgb(60, 60, 60)
    }
}
