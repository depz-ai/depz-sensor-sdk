package ai.depz.sensor.test.devices;

import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.DepzDevices;
import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.InvalidReplyException;
import ai.depz.sensor.devices.ReplayLink;
import ai.depz.sensor.devices.Sr04Device;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.json.JsonText;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.usb.Discovery;
import ai.depz.sensor.usb.UsbIds;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.RecordComponent;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Stream;

/**
 * Contract 15 device scenarios: the Java runner. Each {@code contracts/scenarios/<id>.json} names
 * a real-hardware capture and the API calls that made it. The capture is replayed causally and
 * strict-tx (every byte the SDK writes must equal the recorded tx stream); the device is opened
 * as {@code probe} says; each step's result is made canonical (§3) and matched against
 * {@code expect}. Canonical names map to Java by lowerCamelCase (an alias table holds the rest);
 * Java has no optional parameters, so a call picks the overload with as many parameters as the
 * step has arguments.
 */
public final class Scenarios {
    private static final Duration REQUEST_TIMEOUT = Duration.ofSeconds(2); // as every runner
    private static final Duration TAKE_TIMEOUT = Duration.ofSeconds(5);    // per streamed item

    // Contract 07 §1 layer 3: canonical class → Java class, for the classes ported so far.
    static final Map<String, Class<? extends DepzDevice>> CLASSES = new HashMap<>(Map.of(
            "sr04", Sr04Device.class,
            "vl53l4cd", ai.depz.sensor.devices.Vl53l4cdDevice.class,
            "vl53l8cx", ai.depz.sensor.devices.Vl53l8cxDevice.class,
            "vl53l8ch", ai.depz.sensor.devices.Vl53l8chDevice.class,
            "vl53l5cx", ai.depz.sensor.devices.Vl53l5cxDevice.class,
            "vl53l7cx", ai.depz.sensor.devices.Vl53l7cxDevice.class,
            "vl53l7ch", ai.depz.sensor.devices.Vl53l7chDevice.class,
            "bno055", ai.depz.sensor.devices.Bno055Device.class,
            "bno086", ai.depz.sensor.devices.Bno086Device.class));

    // Record arguments (§2): the Java type a record argument becomes → its builder.
    static final Map<Class<?>, Function<Map<String, Object>, Object>> RECORD_ARGS = new HashMap<>(Map.of(
            ai.depz.sensor.sensors.bno055.Bno055Regs.Units.class, r -> new ai.depz.sensor.sensors.bno055.Bno055Regs.Units(
                    (Boolean) r.get("accel_mg"), (Boolean) r.get("gyro_rps"), (Boolean) r.get("euler_rad"),
                    (Boolean) r.get("temp_f"), (Boolean) r.get("android")),
            ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup.class, r -> {
                ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup setup = new ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup();
                setup.initConfig(num(r.get("start_bin")), num(r.get("num_bins")), num(r.get("sub_sample")));
                List<?> m = (List<?>) r.get("agg_map");
                setup.createAggMap(num(r.get("resolution")), num(m.get(0)), num(m.get(1)), num(m.get(2)), num(m.get(3)),
                        num(m.get(4)), num(m.get(5)));
                return setup;
            }));

    private static int num(Object v) {
        return Math.toIntExact(((Number) v).longValue());
    }

    // Types that are plain tuples in Python (canonical form: an array of their components, in order).
    static final Set<Class<?>> TUPLE_TYPES = new java.util.HashSet<>(List.of(
            ai.depz.sensor.sensors.vl53l4.RangeTiming.class,
            ai.depz.sensor.sensors.vl53l4.DetectionThresholds.class));

    // The alias table (§2): canonical names Java spells otherwise than lowerCamelCase(name), per class.
    static final Map<String, String> ALIASES = new HashMap<>();

    // Scenarios of a ported class whose driver is not ported yet (they stay in the coverage exceptions).
    static final Map<String, String> PENDING = new HashMap<>();

