#include "BBRProtocolFactory.h"
#include "CommercialBLE/DroidDepot/BBRDroidDepotProtocol.h"
#include "CommercialBLE/Sphero/BBRSpheroProtocol.h"
#include "MCS/ESP/BBRMESPProtocol.h"
#include "MCS/XBee/BBRMXBProtocol.h"

using namespace bb;
using namespace rmt;

Protocol* ProtocolFactory::createProtocol(ProtocolType type) {
    switch(type) {
    case MONACO_XBEE:
        return new MXBProtocol;
        break;
    
    case MONACO_ESPNOW:
#if ARDUINO_SAMD_MKRWIFI1010
        printf("Error creating Protocol: Target does not support ESPNow\n");
        return nullptr;
#else
        return new MESPProtocol;
        break;
#endif

    case MONACO_UDP:
        printf("Error creating Protocol: MONACO_UDP not yet implemented\n");
        return nullptr;
        break;

    case MONACO_BLE:
#if CONFIG_IDF_TARGET_ESP32S2 || ARDUINO_SAMD_MKRWIFI1010
        printf("Error creating Protocol: Target does not support BLE\n");
        return nullptr;
#else
        printf("Error creating Protocol: MONACO_UDP not yet implemented\n");
        return nullptr;
#endif
        break;
        
    case SPEKTRUM_DSSS:
        printf("Error creating Protocol: SPEKTRUM_DSSS protocol not yet implemented\n");
        return nullptr;
        break;

    case SPHERO_BLE:
#if CONFIG_IDF_TARGET_ESP32S2 || ARDUINO_SAMD_MKRWIFI1010
        printf("Error creating Protocol: Target does not support BLE\n");
        return nullptr;
#else
        return new SpheroProtocol;
#endif
        break;

    case DROIDDEPOT_BLE:
#if CONFIG_IDF_TARGET_ESP32S2 || ARDUINO_ARCH_SAMD
        printf("Error creating Protocol: Target does not support BLE\n");
        return nullptr;
#else
        return new DroidDepotProtocol;
#endif
        break;

    default:
        printf("Error creating Protocol: Unknown protocol %d\n", type);
    }

    return nullptr;
}

bool ProtocolFactory::storeProtocolInfo(Protocol* protocol) {
    return true;
}

bool ProtocolFactory::eraseProtocolInfo() {
    return true;
}
