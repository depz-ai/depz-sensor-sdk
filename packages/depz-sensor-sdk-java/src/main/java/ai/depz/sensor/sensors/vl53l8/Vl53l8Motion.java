package ai.depz.sensor.sensors.vl53l8;

/** The motion-indicator block of a frame ({@code VL53L8CX_ResultsData MotionIndicator}). */
public record Vl53l8Motion(long globalIndicator1, long globalIndicator2, int status, int nbOfDetectedAggregates,
        int nbOfAggregates, long[] motion) {}