    static {
        CLASSES.put("vl53lx", ai.depz.sensor.devices.Vl53lxDevice.class);
        CLASSES.put("vl53l0x", ai.depz.sensor.devices.Vl53l0xDevice.class);
        CLASSES.put("vl53l1cx", ai.depz.sensor.devices.Vl53l1cxDevice.class);
        CLASSES.put("vl53l1cb", ai.depz.sensor.devices.Vl53l1cbDevice.class);
        CLASSES.put("vl53l3cx", ai.depz.sensor.devices.Vl53l3cxDevice.class);
        CLASSES.put("vl53l4cx", ai.depz.sensor.devices.Vl53l4cxDevice.class);
    }

    private Scenarios() {}

    /** The scenario files this SDK runs, by name. */
    public static List<Path> files(Path vectorsDir) throws Exception {
        Path dir = vectorsDir.resolve("../scenarios").normalize();
        List<Path> out = new ArrayList<>();
        try (Stream<Path> s = Files.list(dir)) {
            for (Path p : s.filter(x -> x.toString().endsWith(".json")).sorted().toList()) {
                Map<String, Object> sc = load(p);
                if (CLASSES.containsKey((String) sc.get("class")) && !PENDING.containsKey((String) sc.get("id"))) {
                    out.add(p);
                }
            }
        }
        return out;
    }

    @SuppressWarnings("unchecked")
    private static Map<String, Object> load(Path p) throws Exception {
        return (Map<String, Object>) JsonText.parse(Files.readString(p, StandardCharsets.UTF_8));
    }

    /** Run one scenario; returns the case ids it asserted. */
    @SuppressWarnings("unchecked")
    public static List<String> run(Path file, Path vectorsDir) throws Exception {
        Map<String, Object> sc = load(file);
        T.eq("depz.scenario/1", sc.get("schema"));
        String id = (String) sc.get("id");
        T.eq(id + ".json", file.getFileName().toString());

        List<Map<String, Object>> steps = new ArrayList<>();
        for (Object s : (List<Object>) sc.get("steps")) {
            steps.add((Map<String, Object>) s);
        }
        // No drops: each queue holds everything its scenario takes, plus what streams on after.
        Map<String, Integer> sizes = new HashMap<>();
        for (Map<String, Object> s : steps) {
            if (s.containsKey("take")) {
                sizes.merge((String) s.get("take"), ((Number) s.get("count")).intValue(), Integer::sum);
            }
        }

        ReplayLink link = new ReplayLink(vectorsDir.resolve("recordings").resolve((String) sc.get("recording")), true, false);
        DepzDevice dev = open(sc, link);
        Map<String, Subscription<?>> subs = new HashMap<>();
        try {
            T.check(dev.getClass() == CLASSES.get((String) sc.get("class")),
                    "resolved " + dev.getClass().getSimpleName() + ", not " + sc.get("class"));
            for (int i = 0; i < steps.size(); i++) {
                try {
                    runStep(dev, steps.get(i), subs, sizes);
                } catch (Throwable e) {
                    Throwable u = unwrap(e);
                    throw new AssertionError(id + " step " + i + " " + describe(steps.get(i)) + ": "
                            + u.getClass().getSimpleName() + ": " + u.getMessage(), u);
                }
            }
            for (Map.Entry<String, Subscription<?>> kv : subs.entrySet()) {
                T.check(kv.getValue().droppedCount() == 0, kv.getKey() + ": " + kv.getValue().droppedCount() + " items dropped");
            }
            if ("exhausted".equals(sc.getOrDefault("end", "open"))) {
                T.until(() -> link.rxExhausted() && link.txRemaining() == 0, 2000);
                T.check(link.rxExhausted() && link.txRemaining() == 0, id + ": capture not exhausted: "
                        + link.txRemaining() + " tx bytes never written, " + link.rxRemaining() + " rx chunks never delivered");
            }
        } finally {
            dev.close();
        }
        return List.of("scenarios/" + id, "recordings/" + sc.get("recording"));
    }

