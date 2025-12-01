#include "BBRProtocolFactory.h"
#include "CommercialBLE/DroidDepot/BBRDroidDepotProtocol.h"
#include "CommercialBLE/Sphero/BBRSpheroProtocol.h"
#include "MCS/ESP/BBRMESPProtocol.h"
#include "MCS/XBee/BBRMXBProtocol.h"
#include "MCS/Sat/BBRMSatProtocol.h"

using namespace bb;
using namespace rmt;

bool dummyReadFn(ProtocolStorage& storage) { return false; }
bool dummyWriteFn(const ProtocolStorage& storage) { return false; }

static std::function<bool(ProtocolStorage&)> readFn_ = dummyReadFn;
static std::function<bool(const ProtocolStorage&)> writeFn_ = dummyWriteFn;
static std::vector<std::function<void(Protocol*)>> newProtoCBs_;
static ProtocolStorage storage_;
static bool needsRead_ = true;
static std::map<ProtocolType, Protocol*> protocols_;

void ProtocolFactory::setMemoryReadFunction(std::function<bool(ProtocolStorage&)> readFn) {
    readFn_ = readFn;
}

void ProtocolFactory::setMemoryWriteFunction(std::function<bool(const ProtocolStorage&)> writeFn) {
    writeFn_ = writeFn;
}

Protocol* ProtocolFactory::getOrCreateProtocol(ProtocolType type) {
    Protocol *proto = protocols_[type];
    if(proto != nullptr) return proto;

    switch(type) {
    case MONACO_SAT:
        proto = new MSatProtocol;
        break;

    case MONACO_XBEE:
        proto = new MXBProtocol;
        break;
    
    case MONACO_ESPNOW:
#if ARDUINO_SAMD_MKRWIFI1010
        printf("Error creating Protocol: Target does not support ESPNow\n");
        return nullptr;
#else
        proto =  new MESPProtocol;
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
        printf("Error creating Protocol: MONACO_BLE not yet implemented\n");
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
        proto =  new SpheroProtocol;
#endif
        break;

    case DROIDDEPOT_BLE:
#if CONFIG_IDF_TARGET_ESP32S2 || ARDUINO_ARCH_SAMD
        printf("Error creating Protocol: Target does not support BLE\n");
        return nullptr;
#else
        proto =  new DroidDepotProtocol;
#endif
        break;

    default:
        printf("Error creating Protocol: Unknown protocol %c\n", type);
    }

    if(proto != nullptr) protocols_[type] = proto;

    return proto;
}

bool ProtocolFactory::destroyProtocol(Protocol** proto) {
    if(proto == nullptr) {
        printf("Refuse to destroy a NULL protocol\n");
        return false;
    }

    ProtocolType type = (*proto)->protocolType();

    Protocol *proto2 = protocols_[type];
    if(proto2 != (*proto)) {
        printf("ProtocolFactory doesn't own this protocol, refuse to destroy it\n");
        return false;
    }
    
    protocols_[type] = nullptr;
    delete *proto;
    *proto = nullptr;
    return true;
}

Protocol* ProtocolFactory::loadLastUsedProtocol() {
    return loadProtocol(lastUsedProtocolName());
}


Protocol* ProtocolFactory::loadProtocol(const std::string& name) {
    if(needsRead_) {
        readFn_(storage_);
        printStorage();
        needsRead_ = false;
    }

    unsigned int i=0;
    for(i=0; i<storage_.num; i++) {
        if(name == std::string(storage_.blocks[i].storageName)) break;
    }

    if(i>=storage_.num) { // not found 
        printf("Error: Couldn't find stored protocol \"%s\"\n", name.c_str());
        return nullptr;
    }

    ProtocolType type = storage_.blocks[i].type;
    Protocol *protocol = getOrCreateProtocol(type);

    if(protocol == nullptr) {
        printf("Error: Could not get or create protocol of type %c\n", type);
        return nullptr;
    }

    if(protocol->init(name) == false) {
        printf("Warning: Init returned false\n");
        return nullptr;
    }

    if(protocol->deserialize(storage_.blocks[i]) == false) {
        printf("Warning: Could not deserialize block %d into protocol\n");
    }

    return protocol;
}

std::vector<std::string> ProtocolFactory::storedProtocolNames() {
    if(needsRead_) {
        readFn_(storage_);
        printStorage();
        needsRead_ = false;
    }

    std::vector<std::string> retval;
    for(int i=0; i<storage_.num; i++) {
        retval.push_back(storage_.blocks[i].storageName);
    }

    return retval;
}

std::string ProtocolFactory::lastUsedProtocolName() {
    if(needsRead_) {
        readFn_(storage_);
        printStorage();
        needsRead_ = false;
    }

    return std::string(storage_.last);
}

