#include "BBRProtocol.h"
#include <limits.h> // for ULONG_MAX

using namespace bb;
using namespace rmt;

#if !defined(WRAPPEDDIFF)
#define WRAPPEDDIFF(a, b, max) ((a>=b) ? a-b : (max-b)+a)
#endif

static const std::string EMPTY("");

static unsigned long transmitUSGap_ = 40;

static MixManager invalidMgr = MixManager::InvalidManager;

void Protocol::setTransmitFrequencyHz(uint8_t transmitFrequencyHz) {
    transmitUSGap_ = 1000000 / transmitFrequencyHz;
}

Protocol::Protocol(): commTimeoutWD_(nullptr), telemReceivedCB_(nullptr) {
}

Protocol::~Protocol() {
    // notify people we're going away
    for(auto cb: destroyCBs_) {
        cb(this);
    }

    if(transmitter_ != nullptr) {
        delete transmitter_;
        transmitter_ = nullptr;
    }
    
    if(receiver_ != nullptr) {
        delete receiver_;
        receiver_ = nullptr;
    }
    
    //deinit();
}

void Protocol::deinit() {
}

bool Protocol::serialize(StorageBlock& block) {
    block.nodeName = nodeName_;
    block.type = protocolType();

    if(pairedNodes_.size() > StorageBlock::MAX_NUM_NODES) {
        printf("Warning: Can only store %d of %d paired nodes due to memory limitation\n", 
               StorageBlock::MAX_NUM_NODES, pairedNodes_.size());
    }
    block.numPairedNodes = pairedNodes_.size() < StorageBlock::MAX_NUM_NODES ? pairedNodes_.size() : StorageBlock::MAX_NUM_NODES;
    for(int i=0; i<block.numPairedNodes; i++) {
        block.pairedNodes[i] = pairedNodes_[i];
    }
    
    int mixmapping = 0;
    for(auto pair1: mixManagers_) {
        for(auto pair2: pair1.second.mixes()) {
            block.mapping[mixmapping].addr = pair1.first;
            block.mapping[mixmapping].input = pair2.first;
            block.mapping[mixmapping].mix = pair2.second;
            mixmapping++;
            if(mixmapping >= StorageBlock::MAX_NUM_MAPPINGS) {
                printf("Warning: Can only store %d mix mappings due to memory limitation\n", 
                       StorageBlock::MAX_NUM_MAPPINGS);
                break;
            }
        }
    }
    block.numMappings = mixmapping;

    return true;
}

bool Protocol::deserialize(StorageBlock& block) {
    if(protocolType() != block.type) {
        printf("Error: Can't load storage of type %c into %c\n",
               block.type, protocolType());
        return false;
    }

    pairedNodes_.clear();
    for(unsigned int i=0; i<block.numPairedNodes; i++) {
        pairedNodes_.push_back(block.pairedNodes[i]);
    }

    #if 0 // FIXME
    mixManagers_.clear();
    for(unsigned int i=0; i<block.numMappings; i++) {
        if(mixManager(block.mapping[i].addr) == MixManager::InvalidManager) {
            mixManagers_[block.mapping[i].addr] = MixManager();
        }
        mixManager(block.mapping[i].addr).setMix(block.mapping[i].input, block.mapping[i].mix);
    }
    #endif

    return true;
}

Transmitter* Protocol::createTransmitter(uint8_t transmitterType) { 
    return nullptr; 
}

Receiver* Protocol::createReceiver() { 
    return nullptr; 
}

unsigned int Protocol::numDiscoveredNodes() { 
    return discoveredNodes_.size(); 
}

const NodeDescription& Protocol::discoveredNode(unsigned int index) {
    static NodeDescription nil;
    if(index >= discoveredNodes_.size()) return nil;
    return discoveredNodes_[index];
}

bool Protocol::isPaired() { 
    return pairedNodes_.size() != 0;
}

bool Protocol::isPaired(const NodeAddr& addr) { 
    for(auto& d: pairedNodes_) {
        if(d.addr == addr) return true;
    }

    return false;
}

bool Protocol::pairWith(const NodeDescription& node) {
    if(isPaired(node.addr)) {
        printf("Already paired with %s\n", node.addr.toString().c_str());
        return true; // already paired
    }
    printf("Now paired with %s (c:%d r:%d t:%d)\n", node.addr.toString().c_str(), node.isConfigurator, node.isReceiver, node.isTransmitter);
    pairedNodes_.push_back(node);
    return true;
}

bool Protocol::rePairWithConnected(bool receivers, bool transmitters, bool configurators) {
    bool success = true;
    for(auto& n: pairedNodes_) {
        if((n.isTransmitter && transmitters) || (n.isReceiver && receivers) || (n.isConfigurator && configurators)) {
            bb::rmt::printf("Re-pairing with %s\n", n.addr.toString().c_str());
            if(pairWith(n) == false) success = false;
        }
    }
    return success;
}

    
bool Protocol::step() {
    //if(protocolType() == DROIDDEPOT_BLE) ::rmt::printf("step() in %c\n", protocolType());

    float secondsSinceLastComm = float(WRAPPEDDIFF(millis(), lastCommHappenedMS_, ULONG_MAX)) / 1000.0f;
    if(secondsSinceLastComm > commTimeoutSeconds_ && commTimeoutWD_ != nullptr) {
        commTimeoutWD_(this, secondsSinceLastComm);
    }

    bool retval = true;
    if(WRAPPEDDIFF(micros(), usLastTransmit_, ULONG_MAX) > transmitUSGap_) {
        if(transmitter_ != nullptr) {
            //if(protocolType() == DROIDDEPOT_BLE) bb::rmt::printf("transmitting in %c\n", protocolType());
            if(transmitter_->transmit() == false) retval = false;
            usLastTransmit_ = micros();
        } 
    } 

    return retval;
}

