#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
#include <time.h>
#include <ArduinoJson.h>


// Wi-Fi credentials
const char* ssid = "SKY1C6AC";
const char* password = "DUsxUyNRWXFX";
//const char* ssid = "Camipho";
//const char* password = "Passwordee";

// AWS IoT
const char* mqtt_server = "a2qxwnrprho4b8-ats.iot.us-east-1.amazonaws.com";

// Certificates (fill in your real certificate values)

const char* cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDWTCCAkGgAwIBAgIUV+XDhU/9FgRsxlnmPNh7/bMfbSswDQYJKoZIhvcNAQEL\n" \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n" \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MDYxMjA0NTI0\n" \
"MVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n" \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMjMvGwLxg6txovk2oDe\n" \
"uvJw4uRWA92m+JrO+kasZKUAeenHS06mXDBy2IrW1ZDz66fUqBx8UQjwGM5pyp1+\n" \
"6QbUo9YKylQBPCVvTF97GZlpNxSx/AnCWCnqHYL9ojYgDeyxJQTYJByMPDsRzqJ7\n" \
"0NPMhBfLkKIQUN2txqWwHEbw07R3kzZBjKLOXvgCUfV2aEknd6btzPvjERnXcNaz\n" \
"rDe1nsYftG5jtM+tQjb8KALX8wZez/1YqVb5Kdb2oQiVx1cGPg3Yf13NN/sZ+v0Q\n" \
"mlUxZk+Mc3fTJhvz4fTnfB/LKqHyvMnlbzouOxByUw9cgSjxYQDon/37EP9ClsfG\n" \
"CzkCAwEAAaNgMF4wHwYDVR0jBBgwFoAUbtnqOQO+Lp2JFiImSmiDlSOD9lUwHQYD\n" \
"VR0OBBYEFPFpbHTl2gMeZDAfvnd8Sak/uDzUMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n" \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQC5f+3u+9m59uW559gEOA6vD3Xu\n" \
"OfvgOk6/v2O1RL9+Yj6zC2a5U43VfowiaEdTNbDuBBHuEvjswnl12uzzzshnfXd6\n" \
"57k1cS6r1UZDWfe/xrkCv5wjxOqGU2hSGBhaV9jnD7ovB9UDnTB+RcfDQqV4Z8ou\n" \
"DwFqvQ/crACpWc0MYTWo1oThBe/EHG4Xbe874F8uVIK/x0Wv7wd1UQ4PkMJsAxOm\n" \
"Y0xKyZAnhkRuuidotZ/nXkOBBU3aqkre0KNtdzbB0JxlT5aTkhBx9ZZ6mepGfzYQ\n" \
"hHga2QRrxYv299eEJSNmLXT/A2NFVJCCGYB+bQTmwwX45t+P9xVvpkZ3/Kzw\n" \
"-----END CERTIFICATE-----\n";

