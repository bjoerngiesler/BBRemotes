#include <Arduino.h>

#if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_SAMD_MKRWIFI1010

#include "BBRDroidDepotTransmitter.h"
#include "BBRDroidDepotProtocol.h"
#include <string>
#include <vector>

using namespace bb;
using namespace bb::rmt;

#define BITDEPTH 8
#define MAXVAL ((1<<BITDEPTH)-1)

static std::vector<std::string> axisNames = {"CH0", "CH1", "CH2", "CH3", "CH4"};

static std::string invalidAxisName = "INVALID";

DroidDepotTransmitter::DroidDepotTransmitter(DroidDepotProtocol *proto): TransmitterBase<DroidDepotProtocol>(proto) {
    MixManager& mgr = proto->mixManager();
    mgr.setMix(DroidDepotProtocol::INPUT_SPEED,     AxisMix(0, INTERP_LIN_CENTERED));
    mgr.setMix(DroidDepotProtocol::INPUT_TURN,      AxisMix(1, INTERP_LIN_CENTERED));
    mgr.setMix(DroidDepotProtocol::INPUT_DOME,      AxisMix(2, INTERP_LIN_CENTERED));
    //mgr.setMix(DroidDepotProtocol::INPUT_SOUND1,    AxisMix(3, INTERP_LIN_POSITIVE));
    //mgr.setMix(DroidDepotProtocol::INPUT_SOUND2,    AxisMix(4, INTERP_LIN_POSITIVE));
    //mgr.setMix(DroidDepotProtocol::INPUT_SOUND3,    AxisMix(5, INTERP_LIN_POSITIVE));
    //mgr.setMix(DroidDepotProtocol::INPUT_ACCESSORY, AxisMix(6, INTERP_LIN_POSITIVE));
}

bool DroidDepotTransmitter::transmit() {
    static bool soundPressed[] = {false, false, false};

    const MixManager& mgr = protocol_->mixManager();
    //mgr.printDescription();
    for(int i=0; i<DroidDepotProtocol::NUM_INPUTS; i++) {
        if(!mgr.hasMixForInput(i)) {
            inputVals_[i] = 0;
            continue;
        }

        inputVals_[i] = computeMix(mgr.mixForInput(i));
    }

    float mot0 = (inputVals_[DroidDepotProtocol::INPUT_SPEED]-inputVals_[DroidDepotProtocol::INPUT_TURN]);
    float mot1 = (inputVals_[DroidDepotProtocol::INPUT_SPEED]+inputVals_[DroidDepotProtocol::INPUT_TURN]);
    float dome = inputVals_[DroidDepotProtocol::INPUT_DOME];
    float sound1 = inputVals_[DroidDepotProtocol::INPUT_DOME];
    float sound2 = inputVals_[DroidDepotProtocol::INPUT_DOME];
    float sound3 = inputVals_[DroidDepotProtocol::INPUT_DOME];
    float accessory = inputVals_[DroidDepotProtocol::INPUT_DOME];


    uint8_t mot0DirByte = (mot0 < 0 ? 0x80 : 0x00) | 0;
    uint8_t mot1DirByte = (mot1 < 0 ? 0x80 : 0x00) | 1;
    uint8_t mot2DirByte = (dome < 0 ? 0x80 : 0x00) | 2;
    uint8_t mot0PowerByte = fabs(mot0)*255;
    uint8_t mot1PowerByte = fabs(mot1)*255;
    uint8_t mot2PowerByte = fabs(dome) > 0.1 ? 0xa0 : 0x0;
    //bb::rmt::printf("mot0: %f mot1: %f dome: %f 0x%x 0x%x\n", mot0, mot1, dome, mot2DirByte, mot2PowerByte);

    uint8_t motCmd[] = {0x29, 0x42, 0x05, 0x46, mot0DirByte, mot0PowerByte, 0x01, 0x2C, 0x00, 0x00,
                          0x29, 0x42, 0x05, 0x46, mot1DirByte, mot1PowerByte, 0x01, 0x2C, 0x00, 0x00,
                          0x29, 0x42, 0x05, 0x46, mot2DirByte, mot2PowerByte, 0x01, 0x2C, 0x00, 0x00};

    protocol_->transmitCommand(motCmd, sizeof(motCmd));

    for(uint8_t i=0; i<3; i++) {
        if(soundPressed[i] == false && inputVals_[DroidDepotProtocol::INPUT_SOUND1+i] > 0.5) {
            bb::rmt::printf("Playing sound %d\n", i);
            soundPressed[i] = true;
            uint8_t sndCmd[] = {0x27, 0x42, 0x0F, 0x44, 0x44, 0x00, 0x1F, 0x00, 0x27, 0x42, 0x0F, 0x44, 0x44, 0x00, 0x18, i};
            protocol_->transmitCommand(sndCmd, sizeof(sndCmd));
        } else if(inputVals_[DroidDepotProtocol::INPUT_SOUND1+i] <= 127) soundPressed[i] = false;
    }

    return true;
}


#endif // !CONFIG_IDF_TARGET_ESP32S2