    // ── opening (§1 probe) ───────────────────────────────────────────────────

    @SuppressWarnings("unchecked")
    private static DepzDevice open(Map<String, Object> sc, ReplayLink link) {
        Integer vid = null;
        Integer pid = null;
        if (sc.get("usb_model") instanceof String model) {
            for (Map.Entry<Integer, String> kv : UsbIds.DEPZ_PID_MODEL.entrySet()) {
                if (kv.getValue().equals(model)) {
                    vid = UsbIds.DEPZ_USB_VID;
                    pid = kv.getKey();
                }
            }
            if (pid == null) {
                throw new IllegalArgumentException("usb_model '" + model + "' is not in the USB-id table");
            }
        }
        String probe = (String) sc.getOrDefault("probe", "canonical");
        if ("canonical".equals(probe)) {
            T.check(!sc.containsKey("identity"), "`identity` is for legacy captures only");
            return DepzDevices.openOnLink(link, vid, pid, REQUEST_TIMEOUT);
        }
        T.eq("legacy", probe);
        // The probe the SDKs made before contract 02 §4.4: GET_NAME_ACTIVE_SOFTWARE, plus
        // GET_DEVICE_NAME for the L5/L7 and 1D families only; the class attached in place.
        DepzDevice dev = new DepzDevice(link, REQUEST_TIMEOUT);
        try {
            String software = dev.getSoftwareName();
            Identity ident = Identity.parseSoftwareName(software);
            String deviceName = "";
            if (ident.sensorType() == Identity.SensorType.VL53L7 || ident.sensorType() == Identity.SensorType.VL53LX) {
                try {
                    deviceName = dev.getDeviceName();
                } catch (DepzException e) {
                    deviceName = "";
                }
            }
            if (sc.get("identity") instanceof Map<?, ?> want) {
                String m = mismatch(canon(ident), want, "identity");
                if (m != null) {
                    throw new AssertionError(m);
                }
            }
            return DepzDevices.attach(dev, Discovery.resolveSensorClass(software, deviceName, vid, pid));
        } catch (RuntimeException | AssertionError e) {
            dev.close();
            throw e;
        }
    }

    // ── steps (§2) ───────────────────────────────────────────────────────────

    @SuppressWarnings("unchecked")
    private static void runStep(DepzDevice dev, Map<String, Object> step, Map<String, Subscription<?>> subs,
            Map<String, Integer> sizes) throws Exception {
        if (step.containsKey("call")) {
            List<Object> args = step.get("args") instanceof List<?> l ? (List<Object>) l : List.of();
            if (step.containsKey("error")) {
                Object got;
                try {
                    got = call(dev, (String) step.get("call"), args);
                } catch (Throwable e) {
                    Throwable inner = unwrap(e);
                    String k = errorKind(inner);
                    T.check(Objects.equals(k, step.get("error")), "expected error " + step.get("error") + ", got "
                            + (k == null ? "unmapped" : k) + " (" + inner.getClass().getSimpleName() + ": " + inner.getMessage() + ")");
                    return;
                }
                throw new AssertionError("expected error " + step.get("error") + ", succeeded with " + JsonText.serialize(canon(got)));
            }
            Object result = call(dev, (String) step.get("call"), args);
            if (step.containsKey("expect")) {
                checkMatch(mismatch(canon(result), step.get("expect"), ""));
            }
        } else if (step.containsKey("subscribe")) {
            String name = (String) step.get("as");
            int capacity = sizes.getOrDefault(name, 0) + 4096;
            subs.put(name, subscribe(dev, (String) step.get("subscribe"), capacity));
        } else if (step.containsKey("take")) {
            Subscription<?> sub = subs.get((String) step.get("take"));
            List<Object> want = (List<Object>) step.get("expect");
            T.eq(((Number) step.get("count")).longValue(), (long) want.size());
            for (int i = 0; i < want.size(); i++) {
                Object item;
                try {
                    item = sub.take(TAKE_TIMEOUT);
                } catch (DepzTimeoutException e) {
                    throw new AssertionError("got " + i + " of " + want.size() + " items, then nothing for "
                            + TAKE_TIMEOUT.toSeconds() + " s");
                }
                checkMatch(mismatch(canon(item), want.get(i), "item[" + i + "]"));
            }
        } else if (step.containsKey("get")) {
            String prop = (String) step.get("get");
            Object value = get(dev, prop);
            checkMatch(mismatch(canon(value), step.get("expect"), prop));
        } else {
            throw new AssertionError("unknown step " + JsonText.serialize(step));
        }
    }

