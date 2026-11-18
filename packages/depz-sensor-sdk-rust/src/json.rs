//! The JSON value of the `.depzrec` / `.depzdata` lines (contracts 08, 09). `serde_json` parses and
//! writes it — `preserve_order`: object key order is contract (records list devices as `d0`, `d1`,
//! …); `arbitrary_precision`: a number is its source text, so integers past f64's exact range stay
//! exact. The nesting limit of contract 08 «Line rules» is [`Value::parse`]'s own check.

use serde::de::Deserialize;
use serde::ser::{Error as _, Serialize, Serializer};

/// A parsed JSON value.
#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    /// Numbers are kept as their source text so integers stay exact (the one normalisation, when
    /// parsed and when written: an exponent is `e` with its sign, `1e9` → `1e+9`). Other text
    /// (say the `NaN` of [`Value::float`]) has no JSON form: [`Value::to_json`] refuses it.
    Number(String),
    String(String),
    Array(Vec<Value>),
    /// Insertion-ordered key/value pairs, each key once: a repeated key in the text keeps its last
    /// value, as the reference SDK's `json.loads` does.
    Object(Vec<(String, Value)>),
}

/// JSON parse error: `msg` and the position `pos` it was detected at — serde_json's 1-based column
/// (the texts parsed are single lines); 0 when the error has no position.
#[derive(Default, Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct JsonError {
    pub pos: usize,
    pub msg: String,
}

impl std::fmt::Display for JsonError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "json error at {}: {}", self.pos, self.msg)
    }
}

impl std::error::Error for JsonError {}

impl From<serde_json::Error> for JsonError {
    /// serde_json's message without its ` at line L column C` tail — the line is the caller's to
    /// name (a dataset's, a capture's).
    fn from(e: serde_json::Error) -> JsonError {
        let full = e.to_string();
        let tail = format!(" at line {} column {}", e.line(), e.column());
        JsonError {
            pos: e.column(),
            msg: full.strip_suffix(&tail).unwrap_or(&full).to_string(),
        }
    }
}

/// Contract 08 «Line rules»: arrays and objects nest at most this deep (the outermost value is
/// level 1) — deeper is a parse error, not a stack overflow.
const MAX_DEPTH: usize = 256;

/// The nesting check, before serde_json sees the text (its own limit of 128 is lifted in
/// [`Value::parse`]; a line nested a million deep would otherwise overflow the stack): brackets
/// outside strings, counted.
pub(crate) fn check_depth(text: &str) -> Result<(), JsonError> {
    let (mut depth, mut in_string, mut escaped) = (0usize, false, false);
    for (pos, b) in text.bytes().enumerate() {
        match b {
            _ if escaped => escaped = false,
            b'\\' if in_string => escaped = true,
            b'"' => in_string = !in_string,
            b'[' | b'{' if !in_string => {
                depth += 1;
                if depth > MAX_DEPTH {
                    return Err(JsonError {
                        pos,
                        msg: "nested deeper than 256".to_string(),
                    });
                }
            }
            b']' | b'}' if !in_string => depth = depth.saturating_sub(1),
            _ => {}
        }
    }
    Ok(())
}

impl Value {
    /// Parse a single strict RFC 8259 JSON value from `text` (surrounding whitespace allowed,
    /// nothing else after the value); nested deeper than 256 is `nested deeper than 256`.
    pub fn parse(text: &str) -> Result<Value, JsonError> {
        check_depth(text)?;
        let mut de = serde_json::Deserializer::from_str(text);
        de.disable_recursion_limit();
        let v = serde_json::Value::deserialize(&mut de)?;
        de.end()?;
        Ok(Value::from(v))
    }

    /// Object lookup (`None` for non-objects or missing keys).
    pub fn get(&self, key: &str) -> Option<&Value> {
        match self {
            Value::Object(entries) => entries.iter().find(|(k, _)| k == key).map(|(_, v)| v),
            _ => None,
        }
    }

    /// The insertion-ordered entries of an object.
    pub fn entries(&self) -> Option<&[(String, Value)]> {
        match self {
            Value::Object(e) => Some(e),
            _ => None,
        }
    }

    pub fn as_array(&self) -> Option<&[Value]> {
        match self {
            Value::Array(a) => Some(a),
            _ => None,
        }
    }

    pub fn as_str(&self) -> Option<&str> {
        match self {
            Value::String(s) => Some(s),
            _ => None,
        }
    }

    /// Parse the number text as a signed integer.
    pub fn as_i64(&self) -> Option<i64> {
        match self {
            Value::Number(s) => s.parse::<i64>().ok(),
            _ => None,
        }
    }

    /// Parse the number text as a float.
    pub fn as_f64(&self) -> Option<f64> {
        match self {
            Value::Number(s) => s.parse::<f64>().ok(),
            _ => None,
        }
    }

    pub fn as_bool(&self) -> Option<bool> {
        match self {
            Value::Bool(b) => Some(*b),
            _ => None,
        }
    }

    /// A number value from an integer.
    pub fn int(v: i64) -> Value {
        Value::Number(v.to_string())
    }

    /// A number value from a float (shortest text that reads back the same; a whole number keeps
    /// its `.0`, as Python writes floats). Not finite: the text `NaN` / `inf` / `-inf`, which
    /// [`Value::to_json`] and so the writers refuse (contract 09).
    pub fn float(v: f64) -> Value {
        Value::Number(format!("{v:?}"))
    }

    /// Compact JSON text (`", "` / `": "`-free, as `json.dumps(separators=(",", ":"))`). A
    /// [`Value::Number`] whose text is no JSON number is the error.
    pub fn to_json(&self) -> Result<String, JsonError> {
        Ok(serde_json::to_string(self)?)
    }
}

impl From<serde_json::Value> for Value {
    fn from(v: serde_json::Value) -> Value {
        use serde_json::Value as J;
        match v {
            J::Null => Value::Null,
            J::Bool(b) => Value::Bool(b),
            J::Number(n) => Value::Number(n.to_string()),
            J::String(s) => Value::String(s),
            J::Array(a) => Value::Array(a.into_iter().map(Value::from).collect()),
            J::Object(o) => {
                Value::Object(o.into_iter().map(|(k, v)| (k, Value::from(v))).collect())
            }
        }
    }
}

impl Serialize for Value {
    fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        match self {
            Value::Null => s.serialize_unit(),
            Value::Bool(b) => s.serialize_bool(*b),
            Value::Number(n) => serde_json::from_str::<serde_json::Number>(n)
                .map_err(|e| S::Error::custom(format!("not a JSON number {n:?}: {e}")))?
                .serialize(s),
            Value::String(t) => s.serialize_str(t),
            Value::Array(a) => s.collect_seq(a),
            Value::Object(e) => s.collect_map(e.iter().map(|(k, v)| (k, v))),
        }
    }
}

/// Contract 08 «Line rules»: a `.depzrec` / `.depzdata` line of nothing but JSON whitespace
/// (space, tab, CR) is blank — NUL, NBSP or another Unicode space is not.
pub(crate) fn is_blank_line(line: &str) -> bool {
    line.bytes().all(|b| matches!(b, b' ' | b'\t' | b'\r'))
}
