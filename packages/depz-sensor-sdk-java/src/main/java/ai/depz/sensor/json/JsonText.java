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
 * {@code byte[]} (as a lowercase hex string). Malformed or truncated text is refused with
 * {@link IllegalArgumentException} naming the index — never an index error of the runtime.
 */
public final class JsonText {
    /**
     * Arrays and objects nest at most this deep, the outermost value being level 1
     * (contract 08 «Line rules»): beyond it the text is refused instead of
     * overflowing the stack of this recursive-descent parser.
     */
    public static final int MAX_DEPTH = 256;

    private final String s;
    private int i;
    private int depth;

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

    /** The character at the cursor; truncated text is a malformed-JSON error, not an index error. */
    private char peek() {
        if (i >= s.length()) {
            throw new IllegalArgumentException("unexpected end of JSON at index " + i);
        }
        return s.charAt(i);
    }

    private char take() {
        char c = peek();
        i++;
        return c;
    }

    private Object value() {
        char c = peek();
        switch (c) {
            case '{': enter(); return object();
            case '[': enter(); return array();
            case '"': return string();
            case 't': expect("true"); return true;
            case 'f': expect("false"); return false;
            case 'n': expect("null"); return null;
            default: return number();
        }
    }

    private void enter() {
        if (++depth > MAX_DEPTH) {
            throw new IllegalArgumentException("nested deeper than " + MAX_DEPTH + " at index " + i);
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
        if (peek() == '}') { i++; depth--; return m; }
        while (true) {
            ws();
            String key = string();
            ws();
            if (peek() != ':') throw new IllegalArgumentException("expected ':' at " + i);
            i++;
            ws();
            m.put(key, value());
            ws();
            char c = take();
            if (c == '}') break;
            if (c != ',') throw new IllegalArgumentException("expected ',' or '}' at " + (i - 1));
        }
        depth--;
        return m;
    }

    private List<Object> array() {
        List<Object> a = new ArrayList<>();
        i++; // [
        ws();
        if (peek() == ']') { i++; depth--; return a; }
        while (true) {
            ws();
            a.add(value());
            ws();
            char c = take();
            if (c == ']') break;
            if (c != ',') throw new IllegalArgumentException("expected ',' or ']' at " + (i - 1));
        }
        depth--;
        return a;
    }

    private String string() {
        if (peek() != '"') throw new IllegalArgumentException("expected string at " + i);
        i++;
        StringBuilder sb = new StringBuilder();
        while (true) {
            char c = take();
            if (c == '"') break;
            if (c == '\\') {
                char e = take();
                switch (e) {
                    case '"': sb.append('"'); break;
                    case '\\': sb.append('\\'); break;
                    case '/': sb.append('/'); break;
                    case 'b': sb.append('\b'); break;
                    case 'f': sb.append('\f'); break;
                    case 'n': sb.append('\n'); break;
                    case 'r': sb.append('\r'); break;
                    case 't': sb.append('\t'); break;
                    case 'u': {
                        int code = 0;
                        for (int k = 0; k < 4; k++) {
                            int d = hexDigit(take());
                            if (d < 0) throw new IllegalArgumentException("bad \\u escape at " + (i - 1));
                            code = (code << 4) | d;
                        }
                        sb.append((char) code);
                        break;
                    }
                    default: throw new IllegalArgumentException("bad escape \\" + e);
                }
            } else if (c < 0x20) {
                throw new IllegalArgumentException("control character in string at index " + (i - 1));
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    /** RFC 8259 number: {@code -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)?}. */
    private Object number() {
        int start = i;
        boolean real = false;
        if (at('-')) i++;
        if (at('0')) {
            i++;
        } else if (!digits()) {
            if (i == start) {
                throw new IllegalArgumentException("unexpected character '" + s.charAt(i) + "' at index " + i);
            }
            throw new IllegalArgumentException("bad number at index " + start);
        }
        if (at('.')) {
            i++;
            real = true;
            if (!digits()) throw new IllegalArgumentException("bad number at index " + start);
        }
        if (at('e') || at('E')) {
            i++;
            real = true;
            if (at('+') || at('-')) i++;
            if (!digits()) throw new IllegalArgumentException("bad number at index " + start);
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

    private boolean at(char c) {
        return i < s.length() && s.charAt(i) == c;
    }

    /** Skips ASCII digits; false when there was none. */
    private boolean digits() {
        int from = i;
        while (i < s.length() && s.charAt(i) >= '0' && s.charAt(i) <= '9') i++;
        return i > from;
    }

    /** An ASCII hex digit's value, else -1 ({@link Character#digit} also takes other scripts' digits). */
    private static int hexDigit(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    /** Contract 08 «Line rules»: a line of nothing but JSON whitespace (space, tab, CR) is blank. */
    public static boolean isBlankLine(String line) {
        for (int k = 0; k < line.length(); k++) {
            char c = line.charAt(k);
            if (c != ' ' && c != '\t' && c != '\r') return false;
        }
        return true;
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

    /** Bytes of a hex string: even length, hex digits only, else {@link IllegalArgumentException}. */
    public static byte[] unhex(String s) {
        if ((s.length() & 1) != 0) {
            throw new IllegalArgumentException("hex string of odd length " + s.length());
        }
        byte[] out = new byte[s.length() / 2];
        for (int k = 0; k < out.length; k++) {
            int hi = hexDigit(s.charAt(2 * k));
            int lo = hexDigit(s.charAt(2 * k + 1));
            if (hi < 0 || lo < 0) {
                throw new IllegalArgumentException("not a hex string at index " + 2 * k);
            }
            out[k] = (byte) ((hi << 4) | lo);
        }
        return out;
    }

    /** Standard base64 of {@code b}. */
    public static String base64(byte[] b) {
        return Base64.getEncoder().encodeToString(b);
    }
}
