//! The dependency-free JSON parser / writer the `.depzrec` / `.depzdata` readers stand on: strict
//! RFC 8259 (contract 08 «Line rules»), compact output as Python's
//! `json.dumps(separators=(",", ":"))`.
use depz_sensor_sdk::json::{object_map, Value};

fn s(v: &str) -> Value {
    Value::String(v.into())
}

#[test]
fn string_escapes_decode_to_their_characters() {
    let v = Value::parse(r#""q\" b\\ s\/ \b\f\n\r\t é 😀""#).unwrap();
    assert_eq!(v, s("q\" b\\ s/ \u{8}\u{c}\n\r\t é 😀"));
    // raw multi-byte UTF-8 is copied as is
    assert_eq!(Value::parse("\"дальность 😀\"").unwrap(), s("дальность 😀"));
}

#[test]
fn strings_refuse_raw_control_characters_and_bad_escapes() {
    for (text, msg) in [
        ("\"a\u{1}b\"", "control character in string"),
        ("\"tab\there\"", "control character in string"),
        (r#""\x""#, "unknown escape"),
        (r#""\u12""#, "short unicode escape"),
        (r#""\u12zz""#, "bad unicode escape"),
        ("\"open", "unterminated string"),
        ("\"\\", "bad escape"),
    ] {
        let e = Value::parse(text).unwrap_err();
        assert_eq!(e.msg, msg, "{text:?}");
    }
}

#[test]
fn numbers_follow_the_rfc_grammar_and_keep_their_text() {
    for text in ["0", "-0", "12", "-3.25", "1e9", "6.02E+23", "-1.5e-3"] {
        assert_eq!(Value::parse(text).unwrap(), Value::Number(text.into()));
    }
    let big = Value::parse("9007199254740993").unwrap(); // past f64's exact integers
    assert_eq!(big.as_i64(), Some(9_007_199_254_740_993));
    assert_eq!(Value::parse("-3.25").unwrap().as_f64(), Some(-3.25));
    assert_eq!(Value::parse("2.5").unwrap().as_i64(), None);
    for text in ["-", "1.", ".5", "1e", "1e+", "+1", "-x"] {
        assert!(Value::parse(text).is_err(), "{text:?} parsed");
    }
    // a leading zero ends the number: what follows is trailing data
    assert_eq!(
        Value::parse("01").unwrap_err().msg,
        "trailing data after value"
    );
}

#[test]
fn nan_infinity_and_misspelt_literals_are_refused() {
    for text in ["NaN", "Infinity", "-Infinity", "nul", "tru", "fals", "True"] {
        assert!(Value::parse(text).is_err(), "{text:?} parsed");
    }
    assert_eq!(Value::parse(" true ").unwrap(), Value::Bool(true));
    assert_eq!(Value::parse("false").unwrap().as_bool(), Some(false));
    assert_eq!(Value::parse("null").unwrap(), Value::Null);
}

#[test]
fn errors_carry_the_byte_offset() {
    let e = Value::parse(r#"{"a":1,"b" 2}"#).unwrap_err();
    assert_eq!((e.pos, e.msg.as_str()), (11, "expected character"));
    assert_eq!(e.to_string(), "json error at 11: expected character");
    let e = Value::parse("[1 2]").unwrap_err();
    assert_eq!((e.pos, e.msg.as_str()), (3, "expected ',' or ']'"));
    let e = Value::parse(r#"{"a":1 "b":2}"#).unwrap_err();
    assert_eq!((e.pos, e.msg.as_str()), (7, "expected ',' or '}'"));
    let e = Value::parse("{} x").unwrap_err();
    assert_eq!((e.pos, e.msg.as_str()), (3, "trailing data after value"));
    assert_eq!(Value::parse("").unwrap_err().msg, "unexpected character");
}

#[test]
fn objects_keep_their_key_order_and_lookups_are_typed() {
    let v = Value::parse(r#" { "d1" : [ ] , "d0" : { } , "x" : [1, "two", null] } "#).unwrap();
    let keys: Vec<&str> = v
        .entries()
        .unwrap()
        .iter()
        .map(|(k, _)| k.as_str())
        .collect();
    assert_eq!(keys, ["d1", "d0", "x"]);
    assert_eq!(v.get("d1").unwrap().as_array().unwrap().len(), 0);
    assert_eq!(v.get("d0").unwrap().entries().unwrap().len(), 0);
    let x = v.get("x").unwrap().as_array().unwrap();
    assert_eq!(x[1].as_str(), Some("two"));
    assert_eq!(v.get("missing"), None);
    // the typed views answer None for any other kind of value
    assert_eq!(x[0].get("k"), None);
    assert_eq!(x[0].entries(), None);
    assert_eq!(x[0].as_array(), None);
    assert_eq!(x[0].as_str(), None);
    assert_eq!(x[1].as_i64(), None);
    assert_eq!(x[1].as_f64(), None);
    assert_eq!(x[1].as_bool(), None);
    // a repeated key: every entry kept in order, the map takes the last
    let dup = Value::parse(r#"{"k":1,"k":2}"#).unwrap();
    assert_eq!(dup.entries().unwrap().len(), 2);
    assert_eq!(object_map(&dup).unwrap()["k"], Value::int(2));
    assert_eq!(object_map(&Value::Null), None);
}

#[test]
fn output_is_compact_and_reads_back_the_same() {
    let v = Value::Object(vec![
        ("t".into(), Value::int(-42)),
        ("v".into(), Value::float(2.0)),
        ("ok".into(), Value::Bool(true)),
        ("n".into(), Value::Null),
        ("s".into(), s("a\"b\\c\nd\re\tf\u{1}g\u{1f}é")),
        (
            "a".into(),
            Value::Array(vec![Value::float(0.1), Value::int(0)]),
        ),
    ]);
    let text = v.to_json();
    assert_eq!(
        text,
        r#"{"t":-42,"v":2.0,"ok":true,"n":null,"s":"a\"b\\c\nd\re\tf\u0001g\u001fé","a":[0.1,0]}"#
    );
    assert_eq!(Value::parse(&text).unwrap(), v);
}

#[test]
fn a_float_that_is_not_finite_writes_null() {
    assert_eq!(Value::float(f64::NAN), Value::Null);
    assert_eq!(Value::float(f64::INFINITY), Value::Null);
    assert_eq!(Value::float(-0.5).to_json(), "-0.5");
    assert_eq!(Value::float(1e300).as_f64(), Some(1e300));
}
