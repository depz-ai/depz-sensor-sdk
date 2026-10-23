using System.Globalization;
using System.Text;

namespace Depz.Sensor.Json;

/// <summary>
/// Minimal JSON reader/writer for the JSON Lines formats (contracts 08, 09).
/// The SDK targets netstandard2.1 for Unity, which has no System.Text.Json, so
/// values are plain objects: <c>null</c>, <see cref="bool"/>, <see cref="string"/>,
/// <see cref="long"/> (integers that fit), <see cref="ulong"/> (larger positive
/// integers), <see cref="double"/>, <see cref="List{T}"/> of <c>object?</c> and
/// <see cref="Dictionary{TKey,TValue}"/> of <c>string</c> → <c>object?</c>
/// (key order kept as in the text).
/// </summary>
public static class JsonText
{
    /// <summary>Parse one JSON value; trailing whitespace is allowed, anything else is not.</summary>
    /// <exception cref="FormatException">The text is not valid JSON.</exception>
    public static object? Parse(string text)
    {
        if (text is null) throw new ArgumentNullException(nameof(text));
        var p = new Parser(text);
        p.SkipWs();
        object? v = p.Value(0);
        p.SkipWs();
        if (p.Pos != text.Length)
            throw p.Error("trailing characters");
        return v;
    }

    /// <summary>Compact JSON (no spaces) of a value tree built from the types <see cref="Parse"/> returns,
    /// plus any integer type, <see cref="float"/>, <c>byte[]</c> (lowercase hex string) and any
    /// <see cref="System.Collections.IDictionary"/> / <see cref="System.Collections.IEnumerable"/>.</summary>
    public static string Serialize(object? value)
    {
        var sb = new StringBuilder();
        Write(sb, value);
        return sb.ToString();
    }

    private static void Write(StringBuilder sb, object? v)
    {
        switch (v)
        {
            case null: sb.Append("null"); break;
            case bool b: sb.Append(b ? "true" : "false"); break;
            case string s: WriteString(sb, s); break;
            case byte[] bytes: WriteString(sb, BitConverter.ToString(bytes).Replace("-", "").ToLowerInvariant()); break;
            case double d: WriteDouble(sb, d); break;
            case float f: WriteDouble(sb, f); break;
            case Enum e: sb.Append(Convert.ToInt64(e, CultureInfo.InvariantCulture).ToString(CultureInfo.InvariantCulture)); break;
            case IFormattable n when IsInteger(v):
                sb.Append(n.ToString(null, CultureInfo.InvariantCulture)); break;
            case System.Collections.IDictionary dict:
                sb.Append('{');
                bool firstKey = true;
                foreach (System.Collections.DictionaryEntry kv in dict)
                {
                    if (!firstKey) sb.Append(',');
                    firstKey = false;
                    WriteString(sb, Convert.ToString(kv.Key, CultureInfo.InvariantCulture) ?? "");
                    sb.Append(':');
                    Write(sb, kv.Value);
                }
                sb.Append('}');
                break;
            case System.Collections.IEnumerable list:
                sb.Append('[');
                bool first = true;
                foreach (object? item in list)
                {
                    if (!first) sb.Append(',');
                    first = false;
                    Write(sb, item);
                }
                sb.Append(']');
                break;
            default:
                throw new ArgumentException($"cannot serialize {v.GetType().Name} as JSON");
        }
    }

    private static bool IsInteger(object v) =>
        v is sbyte || v is byte || v is short || v is ushort || v is int || v is uint || v is long || v is ulong;

    private static void WriteDouble(StringBuilder sb, double d)
    {
        if (double.IsNaN(d) || double.IsInfinity(d))
            throw new ArgumentException("JSON has no NaN or infinity");
        // "R" round-trips; a whole number stays without ".0", as Python's json writes ints.
        sb.Append(d.ToString("R", CultureInfo.InvariantCulture));
    }

    private static void WriteString(StringBuilder sb, string s)
    {
        sb.Append('"');
        foreach (char c in s)
        {
            switch (c)
            {
                case '"': sb.Append("\\\""); break;
                case '\\': sb.Append("\\\\"); break;
                case '\n': sb.Append("\\n"); break;
                case '\r': sb.Append("\\r"); break;
                case '\t': sb.Append("\\t"); break;
                case '\b': sb.Append("\\b"); break;
                case '\f': sb.Append("\\f"); break;
                default:
                    if (c < 0x20)
                        sb.Append("\\u").Append(((int)c).ToString("x4", CultureInfo.InvariantCulture));
                    else
                        sb.Append(c);
                    break;
            }
        }
        sb.Append('"');
    }

    private sealed class Parser
    {
        private const int MaxDepth = 256;
        private readonly string _s;
        public int Pos;

        public Parser(string s) => _s = s;

        public FormatException Error(string what) => new FormatException($"JSON: {what} at offset {Pos}");

        public void SkipWs()
        {
            while (Pos < _s.Length && (_s[Pos] == ' ' || _s[Pos] == '\t' || _s[Pos] == '\n' || _s[Pos] == '\r'))
                Pos++;
        }

