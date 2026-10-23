package ai.depz.sensor.test;

import ai.depz.sensor.json.JsonText;

/** The golden-vector parser: the SDK's {@link JsonText}. */
public final class Json {
    private Json() {}

    public static Object parse(String text) {
        return JsonText.parse(text);
    }
}
