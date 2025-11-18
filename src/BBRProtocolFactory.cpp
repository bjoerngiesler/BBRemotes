#include "BBRProtocolFactory.h"
#include "CommercialBLE/DroidDepot/BBRDroidDepotProtocol.h"
#include "CommercialBLE/Sphero/BBRSpheroProtocol.h"
#include "MCS/ESP/BBRMESPProtocol.h"
#include "MCS/XBee/BBRMXBProtocol.h"

using namespace bb;
using namespace rmt;

bool dummyReadFn(ProtocolStorage& storage) { return false; }
bool dummyWriteFn(const ProtocolStorage& storage) { return false; }

static std::function<bool(ProtocolStorage&)> readFn_ = dummyReadFn;
static std::function<bool(const ProtocolStorage&)> writeFn_ = dummyWriteFn;
static ProtocolStorage storage_;
static bool needsRead_ = true;

void ProtocolFactory::setMemoryReadFunction(std::function<bool(ProtocolStorage&)> readFn) {
    readFn_ = readFn;;
}

void ProtocolFactory::setMemoryWriteFunction(std::function<bool(const ProtocolStorage&)> writeFn) {
    writeFn_ = writeFn;
}

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

std::vector<std::string> ProtocolFactory::storedProtocols() {
    if(needsRead_) {
        readFn_(storage_);
        needsRead_ = false;
    }

    std::vector<std::string> retval;
    for(int i=0; i<storage_.num; i++) {
        char buf[NAME_MAXLEN+1];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, storage_.blocks[i].name, NAME_MAXLEN);
        retval.push_back(buf);
    }

    return retval;
}

Protocol* ProtocolFactory::loadProtocol(const std::string& name) {
    if(needsRead_) {
        readFn_(storage_);
        needsRead_ = false;
    }
    return NULL;
}

bool ProtocolFactory::storeProtocol(const std::string& name, Protocol* proto) {
    StorageBlock block;
    memcpy(block.name, name.c_str(), name.size() < NAME_MAXLEN ? name.size() : NAME_MAXLEN);
    return false;
}

bool ProtocolFactory::eraseProtocol(const std::string& name) {
    return false;
}
