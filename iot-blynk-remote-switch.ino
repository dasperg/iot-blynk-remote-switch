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
#define PIN_UPTIME V5

WiFiClient client;

//define your default values here, if there are different values in config.json, they are overwritten.
//char mqtt_server[40];
//char mqtt_port[6] = "8080";
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

//            strcpy(mqtt_server, json["mqtt_server"]);
//            strcpy(mqtt_port, json["mqtt_port"]);
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
//    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
//    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_blynk_token("blynk", "Blynk token", blynk_token, 32);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //set static ip
//    wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //add all your parameters here
//    wifiManager.addParameter(&custom_mqtt_server);
//    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_blynk_token);
    //reset saved settings
    wifiManager.resetSettings();

    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    //wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
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
//    strcpy(mqtt_server, custom_mqtt_server.getValue());
//    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(blynk_token, custom_blynk_token.getValue());

    //save the custom parameters to FS
    if (shouldSaveConfig) {
      Serial.println("saving config");
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
//      json["mqtt_server"] = mqtt_server;
//      json["mqtt_port"] = mqtt_port;
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

// BLYNK_CONNECTED() {
//     Blynk.syncAll();
// }
//
// //here handlers for sync command
// BLYNK_WRITE(function(pin, val) {
//     GPIO.set_mode(GPIO.MODE_OUTPUT);
//     GPIO.write(pin, val);
// });

BLYNK_WRITE(inputPin) {
  if (param.asInt()) { // act only on the HIGH and not the LOW of the momentary
    digitalWrite(outputPin, !digitalRead(outputPin));  // invert pin state just once
    tripWire.setTimeout(1000L, [](){
      digitalWrite(outputPin, !digitalRead(outputPin));  // then invert it back after 1000ms
    });
  }
}

//}

void loop() {
  // put your main code here, to run repeatedly:
  Blynk.run();
}
