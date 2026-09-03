//! Minimal, dependency-free JSON value parser.
//!
//! The foundation crate carries no runtime dependencies; the dataset reader
//! (contract 09) needs to parse `.depzdata` JSON Lines, so this module provides
//! a small, safe recursive-descent parser for the JSON subset the format uses.
//! Object key order is preserved (records list devices as `d0`, `d1`, …).

use std::collections::BTreeMap;

/// A parsed JSON value.
#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    /// Numbers are kept as their source text so integers stay exact.
    Number(String),
    String(String),
    Array(Vec<Value>),
    /// Insertion-ordered key/value pairs.
    Object(Vec<(String, Value)>),
}

/// JSON parse error with a byte offset into the source.
#[derive(Debug, Clone, PartialEq, Eq)]
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

impl Value {
    /// Parse a single JSON value from `text` (trailing whitespace allowed).
    pub fn parse(text: &str) -> Result<Value, JsonError> {
        let bytes = text.as_bytes();
        let mut p = Parser { bytes, pos: 0 };
        p.skip_ws();
        let v = p.parse_value()?;
        p.skip_ws();
        if p.pos != bytes.len() {
            return Err(p.err("trailing data after value"));
        }
        Ok(v)
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
}

struct Parser<'a> {
    bytes: &'a [u8],
    pos: usize,
}

impl<'a> Parser<'a> {
    fn err(&self, msg: &str) -> JsonError {
        JsonError {
            pos: self.pos,
            msg: msg.to_string(),
        }
    }

    fn peek(&self) -> Option<u8> {
        self.bytes.get(self.pos).copied()
    }

    fn skip_ws(&mut self) {
        while let Some(c) = self.peek() {
            if c == b' ' || c == b'\t' || c == b'\n' || c == b'\r' {
                self.pos += 1;
            } else {
                break;
            }
        }
    }

    fn parse_value(&mut self) -> Result<Value, JsonError> {
        match self.peek() {
            Some(b'{') => self.parse_object(),
            Some(b'[') => self.parse_array(),
            Some(b'"') => Ok(Value::String(self.parse_string()?)),
            Some(b't') | Some(b'f') => self.parse_bool(),
            Some(b'n') => self.parse_null(),
            Some(c) if c == b'-' || c.is_ascii_digit() => self.parse_number(),
            _ => Err(self.err("unexpected character")),
        }
    }

    fn expect(&mut self, c: u8) -> Result<(), JsonError> {
        if self.peek() == Some(c) {
            self.pos += 1;
            Ok(())
        } else {
            Err(self.err("expected character"))
        }
    }

    fn parse_object(&mut self) -> Result<Value, JsonError> {
        self.expect(b'{')?;
        let mut entries: Vec<(String, Value)> = Vec::new();
        self.skip_ws();
        if self.peek() == Some(b'}') {
            self.pos += 1;
            return Ok(Value::Object(entries));
        }
        loop {
            self.skip_ws();
            let key = self.parse_string()?;
            self.skip_ws();
            self.expect(b':')?;
            self.skip_ws();
            let val = self.parse_value()?;
            entries.push((key, val));
            self.skip_ws();
            match self.peek() {
                Some(b',') => {
                    self.pos += 1;
                }
                Some(b'}') => {
                    self.pos += 1;
                    return Ok(Value::Object(entries));
                }
                _ => return Err(self.err("expected ',' or '}'")),
            }
        }
    }

    fn parse_array(&mut self) -> Result<Value, JsonError> {
        self.expect(b'[')?;
        let mut items = Vec::new();
        self.skip_ws();
        if self.peek() == Some(b']') {
            self.pos += 1;
            return Ok(Value::Array(items));
        }
        loop {
            self.skip_ws();
            items.push(self.parse_value()?);
            self.skip_ws();
            match self.peek() {
                Some(b',') => {
                    self.pos += 1;
                }
                Some(b']') => {
                    self.pos += 1;
                    return Ok(Value::Array(items));
                }
                _ => return Err(self.err("expected ',' or ']'")),
            }
        }
    }