    private static void checkMatch(String mismatch) {
        if (mismatch != null) {
            throw new AssertionError(mismatch);
        }
    }

    static String camel(String canonical) {
        StringBuilder sb = new StringBuilder();
        boolean up = false;
        for (int k = 0; k < canonical.length(); k++) {
            char c = canonical.charAt(k);
            if (c == '_') {
                up = sb.length() > 0;
            } else {
                sb.append(up ? Character.toUpperCase(c) : c);
                up = false;
            }
        }
        return sb.toString();
    }

    private static String member(Class<?> type, String canonical) {
        for (Class<?> t = type; t != null; t = t.getSuperclass()) {
            String alias = ALIASES.get(t.getSimpleName() + "." + canonical);
            if (alias != null) {
                return alias;
            }
        }
        return camel(canonical);
    }

    private static List<Method> publicMethods(Class<?> type, String name) {
        List<Method> out = new ArrayList<>();
        for (Method m : type.getMethods()) {
            if (m.getName().equals(name) && !Modifier.isStatic(m.getModifiers())) {
                out.add(m);
            }
        }
        return out;
    }

    // A stream's subscribe method, its queue sized by name (as Python passes maxsize=).
    private static Subscription<?> subscribe(DepzDevice dev, String canonical, int capacity) throws Exception {
        String name = member(dev.getClass(), canonical);
        for (Method m : publicMethods(dev.getClass(), name)) {
            if (m.getParameterCount() == 1 && m.getParameterTypes()[0] == int.class
                    && Subscription.class.isAssignableFrom(m.getReturnType())) {
                return (Subscription<?>) m.invoke(dev, capacity);
            }
        }
        throw new AssertionError(dev.getClass().getSimpleName() + " has no " + name + "(int capacity): add '" + canonical
                + "' to the alias table");
    }

    private static Object get(DepzDevice dev, String canonical) throws Exception {
        String name = member(dev.getClass(), canonical);
        String getter = "get" + Character.toUpperCase(name.charAt(0)) + name.substring(1);
        for (String n : List.of(name, getter)) {
            for (Method m : publicMethods(dev.getClass(), n)) {
                if (m.getParameterCount() == 0) {
                    return m.invoke(dev);
                }
            }
        }
        throw new AssertionError(dev.getClass().getSimpleName() + " has no " + name + "() / " + getter + "()");
    }

    // Positional JSON arguments onto the overload with as many parameters.
    private static Object call(DepzDevice dev, String canonical, List<Object> args) throws Exception {
        String name = member(dev.getClass(), canonical);
        List<Method> methods = new ArrayList<>();
        for (Method m : publicMethods(dev.getClass(), name)) {
            if (m.getParameterCount() == args.size()) {
                methods.add(m);
            }
        }
        if (methods.isEmpty()) {
            throw new AssertionError(dev.getClass().getSimpleName() + " has no " + name + "(" + args.size()
                    + " args): add '" + canonical + "' to the alias table");
        }
        IllegalArgumentException last = null;
        for (Method m : methods) {
            Object[] call = new Object[args.size()];
            try {
                Class<?>[] ps = m.getParameterTypes();
                for (int i = 0; i < ps.length; i++) {
                    call[i] = arg(args.get(i), ps[i], canonical, i);
                }
            } catch (IllegalArgumentException e) {
                last = e; // this overload does not take these arguments
                continue;
            }
            return m.invoke(dev, call);
        }
        throw new AssertionError(canonical + ": no overload takes " + JsonText.serialize(args) + " (" + last.getMessage() + ")");
    }

