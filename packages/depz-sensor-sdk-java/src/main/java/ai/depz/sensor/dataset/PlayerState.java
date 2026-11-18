package ai.depz.sensor.dataset;

/** {@link DatasetPlayer} state (contract 09 §Player). */
public enum PlayerState {
    /** Never played. */
    IDLE,

    /** Delivering records paced by the clock. */
    PLAYING,

    /** Stopped mid-timeline; {@link DatasetPlayer#play()} resumes. */
    PAUSED,

    /** Past the last record; {@link DatasetPlayer#play()} restarts from the top. */
    DONE,
}
