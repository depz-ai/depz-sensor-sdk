package ai.depz.sensor.test;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Minimal recursive-descent JSON parser for the golden vectors. No external
 * dependencies. Objects → {@code Map<String,Object>}, arrays → {@code List<Object>},
 * strings → {@code String}, integers → {@code Long}, reals → {@code Double},
 * {@code true/false} → {@code Boolean}, {@code null} → {@code null}.
 */
public final class Json {
    private final String s;
    private int i;

    private Json(String s) {
        this.s = s;
    }

    public static Object parse(String text) {
        Json j = new Json(text);
        j.ws();
        Object v = j.value();
        j.ws();
        if (j.i != j.s.length()) {
            throw new RuntimeException("trailing JSON at index " + j.i);
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
            throw new RuntimeException("expected '" + lit + "' at index " + i);
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
            if (s.charAt(i) != ':') throw new RuntimeException("expected ':' at " + i);
            i++;
            ws();
            m.put(key, value());
            ws();
            char c = s.charAt(i++);
            if (c == '}') break;
            if (c != ',') throw new RuntimeException("expected ',' or '}' at " + (i - 1));
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
            if (c != ',') throw new RuntimeException("expected ',' or ']' at " + (i - 1));
        }
        return a;
    }

    private String string() {
        if (s.charAt(i) != '"') throw new RuntimeException("expected string at " + i);
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
                    default: throw new RuntimeException("bad escape \\" + e);
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
        return Long.parseLong(tok);
    }
}
