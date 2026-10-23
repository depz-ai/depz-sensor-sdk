package ai.depz.sensor.sensors.vl53l4;

/** The distance-window interrupt: low and high distance, mm, and the window
 *  ({@link Vl53l4Uld#WINDOW_BELOW} … {@link Vl53l4Uld#WINDOW_IN}). */
public record DetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {}
