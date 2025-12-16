#if !defined(BBRDROIDDEPOTTRANSMITTER_H)
#define BBRDROIDDEPOTTRANSMITTER_H

#include <Arduino.h>

#if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_SAMD_MKRWIFI1010

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "BBRTransmitter.h"
#include "BBRDroidDepotProtocol.h"

namespace bb {
namespace rmt {
class DroidDepotProtocol;

//! Droid Depot BLE Transmitter
class DroidDepotTransmitter: public TransmitterBase<DroidDepotProtocol> {
public:
    DroidDepotTransmitter(DroidDepotProtocol *proto);

    bool canAddAxes() { return true; }

    virtual bool receiverSideMapping() { return false; }
    virtual bool syncReceiverSideMapping() {return true; }
    virtual bool transmit();
   
protected:
    float inputVals_[DroidDepotProtocol::NUM_INPUTS];

    void initialWrites();

    std::vector<uint8_t> axisValues_;
    bool connected_;
};
}; // rmt
}; // bb

#endif // !CONFIG_IDF_TARGET_ESP32S2
#endif // BBRDROIDDEPOTTRANSMITTER_H