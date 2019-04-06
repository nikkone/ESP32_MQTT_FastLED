# ESP32_MQTT_FastLED
An Arduino sketch for controlling the FastLED library by MQTT on the ESP32.
## Setup
1. Install all needed libraries
2. Edit `settings.h`, and the FastLed settings in the top part of `esp32_mqtt_WS2801.ino`
3. Compile and upload the sketch from the Arduino IDE to the ESP32 board
## Required Arduino libraries 
* [FastLED](https://github.com/FastLED/FastLED)
  * The effects in this sketch are copied from the DemoReel100 example.
* [ArduinoJSON](https://arduinojson.org/)
* [PubSubClient](https://pubsubclient.knolleary.net/)
  * MQTT client for Arduino
* [ESP32 for Arduino](https://github.com/espressif/arduino-esp32)
  * Setting up ESP32 in the Arduino IDE
## Supported MQTT commands
All MQTT commands sent to the device must be in JSON format. Accepted variables should be:
* `mode`: Set the mode the LEDstrip should be in
  * Available options is defined in an enum
  `enum Mode_t { MODE_OFF, MODE_FILL, MODE_EFFECT};`
  * Example `{"mode":2,"effect":0}` sets the mode to `MODE_EFFECT`
* `effect`: Set the effect used when `mode` is `MODE_EFFECT`
  * Available options is defined in an enum
  `enum Effect_t {EFFECT_RAINBOW, EFFECT_RAINBOWGLITTER, EFFECT_BPM, EFFECT_JUGGLE, EFFECT_SINELON, EFFECT_CONFETTI};`
* `brightness`: Set global brightness
  * Example: `{"brightness":127}`
* `used_leds`: Set number of active leds
  * Example: `{"used_leds":5}`
* `rgb`: Set color used when `mode` is `MODE_FILL`
  * Example: `{"mode":1,"rgb":[30,45,255]}`
* `fps`: Supposed to set fps, not sure if works properly
* `hue_update_interval_ms`: Supposed to set how fast the patterns/colors change in effect mode, not sure if works properly
## An easy GUI to send these commands
* Setup a rasperry pi following [RPIoT](https://github.com/nikkone/rpiot)
* In node-red, install MQTT nodes and node-red-dashboard from palette manager
* Make a flow that creates and sends the JSON messages from the dashboard nodes to MQTT
  * I used the function block to assemble a string in JSON format before sending
* GUI should now appear on `http://[IP of RPI]:[node-red port]/ui`
