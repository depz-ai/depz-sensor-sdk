package ai.depz.sensor.sensors.vl53l4;

/** A ranging timing: the budget of one measurement and the period between two (0 = continuous), ms. */
public record RangeTiming(int timingBudgetMs, int interMeasurementMs) {}
