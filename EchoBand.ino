#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs can be anything for testing
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-ba0987654321"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE test...");

  // Initialize BLE
  BLEDevice::deinit(); // Reset BLE in case of previous issues
  BLEDevice::init("EchoBand"); // This is the name your phone will see

  // Create BLE server and service
  pServer = BLEDevice::createServer();
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create characteristic (optional for now)
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true); // Make it visible
  pAdvertising->start();

  Serial.println("BLE device 'EchoBand' advertising now. Scan from your phone!");
}

void loop() {
  // Nothing here, just advertising
}