    // An enum without a `value` field is numbered on the wire by declaration order (contract 15).
    @SuppressWarnings({"unchecked", "EnumOrdinal"})
    private static Object arg(Object a, Class<?> t, String call, int pos) {
        if (a instanceof Map<?, ?> rec) {
            Function<Map<String, Object>, Object> build = RECORD_ARGS.get(t);
            if (build == null) {
                throw new IllegalArgumentException(call + " argument " + pos + " is a record with no builder for " + t.getSimpleName());
            }
            return build.apply((Map<String, Object>) rec);
        }
        if (a == null) {
            if (t.isPrimitive()) {
                throw new IllegalArgumentException("null for a " + t.getSimpleName());
            }
            return null;
        }
        if (t == Duration.class) {
            throw new IllegalArgumentException(call + " argument " + pos + ": scenarios carry no timeouts");
        }
        if (t.isEnum()) {
            long v = ((Number) a).longValue();
            for (Object c : t.getEnumConstants()) {
                try {
                    if (((Number) t.getField("value").get(c)).longValue() == v) {
                        return c;
                    }
                } catch (ReflectiveOperationException e) {
                    if (((Enum<?>) c).ordinal() == v) {
                        return c;
                    }
                }
            }
            throw new IllegalArgumentException(t.getSimpleName() + " has no value " + v);
        }
        if (a instanceof List<?> items) {
            if (t.isArray()) {
                Object arr = Array.newInstance(t.getComponentType(), items.size());
                for (int i = 0; i < items.size(); i++) {
                    Array.set(arr, i, arg(items.get(i), t.getComponentType(), call, pos));
                }
                return arr;
            }
            if (t.isRecord()) {
                RecordComponent[] rc = t.getRecordComponents();
                if (rc.length != items.size()) {
                    throw new IllegalArgumentException(items.size() + " items for a " + rc.length + "-component record");
                }
                Object[] vals = new Object[rc.length];
                Class<?>[] types = new Class<?>[rc.length];
                for (int i = 0; i < rc.length; i++) {
                    types[i] = rc[i].getType();
                    vals[i] = arg(items.get(i), types[i], call, pos);
                }
                try {
                    return t.getDeclaredConstructor(types).newInstance(vals);
                } catch (ReflectiveOperationException e) {
                    throw new IllegalArgumentException(e);
                }
            }
            if (List.class.isAssignableFrom(t)) {
                return items;
            }
            throw new IllegalArgumentException("a list for a " + t.getSimpleName());
        }
        if (a instanceof Number n) {
            if (t == int.class || t == Integer.class) {
                return Math.toIntExact(n.longValue());
            }
            if (t == long.class || t == Long.class) {
                return n.longValue();
            }
            if (t == short.class || t == Short.class) {
                return (short) n.longValue();
            }
            if (t == byte.class || t == Byte.class) {
                return (byte) n.longValue();
            }
            if (t == double.class || t == Double.class) {
                return n.doubleValue();
            }
            if (t == float.class || t == Float.class) {
                return n.floatValue();
            }
            throw new IllegalArgumentException("a number for a " + t.getSimpleName());
        }
        if (a instanceof String s) {
            if (t == String.class) {
                return s;
            }
            throw new IllegalArgumentException("a string for a " + t.getSimpleName());
        }
        if (a instanceof Boolean b) {
            if (t == boolean.class || t == Boolean.class) {
                return b;
            }
            throw new IllegalArgumentException("a boolean for a " + t.getSimpleName());
        }
        throw new IllegalArgumentException("cannot pass " + a.getClass().getSimpleName() + " as " + t.getSimpleName());
    }

