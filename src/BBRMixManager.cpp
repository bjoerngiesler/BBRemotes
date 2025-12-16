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
    bb::rmt::printf("Setting mix for input %d\n", input);
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

void MixManager::printDescription() const {
    bb::rmt::printf("%d mixes\n", mixes_.size());
    for(auto& mix: mixes_) {
        bb::rmt::printf("Input %d: ", mix.first);
        const AxisMix& m = mix.second;
        if(m.axis1 == AXIS_INVALID) bb::rmt::printf("\tAxis 1: INVALID\n");
        else {
            bb::rmt::printf("\tAxis 1: %d ", m.axis1);
            bb::rmt::printf("Curve: 0\%->%d 25\%->%d 50\%->%d 75\%->%d 100\%->%d\n", 
                            m.interp1.i0, m.interp1.i25, m.interp1.i50, m.interp1.i75, m.interp1.i100);
        }
        if(m.axis2 == AXIS_INVALID) bb::rmt::printf("\t\tAxis 2: INVALID\n");
        else {
            bb::rmt::printf("\t\tAxis 2: %d ", m.axis2);
            bb::rmt::printf("Curve: 0\%->%d 25\%->%d 50\%->%d 75\%->%d 100\%->%d\n", 
                            m.interp2.i0, m.interp2.i25, m.interp2.i50, m.interp2.i75, m.interp2.i100);
        }

        switch(m.mixType) {
        case MIX_ADD: bb::rmt::printf("\t\tMix: Additive\n"); break;
        case MIX_MULT: bb::rmt::printf("\t\tMix: Multiplicative\n"); break;
        case MIX_NONE: bb::rmt::printf("\t\tMix: None\n"); break;
        }
    }
}
