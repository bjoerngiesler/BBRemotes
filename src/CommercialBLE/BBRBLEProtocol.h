#if !defined(BBRBLEPROTOCOL_H)
#define BBRBLEPROTOCOL_H

#include <Arduino.h>

#if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_SAMD_MKRWIFI1010

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "BBRProtocol.h"

namespace bb {
namespace rmt {

//! Bluetooth Low Energy Protocol superclass for commercial BLE receivers
class BLEProtocol: public Protocol, public BLEAdvertisedDeviceCallbacks, public BLEClientCallbacks {
public:
    BLEProtocol();

    virtual ProtocolType protocolType() { return INVALID_PROTOCOL; }

    virtual bool init(const std::string& nodeName);
    virtual void deinit();

    // BLE callbacks
    virtual bool discoverNodes(float timeout = 5);
    virtual void onResult(BLEAdvertisedDevice advertisedDevice);

    virtual bool isAcceptableForDiscovery(BLEAdvertisedDevice advertisedDevice) = 0;

    BLEClient* getClient() { return pClient_; }

    virtual bool requiresConnection() { return true; }
    virtual bool isConnected() { bb::rmt::printf("Connected: %d\n", connected_); return connected_; }

    virtual bool connect();

    // BLEClientCallbacks methods
    void onConnect(BLEClient *pClient);
    void onDisconnect(BLEClient *pClient);

protected:
    virtual bool connect(const NodeAddr& addr) = 0;

    BLEScan* pBLEScan_;
    BLEAdvertisedDevice* myDevice_;
    BLERemoteCharacteristic* pRemoteCharacteristic_;

    BLEClient* pClient_;

    bool connected_;
    bool initialized_;
};

};
}; // bb

#endif // !CONFIG_IDF_TARGET_ESP32S2
#endif // BBRBLEPROTOCOL_H