const char* key = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIEpAIBAAKCAQEAyMy8bAvGDq3Gi+TagN668nDi5FYD3ab4ms76RqxkpQB56cdL\n" \
"TqZcMHLYitbVkPPrp9SoHHxRCPAYzmnKnX7pBtSj1grKVAE8JW9MX3sZmWk3FLH8\n" \
"CcJYKeodgv2iNiAN7LElBNgkHIw8OxHOonvQ08yEF8uQohBQ3a3GpbAcRvDTtHeT\n" \
"NkGMos5e+AJR9XZoSSd3pu3M++MRGddw1rOsN7Wexh+0bmO0z61CNvwoAtfzBl7P\n" \
"/VipVvkp1vahCJXHVwY+Ddh/Xc03+xn6/RCaVTFmT4xzd9MmG/Ph9Od8H8sqofK8\n" \
"yeVvOi47EHJTD1yBKPFhAOif/fsQ/0KWx8YLOQIDAQABAoIBAB2ks/BTcKK8HsKK\n" \
"P4Ok3f5qkeRZmmp/etYH1kTWsGj3gAf2JvTudE+mtLcSbibfC5wUNdk2tRPXtiHK\n" \
"1mGX2bjrbWBs6V+rawCvxJuX5biTItGIUQfzy+YsLL0oymipJMUbhbaIXhRk04N4\n" \
"5oNwOez6lUmaALkcJYQEYn4VWKdUJQOVt0LLCLTMtc1YuJEJZ1+LBEknZaXfNP7D\n" \
"b47AsdLzKRSZ11LOioKuKmSWMwmsoLuFFJl2baXixEQp0TOhc2cAM4p+ojXpz1V1\n" \
"1sGkHMwxKMcZctdNPw2rf8Ja57Ej8L4L40zF4sZtyJ7K9Oau+cpdmvRDANBc3Zpw\n" \
"xOr03n0CgYEA+HitAY83WJ/PwIHLZd5ugGRskZdz4jGjSH46UhrioXKUjBTRitcA\n" \
"2AKzjEzWfc3qaUjn7sxGb3baPItjma1dwTsthW1WnSiOw8dnDamGgp5RCq14UMBK\n" \
"JJmgpQ55PtSwA8MJC6aN8WgqXBqThtfPMavzr38MSxl2xf5QDDV7Q+8CgYEAzuJI\n" \
"0D5G8Sbc/7ofljvdKYP7cGN38BnzSps7XlaghpXmCkIYotBOHAi7BmmQBHBCiwhg\n" \
"n6mYdFf5MOb4rZ5o68To37dj/d7gzjzty3Hkd9ycA0ADGNPYUsvXvOrOLpSYVkLI\n" \
"URjwJqBjT4hCB9c9tNjnAj12fDqZdjstCOYEW1cCgYEAnZjtIvATysKSoyewOwy6\n" \
"SfVoZ3AIsI+LYY+uriUfWgey8KbDwaxdfKU5/OM/qMvSwrTbZztp8YkRIxXGNtMf\n" \
"hFBkrxyKs2PmKYdwddnw1yhTftJIXe+ZF13Z5tcnUBLXEYvDUQBzR4sqUwEIUZ7Q\n" \
"bFEEX2vWAExGaY7Edvn1FUECgYEAjsEqCTfP7/snJ/agcSQhudHuoGCilDTz4hk3\n" \
"YCNaZUPuFkoBgedG3qVPmF8jF0z3PcSsF8AJCM7jjlDya6sRLw8Subxr7PPsH6N/\n" \
"WpDeW68IoF19RZZ4gLlTxnInj5DNhUhTvVH99Elb/bdCCPMHC1FYJf2PUq2E00aG\n" \
"DsvZWcUCgYAq+o9LqzyMc3vOwpTT1jPcbnMpI4nz/M0g9kXxRluTcFL9A6Z/M5HG\n" \
"ii7rQLUywj80o/ArE/xFjZS563xwl6Fkw5rynOMThxKg4P0PSUH/Td7Zb8lKws7Q\n" \
"9JueP3xfwV/5gNNDnTuePk8lI0RADK73vt7S+fXg4L5XalEM4sz5IA==\n" \
"-----END RSA PRIVATE KEY-----\n";

const char* ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";


// Pins
#define SOIL_PIN 34
#define ONE_WIRE_BUS 4
#define RXD2 26
#define TXD2 27
const int ledPin = 2;

// Sensors
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// AWS Client
WiFiClientSecure net;
PubSubClient client(net);

// Timers
unsigned long lastDataSend = 0;
const unsigned long dataInterval = 30UL * 60 * 1000; // 30 min

// Snapshot times
struct SnapshotTime {
  int hour;
  int minute;
};
SnapshotTime imageSnapshotTimes[] = {
  {6, 10}, {10, 10}, {14, 10}, {18, 10}, {20, 10}
};
bool snapshotSentToday[sizeof(imageSnapshotTimes) / sizeof(SnapshotTime)];

// Image buffer
const uint32_t MAX_IMAGE_SIZE = 60 * 1024;
uint8_t imageBuffer[MAX_IMAGE_SIZE];

// Base64 table
const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Receiving

// RT
bool realtimeActive = false;
unsigned long lastRealtimeSend = 0;
const unsigned long realtimeInterval = 20UL * 1000; // 10 seconds

// === MQTT Topics ===
const char* topicCommand = "plants/commands";
const char* topicSensor = "plants/status";
const char* topicImage = "plants/images/raw";
const char* topicSensorRT = "plants/rt/status";
const char* topicImageRT = "plants/rt/images";

// === Setup ===
void setup() {
  Serial.begin(115200);
  Serial2.begin(921600, SERIAL_8N1, RXD2, TXD2);
  pinMode(ledPin, OUTPUT);
  tempSensor.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" Connected.");
  syncTime();
  connectAWS();
}

// === Main Loop ===
void loop() {
  if (!client.connected()) connectAWS();
  client.loop();

  unsigned long nowMillis = millis();
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  if (timeinfo->tm_hour == 0 && timeinfo->tm_min == 0)
    memset(snapshotSentToday, 0, sizeof(snapshotSentToday));

  if (nowMillis - lastDataSend >= dataInterval) {
    lastDataSend = nowMillis;
    sendSensorData(timeinfo, false);
  }

  for (int i = 0; i < sizeof(imageSnapshotTimes)/sizeof(SnapshotTime); i++) {
    if (timeinfo->tm_hour == imageSnapshotTimes[i].hour &&
        timeinfo->tm_min == imageSnapshotTimes[i].minute &&
        !snapshotSentToday[i]) {
      sendImage(timeinfo, false);
      snapshotSentToday[i] = true;
    }
  }

  if (realtimeActive && nowMillis - lastRealtimeSend >= realtimeInterval) {
    lastRealtimeSend = nowMillis;
    sendSensorData(timeinfo, true);
    sendImage(timeinfo, true);
  }
}

