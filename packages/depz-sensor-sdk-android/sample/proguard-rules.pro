# The app shows and logs the name of the Java SDK class a board opened as ("Vl53l8chDevice"):
# R8 keeps those names. Nothing else of the SDK needs a rule here — its own come with its jar.
-keepnames class ai.depz.sensor.devices.*Device