    fn parse_string(&mut self) -> Result<String, JsonError> {
        self.expect(b'"')?;
        let mut out = String::new();
        loop {
            let c = self.peek().ok_or_else(|| self.err("unterminated string"))?;
            self.pos += 1;
            match c {
                b'"' => return Ok(out),
                b'\\' => {
                    let e = self.peek().ok_or_else(|| self.err("bad escape"))?;
                    self.pos += 1;
                    match e {
                        b'"' => out.push('"'),
                        b'\\' => out.push('\\'),
                        b'/' => out.push('/'),
                        b'b' => out.push('\u{0008}'),
                        b'f' => out.push('\u{000C}'),
                        b'n' => out.push('\n'),
                        b'r' => out.push('\r'),
                        b't' => out.push('\t'),
                        b'u' => {
                            let cp = self.parse_hex4()?;
                            // Handle surrogate pairs.
                            if (0xD800..=0xDBFF).contains(&cp) {
                                if self.peek() == Some(b'\\') {
                                    self.pos += 1;
                                    self.expect(b'u')?;
                                    let lo = self.parse_hex4()?;
                                    let c = 0x10000
                                        + (((cp - 0xD800) as u32) << 10)
                                        + (lo - 0xDC00) as u32;
                                    out.push(
                                        char::from_u32(c)
                                            .ok_or_else(|| self.err("bad surrogate"))?,
                                    );
                                } else {
                                    return Err(self.err("lone high surrogate"));
                                }
                            } else {
                                out.push(
                                    char::from_u32(cp as u32)
                                        .ok_or_else(|| self.err("bad code point"))?,
                                );
                            }
                        }
                        _ => return Err(self.err("unknown escape")),
                    }
                }
                _ => {
                    // Copy the (possibly multi-byte UTF-8) character.
                    let start = self.pos - 1;
                    let mut end = self.pos;
                    while end < self.bytes.len() && (self.bytes[end] & 0xC0) == 0x80 {
                        end += 1;
                    }
                    self.pos = end;
                    let s = std::str::from_utf8(&self.bytes[start..end])
                        .map_err(|_| self.err("invalid utf-8"))?;
                    out.push_str(s);
                }
            }
        }
    }

    fn parse_hex4(&mut self) -> Result<u16, JsonError> {
        if self.pos + 4 > self.bytes.len() {
            return Err(self.err("short unicode escape"));
        }
        let s = std::str::from_utf8(&self.bytes[self.pos..self.pos + 4])
            .map_err(|_| self.err("bad unicode escape"))?;
        let v = u16::from_str_radix(s, 16).map_err(|_| self.err("bad unicode escape"))?;
        self.pos += 4;
        Ok(v)
    }

    fn parse_bool(&mut self) -> Result<Value, JsonError> {
        if self.bytes[self.pos..].starts_with(b"true") {
            self.pos += 4;
            Ok(Value::Bool(true))
        } else if self.bytes[self.pos..].starts_with(b"false") {
            self.pos += 5;
            Ok(Value::Bool(false))
        } else {
            Err(self.err("invalid literal"))
        }
    }

    fn parse_null(&mut self) -> Result<Value, JsonError> {
        if self.bytes[self.pos..].starts_with(b"null") {
            self.pos += 4;
            Ok(Value::Null)
        } else {
            Err(self.err("invalid literal"))
        }
    }

    fn parse_number(&mut self) -> Result<Value, JsonError> {
        let start = self.pos;
        if self.peek() == Some(b'-') {
            self.pos += 1;
        }
        while let Some(c) = self.peek() {
            if c.is_ascii_digit() || c == b'.' || c == b'e' || c == b'E' || c == b'+' || c == b'-' {
                self.pos += 1;
            } else {
                break;
            }
        }
        let s = std::str::from_utf8(&self.bytes[start..self.pos])
            .map_err(|_| self.err("bad number"))?;
        if s.is_empty() || s == "-" {
            return Err(self.err("bad number"));
        }
        Ok(Value::Number(s.to_string()))
    }
}

/// Convenience: collect an object into a key→value map (last key wins).
pub fn object_map(v: &Value) -> Option<BTreeMap<String, Value>> {
    v.entries()
        .map(|e| e.iter().cloned().collect::<BTreeMap<_, _>>())
}