bool ProtocolFactory::setLastUsedProtocolName(const std::string& name) {
    storage_.last = name;
    return true;
}


bool ProtocolFactory::storeProtocol(const std::string& name, Protocol* proto) {
    if(proto == nullptr) {
        printf("Not storing nullptr\n");
        return false;
    }

    bool bumpNum = false;
    
    StorageBlock *block = nullptr;
    for(unsigned int i=0; i<storage_.num; i++) {
        if(storage_.blocks[i].storageName == name) {
            block = &storage_.blocks[i];
            printf("Storing into existing block %d\n", i);
            break;
        }
    }
    if(block == nullptr) {
        if(storage_.num == ProtocolStorage::MAX_NUM_PROTOCOLS) {
            printf("Storage exhausted\n");
            return false;
        }
        printf("Storing into new block %d\n", storage_.num);
        block = &storage_.blocks[storage_.num];
        block->storageName = name;
        bumpNum = true;
    }

    block->storageName = name;
    if(proto->serialize(*block) == false) {
        printf("Error storing\n");
        return false;
    }

    if(bumpNum == true) storage_.num++;

    return true; //setLastUsedProtocolName(name);
}

bool ProtocolFactory::eraseProtocol(const std::string& name) {
    unsigned int i;

    for(i=0; i<storage_.num; i++) {
        if(storage_.blocks[i].storageName == name) {
            break;
        }
    }
    if(i >= storage_.num) {
        printf("Can't erase protocol %s -- can't find it.\n", name.c_str());
    }

    if(i+1 <= storage_.num) {
        memcpy(&(storage_.blocks[i]), &(storage_.blocks[i+1]), sizeof(StorageBlock)*(storage_.num - i - 1));
    }
    storage_.num--;

    printf("Erased OK\n");
    return true;
}

bool ProtocolFactory::eraseAll() {
    memset((void*)&storage_, 0, sizeof(ProtocolStorage));
    return true;
}

bool ProtocolFactory::commit() {
    if(writeFn_(storage_) == false) {
        printf("Error storing\n");
        return false;
    }

    printf("Stored OK\n");
    return true;
}


void ProtocolFactory::printStorage() {
    printf("%d protocols\n", storage_.num);
    printf("Last used protocol: '%s'\n", String(storage_.last).c_str());
    for(unsigned int i=0; i<storage_.num; i++) {
        const StorageBlock& block = storage_.blocks[i];

        printf("Protocol %d:\n", i);
        printf("  Storage name: '%s'\n", String(block.storageName).c_str());
        printf("  Node name: '%s'\n", String(block.nodeName).c_str());
        printf("  Protocol type: %c\n", block.type);
        printf("  %d paired nodes\n", block.numPairedNodes);
        for(unsigned int j=0; j<block.numPairedNodes; j++) {
            printf("  Paired node %d:\n", j);
            printf("    Name: '%s'\n", String(block.pairedNodes[j].name).c_str());
            printf("    Addr: %s\n", block.pairedNodes[j].addr.toString().c_str());
            printf("    Is Configurator: %s\n", block.pairedNodes[j].isConfigurator ? "yes" : "no");
            printf("    Is Transmitter: %s\n", block.pairedNodes[j].isTransmitter ? "yes" : "no");
            printf("    Is Receiver: %s\n", block.pairedNodes[j].isReceiver ? "yes" : "no");
        }
        printf("  %d input mappings\n", block.numMappings);
        for(unsigned int j=0; j<block.numMappings; j++) {
            printf("  Input mapping %d:\n", j);
            printf("    Addr: %s\n", block.mapping[j].addr.toString().c_str());
            printf("    Input: %d\n", block.mapping[j].input);
            printf("    Mix type: %d\n", block.mapping[j].mix.mixType);
            printf("    Axis 1: %d\n", block.mapping[j].mix.axis1);
            printf("    Axis 1 Interp: %d-%d-%d-%d\n", 
                   block.mapping[j].mix.interp1.i0, block.mapping[j].mix.interp1.i25, block.mapping[j].mix.interp1.i50,
                   block.mapping[j].mix.interp1.i75, block.mapping[j].mix.interp1.i100);
            printf("    Axis 2: %s\n", block.mapping[j].mix.axis2);
            printf("    Axis 2 Interp: %d-%d-%d-%d\n", 
                   block.mapping[j].mix.interp2.i0, block.mapping[j].mix.interp2.i25, block.mapping[j].mix.interp2.i50,
                   block.mapping[j].mix.interp2.i75, block.mapping[j].mix.interp2.i100);
        }
        printf("  Protocol specific block: ");
        for(unsigned int j=0; j<sizeof(block.protocolSpecific); j++) {
            printf("%02x ", block.protocolSpecific[j]);
            if(j%25 == 0) printf("\n");
        }
        printf("\n");
    }
}

