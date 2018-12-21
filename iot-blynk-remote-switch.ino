#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <BlynkSimpleEsp8266.h>  //https://github.com/blynkkk/blynk-library/releases/latest
#include <ArduinoJson.h>         //https://github.com/bblanchon/ArduinoJson

#define DEBUG_MAX;  // WifiManager
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
// Use Virtual pin 5 for uptime display
#define INPUT_PIN V5
// Use physical pin 2 to output
#define OUTPUT_PIN 5

#define LED_PIN 2

WiFiClient client;

char blynk_token[34];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
    Serial.begin(115200);
    pinMode(OUTPUT_PIN, OUTPUT);
    digitalWrite(OUTPUT_PIN, HIGH);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    //clean FS, for testing
    SPIFFS.format();

    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin()) {
      Serial.println("mounted file system");
      if (SPIFFS.exists("/config.json")) {
        //file exists, reading and loading
        Serial.println("reading config file");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile) {
          Serial.println("opened config file");
          size_t size = configFile.size();
          // Allocate a buffer to store contents of the file.
          std::unique_ptr<char[]> buf(new char[size]);

          configFile.readBytes(buf.get(), size);
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(buf.get());
          json.printTo(Serial);

          if (json.success()) {
            Serial.println("\nparsed json");
            strcpy(blynk_token, json["blynk_token"]);
          } else {
            Serial.println("failed to load json config");
          }
          configFile.close();
        }
      }
    } else {
      Serial.println("failed to mount FS");
    }
    //end read

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_blynk_token("blynk", "Blynk token", blynk_token, 32);
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_blynk_token);

    //reset saved settings
    wifiManager.resetSettings();

    //fetches ssid and pass from eeprom and tries to connect
    //and goes into a blocking loop awaiting configuration
    //uses auto generated name ESP + ChipID
    if (!wifiManager.autoConnect()) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    //read updated parameters
    strcpy(blynk_token, custom_blynk_token.getValue());

    //save the custom parameters to FS
    if (shouldSaveConfig) {
      Serial.println("saving config");
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["blynk_token"] = blynk_token;

      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      }

      json.printTo(Serial);
      json.printTo(configFile);
      configFile.close();
      //end save
    }

    Serial.println("local ip");
    Serial.println(WiFi.localIP());

    Serial.println("Blynk token:");
    Serial.print(blynk_token);
    Serial.println();
    Blynk.config(blynk_token);
}

//BLYNK_READ(PIN_UPTIME)
//{
  // This command writes Arduino's uptime in seconds to Virtual Pin (5)
//  Blynk.virtualWrite(PIN_UPTIME, millis() / 1000);

/*
 *  This is from mos
 */
//Blynk.setHandler(function(conn, cmd, pin, val, id) {
//  let ram = Sys.free_ram() / 1024;
//  if (cmd === 'vr') {
//    // When reading any virtual pin, report free RAM in KB
//    Blynk.virtualWrite(conn, pin, ram, id);
//  } else if (cmd === 'vw') {
//    // Writing to virtual pin translate to writing to physical pin
//    GPIO.set_mode(pin, GPIO.MODE_OUTPUT);
//    GPIO.write(pin, val);
//  }
//  print('BLYNK JS handler, ram', ram, cmd, id, pin, val);
//}, null);

BLYNK_CONNECTED() {
    Blynk.syncAll();
}

BLYNK_WRITE(INPUT_PIN) {
  Serial.print("inputPin: ");
  Serial.println(INPUT_PIN);
  if (param.asInt()) {
    digitalWrite(OUTPUT_PIN, LOW);
    Serial.print("Output: ");
    Serial.println(HIGH);
  } else {
    digitalWrite(OUTPUT_PIN, HIGH);
    Serial.print("Output: ");
    Serial.println(LOW);
  }
  Serial.print("PinValue: ");
  Serial.println(digitalRead(OUTPUT_PIN));
}

// BLYNK_WRITE(V1) {
//   Serial.print("Reset settings");
//   SPIFFS.format();
//   wifiManager.resetSettings();
//   delay(3000);
//   ESP.reset();
// }

BLYNK_WRITE(V2) {
  if (param.asInt()) {
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, HIGH);
  }
}

void loop() {
  Blynk.run();
}
