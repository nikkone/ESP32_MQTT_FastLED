#include <FastLED.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "settings.h" // Edit the file this is including to add WIFI and MQTT settings

// FastLED settings
#define DATA_PIN    23
#define CLK_PIN     22
#define LED_TYPE    WS2801
#define COLOR_ORDER RBG
#define MAX_NUMBER_OF_LEDS 10

// WIFI settings
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// MQTT settings
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
String clientId = "ESP32Client-WS2801"; // CHANGE TO C STRING? 
const char* mqtt_receive_topic = "WS2801/command";

// Custom datatypes
enum Mode_t { MODE_OFF, MODE_FILL, MODE_EFFECT};
enum Effect_t {EFFECT_RAINBOW, EFFECT_RAINBOWGLITTER, EFFECT_BPM, EFFECT_JUGGLE, EFFECT_SINELON, EFFECT_CONFETTI};
// Global variables
WiFiClient espClient;
PubSubClient client(espClient);
CRGB leds[MAX_NUMBER_OF_LEDS];

Mode_t led_mode = MODE_OFF;
Effect_t led_effect = EFFECT_RAINBOW;
int red = 0;
int green = 0;
int blue = 0;
int brightness = 16;
int number_of_used_leds = MAX_NUMBER_OF_LEDS;
int frames_per_second = 120;
int gHue = 0; // rotating "base color" used by many of the patterns
int hue_update_interval_ms = 20;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // Connect to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect_mqtt() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_receive_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if(strcmp(topic, mqtt_receive_topic) == 0) {
    // ArduinoJSON parsing
    StaticJsonDocument<200> doc; // Create an object on the stack for parsing TODO: Check if this can be done dynamic(instead of setting it to 200)
    DeserializationError error = deserializeJson(doc, (char*)payload); // Deserialize means parse
    // Test if parsing succeeds
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    } 
    // Update global variables with the received values
    if(doc.containsKey("mode")) {
      led_mode = (Mode_t) ((int)doc["mode"]); 
    }
    if(doc.containsKey("effect")) {
      led_effect = (Effect_t) ((int)doc["effect"]); 
    }
    if(doc.containsKey("brightness")) {
      brightness = doc["brightness"];
      FastLED.setBrightness(brightness);
    }
    if(doc.containsKey("fps")) {
      frames_per_second = doc["fps"];
    }
    if(doc.containsKey("used_leds")) {
      int temp = doc["used_leds"];
      if(temp > MAX_NUMBER_OF_LEDS) {
        Serial.println("WARNING: used_leds is higher than allowed. Ignoring..");
      } else {
        number_of_used_leds = temp;
      }
    }
    if(doc.containsKey("hue_update_interval_ms")) {
      hue_update_interval_ms = doc["hue_update_interval_ms"];
    }
    if(doc.containsKey("rgb")) {
      red = doc["rgb"][0];
      green = doc["rgb"][1];
      blue = doc["rgb"][2];
    }
  } else {
    Serial.println("WARNING: Command not recognized. Ignoring..");
  }
}
void setup() {
  // Debug
  Serial.begin(115200);
  // Networking
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  // WS2801 using fastLED library
    delay(3000); // 3 second delay for recovery //TODO: WHY?
  
  // tell FastLED about the LED strip configuration
  //FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, number_of_used_leds).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, number_of_used_leds).setCorrection(TypicalLEDStrip);

  // Set master brightness control
  FastLED.setBrightness(brightness);
}
  
void loop() {

  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
    // React to command
  switch(led_mode) {
    case MODE_FILL:
        fill_solid(leds, number_of_used_leds, CRGB(red,green,blue));
        FastLED.show();
      break;
    case MODE_EFFECT:
        switch(led_effect) {
          case EFFECT_RAINBOW:
            fill_rainbow( leds, number_of_used_leds, gHue, 7);
            FastLED.show(); 
            break;
          case EFFECT_RAINBOWGLITTER:
            fill_rainbow( leds, number_of_used_leds, gHue, 7);
            addGlitter(80);
            FastLED.show(); 
            break;
          case EFFECT_BPM:
            bpm();
            break;
          case EFFECT_JUGGLE:
            juggle();
            break;
          case EFFECT_SINELON:
            sinelon();
            break;
          case EFFECT_CONFETTI:
            confetti();
            break;
        }
      break;
    case MODE_OFF: 
    default:
      FastLED.clear();
      break;
  }
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/frames_per_second); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( hue_update_interval_ms ) { gHue++; } // slowly cycle the "base color" through the rainbow
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(number_of_used_leds) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, number_of_used_leds, 10);
  int pos = random16(number_of_used_leds);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, number_of_used_leds, 20);
  int pos = beatsin16( 13, 0, number_of_used_leds-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}
void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < number_of_used_leds; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, number_of_used_leds, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, number_of_used_leds-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