    // §4: the canonical kind of a Java exception, null for anything else.
    static String errorKind(Throwable e) {
        if (e instanceof DepzTimeoutException) {
            return "timeout";
        }
        if (e instanceof StatusException s) {
            return s.status() == Common.Status.ERR_CMD_NOT_SUPPORTED.value ? "not_supported" : "device_error";
        }
        if (e instanceof InvalidReplyException) {
            return "invalid_reply";
        }
        if (e instanceof IllegalArgumentException) {
            return "invalid_argument";
        }
        if (e instanceof UnsupportedOperationException) {
            return "not_supported";
        }
        return null;
    }

    private static Throwable unwrap(Throwable e) {
        while (e instanceof InvocationTargetException && e.getCause() != null) {
            e = e.getCause();
        }
        return e;
    }

    @SuppressWarnings("unchecked")
    private static String describe(Map<String, Object> step) {
        if (step.containsKey("call")) {
            List<Object> args = step.get("args") instanceof List<?> l ? (List<Object>) l : List.of();
            List<String> parts = new ArrayList<>();
            for (Object a : args) {
                parts.add(JsonText.serialize(a));
            }
            return step.get("call") + "(" + String.join(", ", parts) + ")";
        }
        if (step.containsKey("subscribe")) {
            return "subscribe " + step.get("subscribe") + " as " + step.get("as");
        }
        if (step.containsKey("take")) {
            return "take " + step.get("count") + " from " + step.get("take");
        }
        return "get " + step.get("get");
    }

    // ── matching (§3) ────────────────────────────────────────────────────────

