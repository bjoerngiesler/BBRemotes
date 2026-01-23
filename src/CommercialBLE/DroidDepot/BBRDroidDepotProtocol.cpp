#include <Arduino.h>

#if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_SAMD_MKRWIFI1010

#include "BBRDroidDepotProtocol.h"
#include "BBRDroidDepotTransmitter.h"

using namespace bb;
using namespace bb::rmt;

static BLEUUID SERVICE_UUID("09b600A0-3e42-41fc-b474-e9c0c8f0c801");
static BLEUUID NOTIFICATION_UUID("09b600b0-3e42-41fc-b474-e9c0c8f0c801");
static BLEUUID WRITE_UUID("09b600b1-3e42-41fc-b474-e9c0c8f0c801");

const std::vector<std::string> DroidDepotProtocol::inputNames = {
    INPUT_NAME_SPEED, 
    INPUT_NAME_TURN_RATE, 
    INPUT_NAME_DOME_RATE, 
    INPUT_NAME_EMOTE_0, 
    INPUT_NAME_EMOTE_1, 
    INPUT_NAME_EMOTE_2, 
    "Accessory"
};

static const std::string EMPTY("");

DroidDepotProtocol::DroidDepotProtocol() {
    pBLEScan_ = nullptr;
    myDevice_ = nullptr;
    pCharacteristic_ = nullptr;
    transmitter_ = nullptr;


}

uint8_t DroidDepotProtocol::numTransmitterTypes() { return 1; }

Transmitter* DroidDepotProtocol::createTransmitter(uint8_t transmitterType) {
    if(transmitter_ == nullptr) {
        transmitter_ = new DroidDepotTransmitter(this);
    }
    return transmitter_;
}

bool DroidDepotProtocol::isAcceptableForDiscovery(BLEAdvertisedDevice advertisedDevice) {
    if(advertisedDevice.getName() == "DROID") {
        printf("Found device with name \"%s\"\n", advertisedDevice.getName().c_str());
        return true;
    }
    return false;
}

bool DroidDepotProtocol::connect(const NodeAddr& addr) {
    //bb::rmt::printf("FIXME not connecting for now - just for test\n");
    //return false;

    if(pClient_ == nullptr) {
        pClient_ = BLEDevice::createClient();
        Serial.printf(" - Created client\n");

        pClient_->setClientCallbacks(this);
    }

    // Connect to the remote BLE Server.
    Serial.printf("Connecting to %s\n", addr.toString().c_str());
    if(pClient_->connect(BLEAddress(addr.toString()), esp_ble_addr_type_t(1)) == false) {
        Serial.printf("Connecting failed\n");
        pClient_ = nullptr;
        return false;
    }

    Serial.printf(" - Connected to server\n");
    pClient_->setMTU(46); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient_->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
      Serial.printf("Failed to find our service UUID: %s\n", SERVICE_UUID.toString().c_str());
      pClient_->disconnect();
      return false;
    }
    Serial.printf(" - Found our service\n");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pCharacteristic_ = pRemoteService->getCharacteristic(WRITE_UUID);
    if (pCharacteristic_ == nullptr) {
      Serial.printf("Failed to find our characteristic UUID: %s\n", WRITE_UUID.toString().c_str());
      pClient_->disconnect();
      return false;
    }
    Serial.printf(" - Found our characteristic\n");

    initialWrites();

    return true;
}

bool DroidDepotProtocol::initialWrites() {
    if(pCharacteristic_ == nullptr) {
        Serial.printf("Characteristic is NULL\n");
        return false;
    }

    Serial.printf("Doing initial writes\n");
    for(int i=0; i<4; i++) {
        pCharacteristic_->writeValue({0x22, 0x20, 0x01}, 3);
        delay(500);
    }
    for(int i=0; i<2; i++) {
        pCharacteristic_->writeValue({0x27, 0x42, 0x0f, 0x44, 0x44, 0x00, 0x1f, 0x00}, 8);
        delay(500);
        pCharacteristic_->writeValue({0x27, 0x42, 0x0f, 0x44, 0x44, 0x00, 0x18, 0x02}, 8);
        delay(500);
    }
    Serial.printf("Initial writes done\n");

    return true;
}

bool DroidDepotProtocol::pairWith(const NodeDescription& node) {
    bb::rmt::printf("Pairing with DroidDepot\n");
    if(isPaired(node.addr)) {
        Serial.printf("Already paired with %s\n", node.addr.toString().c_str());
        return false;
    }

    if(pClient_ == nullptr) {
        pClient_ = BLEDevice::createClient();
        Serial.printf(" - Created client\n");

        pClient_->setClientCallbacks(this);
    }

    if(connect(node.addr) == false) {
        Serial.printf("Could not connect\n");
        return false;
    }
    
    return Protocol::pairWith(node);
}

bool DroidDepotProtocol::transmitCommand(uint8_t *payload, uint8_t len) {
    if(pCharacteristic_ == nullptr) return false;
    
    pCharacteristic_->writeValue(payload, len);
    return true;
}

uint8_t DroidDepotProtocol::numInputs(const NodeAddr& addr) {
    return NUM_INPUTS;
}

const std::string& DroidDepotProtocol::inputName(const NodeAddr& addr, uint8_t input) {
    if(input < NUM_INPUTS) return inputNames[input];
    return EMPTY;
}

uint8_t DroidDepotProtocol::inputWithName(const NodeAddr& addr, const std::string& name) {
    for(int i=0; i<NUM_INPUTS; i++) if(inputNames[i] == name) return i;
    return INPUT_INVALID;
}


#endif // !CONFIG_IDF_TARGET_ESP32S2