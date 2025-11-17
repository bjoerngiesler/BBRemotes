#include "BBRProtocol.h"

using namespace bb;
using namespace rmt;

static const std::string EMPTY("");

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
    
bool Protocol::step() {
    bool retval = true;
    if(transmitter_ != nullptr) {
        if(transmitter_->transmit() == false) retval = false;
    }
    return retval;
}

uint8_t Protocol::numInputs(const NodeAddr& addr) {
    for(auto& pair: inputs_) {
        if(addr == pair.first) {
            return pair.second.size();
        }
    }
    return 0;
}

const std::string& Protocol::inputName(const NodeAddr& addr, uint8_t input) {
    for(auto& pair: inputs_) {
        if(addr == pair.first) {
            if(input < pair.second.size()) return pair.second[input];
            return EMPTY;
        }
    }
    return EMPTY;
}

uint8_t Protocol::inputWithName(const NodeAddr& addr, const std::string& name) {
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

const MixManager& Protocol::mixManager(const NodeAddr& addr) {
    for(auto& pair: mixManagers_) {
        if(pair.first == addr) return pair.second;
    }
    return MixManager::InvalidManager;
}

const MixManager& Protocol::mixManager() {
    if(pairedNodes_.size() != 1)  return MixManager::InvalidManager;
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
