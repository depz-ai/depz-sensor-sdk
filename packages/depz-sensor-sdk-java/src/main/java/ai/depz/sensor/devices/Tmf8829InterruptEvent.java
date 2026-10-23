package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Tmf8829;

/**
 * RPT_TMF_EVENT that is not an expected stop artefact: a MOTION / PROXIMITY interrupt (code 0) or a
 * refused frame header (code 1 — the sensor is lost, run {@link Tmf8829Device#init} again).
 */
public record Tmf8829InterruptEvent(Tmf8829.Tmf8829Event event) implements DeviceEvent {}
