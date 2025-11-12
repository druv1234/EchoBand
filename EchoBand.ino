#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define DEBOUNCE_DELAY 300
#define SEQUENCE_TIMEOUT 3000
#define MOTION_THRESHOLD 0.7
#define WORD_DISPLAY_TIME 2000

MPU9250_asukiaaa mpu;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === BLE setup ===
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

unsigned long lastGestureTime = 0;
unsigned long lastMotionTime = 0;

float accelOffsetX = 0.0708;
float accelOffsetY = -0.0013;
float accelOffsetZ = -1.0221;
float gyroOffsetX = 1.6801;
float gyroOffsetY = -1.4891;
float gyroOffsetZ = -0.1652;

String gestureSequence[3];
int gestureCount = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // OLED setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not found!"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 25);
  display.println("EchoBand Ready!");
  display.display();
  delay(1000);

  // MPU setup
  mpu.setWire(&Wire);
  mpu.beginAccel();
  mpu.beginGyro();
  mpu.beginMag();

  // BLE setup
  BLEDevice::init("EchoBand"); // Shows up as "EchoBand" in Bluefruit
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
  Serial.println("BLE device 'EchoBand' ready. Open Bluefruit and connect!");

  showReady();
}

void loop() {
  mpu.accelUpdate();
  mpu.gyroUpdate();

  float ax = mpu.accelX() - accelOffsetX;
  float ay = mpu.accelY() - accelOffsetY;
  float az = mpu.accelZ() - accelOffsetZ;
  float gx = mpu.gyroX() - gyroOffsetX;
  float gy = mpu.gyroY() - gyroOffsetY;
  float gz = mpu.gyroZ() - gyroOffsetZ;

  String gesture = detectGesture(ax, ay, az, gx, gy, gz);
  unsigned long now = millis();

  if (gestureCount > 0 && (now - lastMotionTime > SEQUENCE_TIMEOUT)) {
    Serial.println("Sequence timed out — resetting.");
    resetSequence();
  }

  if (gesture != "" && now - lastGestureTime > DEBOUNCE_DELAY) {
    lastGestureTime = now;
    lastMotionTime = now;
    recordGesture(gesture);
  }

  delay(10);
}

String detectGesture(float ax, float ay, float az, float gx, float gy, float gz) {
  if (abs(ax) > MOTION_THRESHOLD && ax > 0.6) return "UP";
  if (abs(ax) > MOTION_THRESHOLD && ax < -0.6) return "DOWN";
  if (abs(ay) > MOTION_THRESHOLD && ay > 0.6) return "LEFT";
  if (abs(ay) > MOTION_THRESHOLD && ay < -0.6) return "RIGHT";
  return "";
}

void recordGesture(String gesture) {
  if (gestureCount < 3) {
    gestureSequence[gestureCount] = gesture;
    gestureCount++;
    showGesture(gesture);
    Serial.println("Gesture: " + gesture);
    sendBLE("Gesture: " + gesture);
  }

  if (gestureCount == 3) {
    String word = matchSequence();
    Serial.println("Word: " + word);
    sendBLE("Word: " + word);
    showWord(word);
    delay(WORD_DISPLAY_TIME);
    resetSequence();
  }
}

String matchSequence() {
  String g1 = gestureSequence[0];
  String g2 = gestureSequence[1];
  String g3 = gestureSequence[2];

  if (g1 == "UP" && g2 == "UP" && g3 == "UP") return "HELLO";
  if (g1 == "DOWN" && g2 == "DOWN" && g3 == "DOWN") return "BYE";
  if (g1 == "LEFT" && g2 == "LEFT" && g3 == "LEFT") return "THANK YOU";
  if (g1 == "RIGHT" && g2 == "RIGHT" && g3 == "RIGHT") return "SORRY";
  if (g1 == "UP" && g2 == "DOWN" && g3 == "RIGHT") return "GIG EM";
  if (g1 == "UP" && g2 == "DOWN" && g3 == "LEFT") return "HOWDY";
  if (g1 == "DOWN" && g2 == "DOWN" && g3 == "RIGHT") return "PERFECT";
  if (g1 == "DOWN" && g2 == "DOWN" && g3 == "LEFT") return "EXCUSE ME";
  if (g1 == "LEFT" && g2 == "RIGHT" && g3 == "UP") return "READY";
  if (g1 == "LEFT" && g2 == "RIGHT" && g3 == "DOWN") return "NO";
  if (g1 == "LEFT" && g2 == "DOWN" && g3 == "RIGHT") return "YES";

  return "UNKNOWN";
}

void showGesture(String gesture) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(25, 20);
  display.print(gesture);
  display.display();
}

void showWord(String word) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 20);
  display.print(word);
  display.display();
}

void showReady() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(30, 25);
  display.print("Ready...");
  display.display();
}

void resetSequence() {
  gestureCount = 0;
  for (int i = 0; i < 3; i++) gestureSequence[i] = "";
  lastMotionTime = millis();
  showReady();
}

void sendBLE(String message) {
  if (pCharacteristic) {
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();
  }
}
