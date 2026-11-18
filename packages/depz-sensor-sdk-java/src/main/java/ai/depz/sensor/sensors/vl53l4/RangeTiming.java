package ai.depz.sensor.sensors.vl53l4;

/** A ranging timing: the budget of one measurement and the period between two (0 = continuous), ms. */
public record RangeTiming(int timingBudgetMs, int interMeasurementMs) {
    /** Build one: the path that survives a component appended after 1.0 (contract 07 §5). */
    public static RangeTiming of(int timingBudgetMs, int interMeasurementMs) {
        return new RangeTiming(timingBudgetMs, interMeasurementMs);
    }
}