    static String snake(String camel) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < camel.length(); i++) {
            char c = camel.charAt(i);
            if (Character.isUpperCase(c)) {
                boolean prevLowerOrDigit = i > 0 && (Character.isLowerCase(camel.charAt(i - 1)) || Character.isDigit(camel.charAt(i - 1)));
                boolean acronymEnd = i > 0 && Character.isUpperCase(camel.charAt(i - 1)) && i + 1 < camel.length()
                        && Character.isLowerCase(camel.charAt(i + 1));
                if (prevLowerOrDigit || acronymEnd) {
                    sb.append('_');
                }
                sb.append(Character.toLowerCase(c));
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    private static boolean isByteString(String component) {
        return component.equals("data") || component.equals("raw") || component.endsWith("Raw");
    }

    /**
     * A result as canonical JSON: records keyed by the Python field names (snake_case of the Java
     * component), enums by their wire value, bytes as hex when the component says raw data
     * ({@code …Raw}, {@code data}), else as an array of numbers; a {@code fields()} map where a
     * type offers one.
     */
    static Object canon(Object v) {
        return canon(v, true);
    }

    // An enum without a `value` field is numbered on the wire by declaration order (contract 15).
    @SuppressWarnings({"unchecked", "EnumOrdinal"})
    private static Object canon(Object v, boolean bytesAsHex) {
        if (v == null || v instanceof Boolean || v instanceof String) {
            return v;
        }
        if (v instanceof Optional<?> o) {
            return canon(o.orElse(null), bytesAsHex);
        }
        if (v instanceof Identity.SensorType st) {
            return st.label;
        }
        if (v instanceof Enum<?> e) {
            try {
                return ((Number) e.getDeclaringClass().getField("value").get(e)).longValue();
            } catch (ReflectiveOperationException x) {
                return (long) e.ordinal();
            }
        }
        if (v instanceof BigInteger) {
            return v;
        }
        if (v instanceof Double || v instanceof Float) {
            return ((Number) v).doubleValue();
        }
        if (v instanceof Number n) {
            return n.longValue();
        }
        if (v instanceof byte[] b) {
            if (bytesAsHex) {
                return JsonText.hex(b);
            }
            List<Object> out = new ArrayList<>();
            for (byte x : b) {
                out.add((long) (x & 0xFF));
            }
            return out;
        }
        if (v.getClass().isArray()) {
            List<Object> out = new ArrayList<>();
            for (int i = 0; i < Array.getLength(v); i++) {
                out.add(canon(Array.get(v, i), true));
            }
            return out;
        }
        if (v instanceof Map<?, ?> m) {
            Map<String, Object> out = new LinkedHashMap<>();
            for (Map.Entry<?, ?> e : m.entrySet()) {
                out.put(String.valueOf(e.getKey()), canon(e.getValue(), true));
            }
            return out;
        }
        if (v instanceof Iterable<?> it) {
            List<Object> out = new ArrayList<>();
            for (Object x : it) {
                out.add(canon(x, true));
            }
            return out;
        }
        try {
            Method fields = v.getClass().getMethod("fields");
            if (Map.class.isAssignableFrom(fields.getReturnType()) && fields.getParameterCount() == 0) {
                return canon(fields.invoke(v), true);
            }
        } catch (NoSuchMethodException e) {
            // no field map: walk the record
        } catch (ReflectiveOperationException e) {
            throw new IllegalStateException(e);
        }
        Class<?> t = v.getClass();
        if (t.isRecord()) {
            try {
                if (TUPLE_TYPES.contains(t)) {
                    List<Object> out = new ArrayList<>();
                    for (RecordComponent rc : t.getRecordComponents()) {
                        out.add(canon(rc.getAccessor().invoke(v), true));
                    }
                    return out;
                }
                Map<String, Object> out = new LinkedHashMap<>();
                for (RecordComponent rc : t.getRecordComponents()) {
                    out.put(snake(rc.getName()), canon(rc.getAccessor().invoke(v), isByteString(rc.getName())));
                }
                return out;
            } catch (ReflectiveOperationException e) {
                throw new IllegalStateException(e);
            }
        }
        throw new IllegalArgumentException("no canonical form for " + t.getSimpleName());
    }

    /** §3 matching; null when {@code got} matches {@code want}. */
    static String mismatch(Object got, Object want, String path) {
        String at = path.isEmpty() ? "result" : path;
        if (want instanceof Map<?, ?> w) {
            if (!(got instanceof Map<?, ?> g)) {
                return at + ": expected an object, got " + JsonText.serialize(got);
            }
            for (Map.Entry<?, ?> kv : w.entrySet()) {
                if (!g.containsKey(kv.getKey())) {
                    return at + "." + kv.getKey() + ": missing (fields: " + String.join(", ",
                            g.keySet().stream().map(String::valueOf).toList()) + ")";
                }
                String m = mismatch(g.get(kv.getKey()), kv.getValue(), path + "." + kv.getKey());
                if (m != null) {
                    return m;
                }
            }
            return null;
        }
        if (want instanceof List<?> wl) {
            if (!(got instanceof List<?> gl)) {
                return at + ": expected an array, got " + JsonText.serialize(got);
            }
            if (gl.size() != wl.size()) {
                return at + ": expected " + wl.size() + " elements, got " + gl.size();
            }
            for (int i = 0; i < wl.size(); i++) {
                String m = mismatch(gl.get(i), wl.get(i), path + "[" + i + "]");
                if (m != null) {
                    return m;
                }
            }
            return null;
        }
        if (want instanceof Double wd) {
            if (got instanceof Long || got instanceof Double || got instanceof BigInteger) {
                double gd = ((Number) got).doubleValue();
                if (Math.abs(gd - wd) <= Math.max(1e-9 * Math.abs(wd), 1e-12)) {
                    return null;
                }
            }
            return at + ": expected " + JsonText.serialize(wd) + ", got " + JsonText.serialize(got);
        }
        if (want instanceof Long wl2 && got instanceof Double gd2) {
            // An integer expectation matches a floating-point result of the same value.
            return gd2 == (double) wl2 ? null : at + ": expected " + wl2 + ", got " + JsonText.serialize(gd2);
        }
        // integers, strings, booleans, null: exact, and a bool never equals an integer
        if (got == null ? want == null : want != null && got.getClass() == want.getClass() && got.equals(want)) {
            return null;
        }
        return at + ": expected " + JsonText.serialize(want) + ", got " + JsonText.serialize(got);
    }
}
