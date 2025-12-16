#if !defined(BBRAXISINPUTMANAGER_H)
#define BBRAXISINPUTMANAGER_H

#include "BBRTypes.h"
#include <map>

namespace bb {
namespace rmt {

/**
 * Mix Manager class -- handles mixes and computes values.
 */
class MixManager {
public:
    static const MixManager InvalidManager;

    virtual uint8_t numMixes() const;
    const std::map<InputID,AxisMix>& mixes();
    virtual bool hasMixForInput(InputID input) const;
    virtual const AxisMix& mixForInput(uint8_t index) const;
    virtual bool setMix(InputID input, const AxisMix& mix);
    virtual void clearMixes();
    virtual void printDescription() const;

protected:
    std::map<InputID,AxisMix> mixes_;
};

}; // rmt
}; // bb

#endif // BBRAXISINPUTMANAGER_H