        public object? Value(int depth)
        {
            if (depth > MaxDepth) throw Error("nesting too deep");
            if (Pos >= _s.Length) throw Error("unexpected end");
            char c = _s[Pos];
            switch (c)
            {
                case '{': return Obj(depth);
                case '[': return Arr(depth);
                case '"': return Str();
                case 't': Literal("true"); return true;
                case 'f': Literal("false"); return false;
                case 'n': Literal("null"); return null;
                default:
                    if (c == '-' || (c >= '0' && c <= '9')) return Num();
                    throw Error($"unexpected '{c}'");
            }
        }

        private void Literal(string word)
        {
            if (string.CompareOrdinal(_s, Pos, word, 0, word.Length) != 0) throw Error($"expected {word}");
            Pos += word.Length;
        }

        private Dictionary<string, object?> Obj(int depth)
        {
            var d = new Dictionary<string, object?>();
            Pos++;
            SkipWs();
            if (Pos < _s.Length && _s[Pos] == '}') { Pos++; return d; }
            while (true)
            {
                SkipWs();
                if (Pos >= _s.Length || _s[Pos] != '"') throw Error("expected a key");
                string key = Str();
                SkipWs();
                if (Pos >= _s.Length || _s[Pos] != ':') throw Error("expected ':'");
                Pos++;
                SkipWs();
                d[key] = Value(depth + 1);
                SkipWs();
                if (Pos >= _s.Length) throw Error("unexpected end");
                if (_s[Pos] == ',') { Pos++; continue; }
                if (_s[Pos] == '}') { Pos++; return d; }
                throw Error("expected ',' or '}'");
            }
        }

        private List<object?> Arr(int depth)
        {
            var l = new List<object?>();
            Pos++;
            SkipWs();
            if (Pos < _s.Length && _s[Pos] == ']') { Pos++; return l; }
            while (true)
            {
                SkipWs();
                l.Add(Value(depth + 1));
                SkipWs();
                if (Pos >= _s.Length) throw Error("unexpected end");
                if (_s[Pos] == ',') { Pos++; continue; }
                if (_s[Pos] == ']') { Pos++; return l; }
                throw Error("expected ',' or ']'");
            }
        }

        private string Str()
        {
            Pos++;
            var sb = new StringBuilder();
            while (true)
            {
                if (Pos >= _s.Length) throw Error("unterminated string");
                char c = _s[Pos++];
                if (c == '"') return sb.ToString();
                if (c < 0x20) throw Error("control character in string");
                if (c != '\\') { sb.Append(c); continue; }
                if (Pos >= _s.Length) throw Error("unterminated escape");
                char e = _s[Pos++];
                switch (e)
                {
                    case '"': sb.Append('"'); break;
                    case '\\': sb.Append('\\'); break;
                    case '/': sb.Append('/'); break;
                    case 'b': sb.Append('\b'); break;
                    case 'f': sb.Append('\f'); break;
                    case 'n': sb.Append('\n'); break;
                    case 'r': sb.Append('\r'); break;
                    case 't': sb.Append('\t'); break;
                    case 'u':
                        if (Pos + 4 > _s.Length
                            || !int.TryParse(_s.Substring(Pos, 4), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out int code))
                            throw Error("bad \\u escape");
                        sb.Append((char)code);
                        Pos += 4;
                        break;
                    default: throw Error($"bad escape \\{e}");
                }
            }
        }

        private object Num()
        {
            int start = Pos;
            if (_s[Pos] == '-') Pos++;
            if (Pos >= _s.Length || _s[Pos] < '0' || _s[Pos] > '9') throw Error("bad number");
            if (_s[Pos] == '0') Pos++;
            else while (Pos < _s.Length && _s[Pos] >= '0' && _s[Pos] <= '9') Pos++;
            bool isInt = true;
            if (Pos < _s.Length && _s[Pos] == '.')
            {
                isInt = false;
                Pos++;
                if (Pos >= _s.Length || _s[Pos] < '0' || _s[Pos] > '9') throw Error("bad fraction");
                while (Pos < _s.Length && _s[Pos] >= '0' && _s[Pos] <= '9') Pos++;
            }
            if (Pos < _s.Length && (_s[Pos] == 'e' || _s[Pos] == 'E'))
            {
                isInt = false;
                Pos++;
                if (Pos < _s.Length && (_s[Pos] == '+' || _s[Pos] == '-')) Pos++;
                if (Pos >= _s.Length || _s[Pos] < '0' || _s[Pos] > '9') throw Error("bad exponent");
                while (Pos < _s.Length && _s[Pos] >= '0' && _s[Pos] <= '9') Pos++;
            }
            string t = _s.Substring(start, Pos - start);
            if (isInt)
            {
                if (long.TryParse(t, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out long l)) return l;
                if (ulong.TryParse(t, NumberStyles.None, CultureInfo.InvariantCulture, out ulong u)) return u;
            }
            return double.Parse(t, NumberStyles.Float, CultureInfo.InvariantCulture);
        }
    }
}
