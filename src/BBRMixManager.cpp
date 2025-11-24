#include "BBRMixManager.h"
#include "BBRTypes.h"

using namespace bb;
using namespace bb::rmt;

static AxisMix InvalidMix;
const MixManager MixManager::InvalidManager;

uint8_t MixManager::numMixes() const { 
    return mixes_.size(); 
}

const std::map<InputID,AxisMix>& MixManager::mixes() {
    return mixes_;
}

const AxisMix& MixManager::mixForInput(InputID input) const { 
    for(auto& mix: mixes_) {
        if(mix.first == input) return mix.second;
    }
    return InvalidMix;
}

bool MixManager::setMix(InputID input, const AxisMix& mix) { 
    if(input == INPUT_INVALID) return false;
    mixes_[input] = mix;
    return true;
}

void MixManager::clearMixes() { 
    mixes_.clear(); 
}

bool MixManager::hasMixForInput(InputID input) const {
    for(auto& mix: mixes_) {
        if(mix.first == input) return true;
    }
    return false;
}

