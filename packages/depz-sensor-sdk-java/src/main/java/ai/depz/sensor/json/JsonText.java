package ai.depz.sensor.json;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Base64;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * The SDK's own minimal JSON, for {@code .depzrec} captures and {@code .depzdata}
 * datasets (no external dependency). {@link #parse}: objects → {@code Map<String,Object>}
 * (insertion-ordered), arrays → {@code List<Object>}, strings → {@code String},
 * integers → {@code Long} ({@code BigInteger} beyond the long range), reals →
 * {@code Double}, {@code true/false} → {@code Boolean}, {@code null} → {@code null}.
 * {@link #serialize} writes those back, plus any {@code Number}, primitive arrays and
 * {@code byte[]} (as a lowercase hex string).
 */
public final class JsonText {
    private final String s;
    private int i;

    private JsonText(String s) {
        this.s = s;
    }

    public static Object parse(String text) {
        JsonText j = new JsonText(text);
        j.ws();
        Object v = j.value();
        j.ws();
        if (j.i != j.s.length()) {
            throw new IllegalArgumentException("trailing JSON at index " + j.i);
        }
        return v;
    }

    private void ws() {
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                i++;
            } else {
                break;
            }
        }
    }

    private Object value() {
        char c = s.charAt(i);
        switch (c) {
            case '{': return object();
            case '[': return array();
            case '"': return string();
            case 't': expect("true"); return Boolean.TRUE;
            case 'f': expect("false"); return Boolean.FALSE;
            case 'n': expect("null"); return null;
            default: return number();
        }
    }

    private void expect(String lit) {
        if (!s.startsWith(lit, i)) {
            throw new IllegalArgumentException("expected '" + lit + "' at index " + i);
        }
        i += lit.length();
    }

    private Map<String, Object> object() {
        Map<String, Object> m = new LinkedHashMap<>();
        i++; // {
        ws();
        if (s.charAt(i) == '}') { i++; return m; }
        while (true) {
            ws();
            String key = string();
            ws();
            if (s.charAt(i) != ':') throw new IllegalArgumentException("expected ':' at " + i);
            i++;
            ws();
            m.put(key, value());
            ws();
            char c = s.charAt(i++);
            if (c == '}') break;
            if (c != ',') throw new IllegalArgumentException("expected ',' or '}' at " + (i - 1));
        }
        return m;
    }

    private List<Object> array() {
        List<Object> a = new ArrayList<>();
        i++; // [
        ws();
        if (s.charAt(i) == ']') { i++; return a; }
        while (true) {
            ws();
            a.add(value());
            ws();
            char c = s.charAt(i++);
            if (c == ']') break;
            if (c != ',') throw new IllegalArgumentException("expected ',' or ']' at " + (i - 1));
        }
        return a;
    }

    private String string() {
        if (s.charAt(i) != '"') throw new IllegalArgumentException("expected string at " + i);
        i++;
        StringBuilder sb = new StringBuilder();
        while (true) {
            char c = s.charAt(i++);
            if (c == '"') break;
            if (c == '\\') {
                char e = s.charAt(i++);
                switch (e) {
                    case '"': sb.append('"'); break;
                    case '\\': sb.append('\\'); break;
                    case '/': sb.append('/'); break;
                    case 'b': sb.append('\b'); break;
                    case 'f': sb.append('\f'); break;
                    case 'n': sb.append('\n'); break;
                    case 'r': sb.append('\r'); break;
                    case 't': sb.append('\t'); break;
                    case 'u':
                        sb.append((char) Integer.parseInt(s.substring(i, i + 4), 16));
                        i += 4;
                        break;
                    default: throw new IllegalArgumentException("bad escape \\" + e);
                }
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    private Object number() {
        int start = i;
        boolean real = false;
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c == '-' || c == '+' || (c >= '0' && c <= '9')) {
                i++;
            } else if (c == '.' || c == 'e' || c == 'E') {
                real = true;
                i++;
            } else {
                break;
            }
        }
        String tok = s.substring(start, i);
        if (real) {
            return Double.parseDouble(tok);
        }
        try {
            return Long.parseLong(tok);
        } catch (NumberFormatException e) {
            // beyond the long range (u64 >= 2^63): exact, as BigInteger
            return new BigInteger(tok);
        }
    }

    // ── writing ──────────────────────────────────────────────────────────────

    /** One value as compact JSON. */
    public static String serialize(Object value) {
        StringBuilder sb = new StringBuilder();
        write(sb, value);
        return sb.toString();
    }

    private static void write(StringBuilder sb, Object v) {
        if (v == null) {
            sb.append("null");
        } else if (v instanceof Boolean b) {
            sb.append(b ? "true" : "false");
        } else if (v instanceof String s) {
            writeString(sb, s);
        } else if (v instanceof byte[] bytes) {
            writeString(sb, hex(bytes));
        } else if (v instanceof Double || v instanceof Float) {
            double d = ((Number) v).doubleValue();
            if (Double.isNaN(d) || Double.isInfinite(d)) {
                throw new IllegalArgumentException("JSON has no NaN or infinity");
            }
            sb.append(d == Math.rint(d) && Math.abs(d) < 1e15 ? String.format(Locale.ROOT, "%.1f", d) : Double.toString(d));
        } else if (v instanceof Number n) {
            sb.append(n instanceof BigInteger ? n.toString() : Long.toString(n.longValue()));
        } else if (v instanceof Map<?, ?> m) {
            sb.append('{');
            boolean first = true;
            for (Map.Entry<?, ?> e : m.entrySet()) {
                if (!first) {
                    sb.append(',');
                }
                first = false;
                writeString(sb, String.valueOf(e.getKey()));
                sb.append(':');
                write(sb, e.getValue());
            }
            sb.append('}');
        } else if (v instanceof Iterable<?> it) {
            sb.append('[');
            boolean first = true;
            for (Object x : it) {
                if (!first) {
                    sb.append(',');
                }
                first = false;
                write(sb, x);
            }
            sb.append(']');
        } else if (v.getClass().isArray()) {
            int n = java.lang.reflect.Array.getLength(v);
            List<Object> items = new ArrayList<>(n);
            for (int i = 0; i < n; i++) {
                items.add(java.lang.reflect.Array.get(v, i));
            }
            write(sb, items);
        } else if (v instanceof Enum<?> e) {
            writeString(sb, e.name());
        } else {
            throw new IllegalArgumentException("cannot serialize " + v.getClass().getSimpleName() + " as JSON");
        }
    }

    private static void writeString(StringBuilder sb, String s) {
        sb.append('"');
        for (int k = 0; k < s.length(); k++) {
            char c = s.charAt(k);
            switch (c) {
                case '"' -> sb.append("\\\"");
                case '\\' -> sb.append("\\\\");
                case '\n' -> sb.append("\\n");
                case '\r' -> sb.append("\\r");
                case '\t' -> sb.append("\\t");
                case '\b' -> sb.append("\\b");
                case '\f' -> sb.append("\\f");
                default -> {
                    if (c < 0x20) {
                        sb.append(String.format(Locale.ROOT, "\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
                }
            }
        }
        sb.append('"');
    }

    /** Lowercase hex of {@code b}. */
    public static String hex(byte[] b) {
        StringBuilder sb = new StringBuilder(b.length * 2);
        for (byte x : b) {
            sb.append(Character.forDigit((x >> 4) & 0xF, 16)).append(Character.forDigit(x & 0xF, 16));
        }
        return sb.toString();
    }

    /** Bytes of a hex string (even length). */
    public static byte[] unhex(String s) {
        byte[] out = new byte[s.length() / 2];
        for (int k = 0; k < out.length; k++) {
            out[k] = (byte) Integer.parseInt(s.substring(2 * k, 2 * k + 2), 16);
        }
        return out;
    }

    /** Standard base64 of {@code b}. */
    public static String base64(byte[] b) {
        return Base64.getEncoder().encodeToString(b);
    }
}