// === MQTT & Time ===
void connectAWS() {
  net.setCACert(ca);
  net.setCertificate(cert);
  net.setPrivateKey(key);
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);

  while (!client.connected()) {
    if (client.connect("esp32-plant-device")) {
      client.subscribe(topicCommand);
    } else {
      digitalWrite(ledPin, HIGH);
      delay(1000);
      digitalWrite(ledPin, LOW);
      delay(1000);
    }
  }
}

void syncTime() {
  configTime(3600, 0, "pool.ntp.org");
  while (time(nullptr) < 100000) delay(500);
}

// === Serial Communication with Camera ===
void sendSnapshotCommand() {
  Serial2.println("__TAKE_SNAPSHOT__");
}

bool receiveImageAndStats(float &brightness, float &greenPercent, uint32_t &imgSize) {
  while (Serial2.available() < 4);
  imgSize = 0;
  for (int i = 0; i < 4; i++) imgSize = (imgSize << 8) | Serial2.read();
  if (imgSize > MAX_IMAGE_SIZE) return false;

  uint32_t received = 0;
  unsigned long timeout = millis();
  while (received < imgSize && millis() - timeout < 3000) {
    if (Serial2.available()) {
      int n = Serial2.readBytes(&imageBuffer[received], imgSize - received);
      received += n;
      timeout = millis();
    }
  }
  if (received != imgSize) return false;

  while (Serial2.available() < 8);
  byte statsBytes[8];
  Serial2.readBytes(statsBytes, 8);
  memcpy(&brightness, statsBytes, 4);
  memcpy(&greenPercent, statsBytes + 4, 4);
  return true;
}

String base64Encode(const uint8_t* data, size_t len) {
  String out;
  for (size_t i = 0; i < len; i += 3) {
    uint32_t val = (data[i] << 16) + ((i+1 < len ? data[i+1] : 0) << 8) + (i+2 < len ? data[i+2] : 0);
    for (int j = 0; j < 4; j++) {
      if (i * 8 + j * 6 > len * 8) out += '=';
      else out += b64_table[(val >> (18 - 6 * j)) & 0x3F];
    }
  }
  return out;
}

// === Send Sensor Data ===
void sendSensorData(struct tm* timeinfo, bool liveOnly) {
  int soil = analogRead(SOIL_PIN);
  float moisture = constrain(map(soil, 2600, 1053, 0, 100), 0, 100);

  tempSensor.requestTemperatures();
  delay(100);
  float tempC = tempSensor.getTempCByIndex(0);

  float brightness = 0, green = 0;
  uint32_t imgSize;

  sendSnapshotCommand();
  if (receiveImageAndStats(brightness, green, imgSize)) {
    char payload[512];
    char timestampStr[32];
    strftime(timestampStr, sizeof(timestampStr), "%m-%d %H:%M", timeinfo);

    snprintf(payload, sizeof(payload),
      "{\"timestamp\":\"%s\",\"temp\":%.2f,\"moist\":%.2f,\"brightness\":%.2f,\"green\":%.2f}",
      timestampStr, tempC, moisture, brightness, green);

    const char* topic = liveOnly ? topicSensorRT : topicSensor;
    client.publish(topic, payload);
    Serial.println("Published sensor data");
  }
}

void sendImage(struct tm* timeinfo, bool liveOnly) {
  float brightness = 0, green = 0;
  uint32_t imgSize;

  sendSnapshotCommand();
  if (receiveImageAndStats(brightness, green, imgSize)) {
    String base64Img = base64Encode(imageBuffer, imgSize);

    // Use ArduinoJson to safely build the JSON
    StaticJsonDocument<1024> doc;
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%m-%d %H:%M", timeinfo);

    doc["t"] = timeStr;
    doc["i"] = base64Img;

    String output;
    serializeJson(doc, output);

    const char* topic = liveOnly ? topicImageRT : topicImage;
    if (client.beginPublish(topic, output.length(), false)) {
      client.write((const uint8_t*)output.c_str(), output.length());
      client.endPublish();
      Serial.println("Published image data: size=" + String(output.length()));
    } else {
      Serial.println("Failed to begin publish");
    }
  } else {
    Serial.println("Image capture failed.");
  }
}

// Handle messages from Unity
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  payload[length] = '\0';  // Null-terminate the payload
  String message = String((char*)payload);
  Serial.println(message);

  // Try to parse as JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (!error && doc.containsKey("realtime")) {
    String val = doc["realtime"];
    realtimeActive = (val == "on");
    Serial.println("Realtime mode set to: " + String(realtimeActive ? "ON" : "OFF"));
  } else {
    // Fall back to plain string parsing
    if (message == "realtime_on") {
      realtimeActive = true;
      Serial.println("Realtime enabled (fallback)");
    } else if (message == "realtime_off") {
      realtimeActive = false;
      Serial.println("Realtime disabled (fallback)");
    }
  }
}
