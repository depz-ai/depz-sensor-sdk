package ai.depz.sensor.sensors.vl53l4;

/** The distance-window interrupt: low and high distance, mm, and the window
 *  ({@link Vl53l4Uld#WINDOW_BELOW} … {@link Vl53l4Uld#WINDOW_IN}). */
public record DetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {
    /** Build one: the path that survives a component appended after 1.0 (contract 07 §5). */
    public static DetectionThresholds of(int distanceLowMm, int distanceHighMm, int window) {
        return new DetectionThresholds(distanceLowMm, distanceHighMm, window);
    }
}
