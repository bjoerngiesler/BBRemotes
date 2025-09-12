#if !defined(BBRAXISINPUTMANAGER_H)
#define BBRAXISINPUTMANAGER_H

#include "BBRTypes.h"
#include <map>

namespace bb {
namespace rmt {

class MixManager {
public:
    static const MixManager InvalidManager;

    virtual uint8_t numMixes() const;
    virtual bool hasMixForInput(InputID input) const;
    virtual const AxisMix& mixForInput(uint8_t index) const;
    virtual bool setMix(InputID input, const AxisMix& mix);
    virtual void clearMixes();

protected:
    std::map<InputID,AxisMix> mixes_;
};

}; // rmt
}; // bb

#endif // BBRAXISINPUTMANAGER_H