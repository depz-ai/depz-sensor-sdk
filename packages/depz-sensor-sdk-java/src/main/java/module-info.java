/**
 * The DEPZ sensor SDK as a Java module. Exported — the public API the 1.0 freeze covers: the live
 * devices and discovery ({@code devices}), the wire protocol ({@code transport}, {@code protocol},
 * {@code usb}), the per-sensor codecs and drivers ({@code sensors.*}), datasets ({@code dataset}) and
 * the JSON helper ({@code json}). Not exported: {@code sensors.vl53lx.bare} (the 1:1 port of ST's
 * Bare Driver, rewritten whenever ST ships a new one) and {@code internal} (SDK plumbing). The ST ULD
 * ports that stay in exported packages ({@code Vl53l4Uld}, {@code Vl53l8Uld}, the {@code *Driver}
 * classes behind the {@code uld()} / {@code driver()} escape hatches) are unstable — not covered by the
 * 1.0 freeze — and say so. On the classpath, and under Android's D8 (which ignores module
 * descriptors), the descriptor has no effect. Every exported package is {@code @NullMarked}: its types
 * are non-null unless they say {@code @Nullable}; JSpecify is annotations only, so it is a compile-time
 * requirement, and {@code transitive} shows the annotations to the code compiled against this module.
 */
module ai.depz.sensor {
    requires java.logging;
    requires com.fazecast.jSerialComm;
    requires static transitive org.jspecify;

    exports ai.depz.sensor.dataset;
    exports ai.depz.sensor.devices;
    exports ai.depz.sensor.json;
    exports ai.depz.sensor.protocol;
    exports ai.depz.sensor.sensors.bno055;
    exports ai.depz.sensor.sensors.bno086;
    exports ai.depz.sensor.sensors.tmf8829;
    exports ai.depz.sensor.sensors.vl53l4;
    exports ai.depz.sensor.sensors.vl53l7;
    exports ai.depz.sensor.sensors.vl53l8;
    exports ai.depz.sensor.sensors.vl53lx;
    exports ai.depz.sensor.transport;
    exports ai.depz.sensor.usb;
}