bool Protocol::sendTelemetry(const Telemetry& telem) {
    bool success = (pairedNodes_.size() != 0);
    for(auto& n: pairedNodes_) {
        if(n.isConfigurator) {
            if(sendTelemetry(n.addr, telem) == false) success = false;
        }
    }
    return success;
}

bool Protocol::sendTelemetry(const NodeAddr& configuratorAddr, const Telemetry& telem) {
    bb::rmt::printf("Dummy send Telemetry -- if this is called you did something wrong\n");
    return false;
}

void Protocol::setTelemetryReceivedCB(std::function<void(Protocol*, const NodeAddr&, uint8_t, const Telemetry&)> cb) {
    telemReceivedCB_ = cb;
}

void Protocol::telemetryReceived(const NodeAddr& source, uint8_t seqnum, const Telemetry& telem) {
    if(telemReceivedCB_ != nullptr) telemReceivedCB_(this, source, seqnum, telem);
}


uint8_t Protocol::numInputs(const NodeAddr& addr) {
    for(auto& pair: inputs_) {
        if(addr == pair.first) {
            return pair.second.size();
        }
    }
    return 0;
}

const std::string& Protocol::inputName(const NodeAddr& addr, InputID input) {
    for(auto& pair: inputs_) {
        if(addr == pair.first) {
            if(input < pair.second.size()) return pair.second[input];
            return EMPTY;
        }
    }
    return EMPTY;
}

InputID Protocol::inputWithName(const NodeAddr& addr, const std::string& name) {
    for(auto& pair: inputs_) {
        if(addr == pair.first) {
            for(unsigned int i=0; i<pair.second.size(); i++) {
                if(pair.second[i] == name) return i;
            }
            return INPUT_INVALID;
        }
    }
    return INPUT_INVALID;
}

MixManager& Protocol::mixManager(const NodeAddr& addr) {
    return mixManagers_[addr];
}

MixManager& Protocol::mixManager() {
    if(pairedNodes_.size() != 1)  return invalidMgr;
    return mixManager(pairedNodes_[0].addr);
}

bool Protocol::retrieveInputs() {
    for(auto& d: pairedNodes_) {
        printf("Protocol: Retrieving inputs in %s\n", d.addr.toString().c_str());
        if(d.isReceiver) retrieveInputs(d);
    }
    return true;
}

bool Protocol::retrieveMixes() {
    for(auto& d: pairedNodes_) {
        if(d.isReceiver) retrieveMixes(d);
    }
    return true;
}

bool Protocol::sendMixes() {
    for(auto& d: pairedNodes_) {
        if(d.isReceiver) sendMixes(d);
    }
    return true;
}

void Protocol::addDestroyCB(std::function<void(Protocol*)> fn) {
    destroyCBs_.push_back(fn);
}

static void printNodeDescription(const NodeDescription& nd, std::string linePrefix = "") {
    bb::rmt::printf("%sNode \"%s\" at %s: ", linePrefix.c_str(), std::string(nd.name).c_str(), nd.addr.toString().c_str());
    if(nd.isTransmitter) bb::rmt::printf("transmitter, ");
    if(nd.isReceiver) bb::rmt::printf("receiver, ");
    if(nd.isConfigurator) bb::rmt::printf("configurator, ");
    bb::rmt::printf("protocol specific: 0x%x.\n", nd.protoSpecific);
}

void Protocol::printInfo() {;
    bb::rmt::printf("Info on protocol type %c, node name \"%s\":\n", 
        protocolType(), nodeName_.c_str());
    if(pairedNodes_.size() == 0) {
        bb::rmt::printf("\tNo paired nodes\n");
    } else {
        bb::rmt::printf("\t%d paired nodes:\n", pairedNodes_.size());
        for(auto &nd: pairedNodes_) {
            printNodeDescription(nd, "\t\t");
        }
    }
    if(discoveredNodes_.size() == 0) {
        bb::rmt::printf("\tNo discovered nodes\n");
    } else {
        bb::rmt::printf("\t%d discovered nodes:\n", discoveredNodes_.size());
        for(auto &nd: discoveredNodes_) {
            printNodeDescription(nd, "\t\t");
        }
    }
    if(transmitter_ != nullptr) bb::rmt::printf("This protocol has a / is a transmitter.\n");
    if(receiver_ != nullptr) bb::rmt::printf("This protocol has a / is a receiver.\n");
    if(configurator_ != nullptr) bb::rmt::printf("This protocol is a configurator.\n");

    bb::rmt::printf("This protocol has %d inputs and %d mix managers.\n", inputs_.size(), mixManagers_.size());
    bb::rmt::printf("This protocol has %d registered destroy callbacks.\n", destroyCBs_.size());
    bb::rmt::printf("This protocol is stored as \"%s\".\n", storageName_.c_str());
}

void Protocol::setCommTimeoutWatchdog(float seconds, std::function<void(Protocol*,float)> commTimeoutWD) {
    commTimeoutWD_ = commTimeoutWD;
    commTimeoutSeconds_ = seconds;
}

void Protocol::commHappened() {
    lastCommHappenedMS_ = millis();
}
