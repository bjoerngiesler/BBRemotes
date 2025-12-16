#if !defined(BBRDROIDDEPOTPROTOCOL_H)
#define BBRDROIDDEPOTPROTOCOL_H

#if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_SAMD_MKRWIFI1010

#include <BLEAdvertisedDevice.h>

#include "../BBRBLEProtocol.h"

namespace bb {
namespace rmt {

//! Droid Depot BLE Protocol
class DroidDepotProtocol: public BLEProtocol {
public:
    DroidDepotProtocol();

    virtual ProtocolType protocolType() { return DROIDDEPOT_BLE; }

    virtual uint8_t numTransmitterTypes();

    virtual Transmitter* createTransmitter(uint8_t transmitterType=0);
    virtual Receiver* createReceiver() { return nullptr; }
    virtual Configurator* createConfigurator() { return nullptr; }

    virtual bool isAcceptableForDiscovery(BLEAdvertisedDevice advertisedDevice);
    virtual bool acceptsPairingRequests() { return false; }

    virtual bool pairWith(const NodeDescription& node);

    bool transmitCommand( uint8_t *payload, uint8_t len);

    virtual uint8_t numInputs(const NodeAddr& addr);
    virtual const std::string& inputName(const NodeAddr& addr, uint8_t input);
    virtual uint8_t inputWithName(const NodeAddr& addr, const std::string& name);

    enum Inputs {
        INPUT_SPEED       = 0,
        INPUT_TURN        = 1,
        INPUT_DOME        = 2,
        INPUT_SOUND1      = 3,
        INPUT_SOUND2      = 4,
        INPUT_SOUND3      = 5,
        INPUT_ACCESSORY   = 6
    };
    static const uint8_t NUM_INPUTS = 7;

    protected:
virtual bool connect(const NodeAddr& addr);
    bool initialWrites();

    static const std::vector<std::string> inputNames;

    BLERemoteCharacteristic *pCharacteristic_;
}; // class DroidDepotProtocol
}; // namespace rmt
}; // namespace bb

#endif // !CONFIG_IDF_TARGET_ESP32S2

#endif // #define BBRDROIDDEPOTPROTOCOL_H