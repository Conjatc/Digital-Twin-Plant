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
"Enter the cert"
"-----END CERTIFICATE-----\n";

const char* key = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"Enter the key"
"-----END RSA PRIVATE KEY-----\n";

const char* ca = \
"-----BEGIN CERTIFICATE-----\n" \
"Enter the cert"
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

