# MQTTPublisher_ESP32

## Architecture
Since this ESP chip does not support the arduino framework on Platformio,
only esp-idf is used.

to add esp-idf components, configure the dependencies in ```src/idf_component.yml```

this python package was missing from esptools, so install it:
```
~/.platformio/penv/Scripts/pip install intelhex
```