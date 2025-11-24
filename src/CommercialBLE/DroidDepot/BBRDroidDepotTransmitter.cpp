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
static std::vector<std::string> inputNames = {"speed", "turn", "dome", "sound", "accessory"};
enum Inputs {
    INPUT_VEL     = 0,
    INPUT_TURN      = 1,
    INPUT_DOME      = 2,
    INPUT_SOUND     = 3,
    INPUT_ACCESSORY = 4
};

static std::string invalidAxisName = "INVALID";

DroidDepotTransmitter::DroidDepotTransmitter(DroidDepotProtocol *proto): TransmitterBase<DroidDepotProtocol>(proto) {
    axes_ = {{"turn", 8, 0}, {"speed", 8, 0}, {"dome1", 8, 0}, {"dome2", 8, 0}, {"dome3", 8, 0}, {"sound1", 1, 0}, {"sound2", 1, 0}, {"soundX", 1, 0}, {"sound3", 1, 0}};
}

uint8_t DroidDepotTransmitter::numInputs() {
    return inputNames.size();
}

const std::string& DroidDepotTransmitter::inputName(uint8_t input) {
    return inputNames[input];
}

bool DroidDepotTransmitter::transmit() {
    static bool soundPressed[] = {false, false, false};
    AxisMix mix;

    const MixManager& mgr = protocol_->mixManager();
    if(protocol_->pairedNodes().size() == 0) return false;
    const NodeDescription& descr = protocol_->pairedNodes()[0];
    const NodeAddr& addr = descr.addr;

    float speed=0, turn=0, dome1=0, dome2=0, dome3=0, sound[] = {0, 0, 0};

    turn = axisValue(0, UNIT_UNITY_CENTERED)*255;
    speed = axisValue(1, UNIT_UNITY_CENTERED)*255;
    dome1 = axisValue(2, UNIT_UNITY_CENTERED)*255;
    dome2 = axisValue(2, UNIT_UNITY_CENTERED)*255;
    dome3 = axisValue(2, UNIT_UNITY_CENTERED)*255;
    sound[0] = axisValue(5, UNIT_UNITY)*255;
    sound[1] = axisValue(6, UNIT_UNITY)*255;
    sound[2] = axisValue(8, UNIT_UNITY)*255;


    float mot0 = (speed+turn), mot1 = (speed-turn);

    uint8_t mot0DirByte = (mot0 < 0 ? 0x80 : 0x00) | 0;
    uint8_t mot1DirByte = (mot1 < 0 ? 0x80 : 0x00) | 1;
    uint8_t mot2DirByte = (dome2 < 0 ? 0x80 : 0x00) | 2;
    uint8_t mot0PowerByte = fabs(mot0);
    uint8_t mot1PowerByte = fabs(mot1);
    uint8_t mot2PowerByte = fabs(dome2) > 2.0 ? 0xa0 : 0x0;
    //bb::rmt::printf("speed: %f turn: %f dome: %f 0x%x 0x%x\n", speed, turn, dome2, mot2DirByte, mot2PowerByte);

    uint8_t motCmd[] = {0x29, 0x42, 0x05, 0x46, mot0DirByte, mot0PowerByte, 0x01, 0x2C, 0x00, 0x00,
                          0x29, 0x42, 0x05, 0x46, mot1DirByte, mot1PowerByte, 0x01, 0x2C, 0x00, 0x00,
                          0x29, 0x42, 0x05, 0x46, mot2DirByte, mot2PowerByte, 0x01, 0x2C, 0x00, 0x00};

    protocol_->transmitCommand(motCmd, sizeof(motCmd));

    for(uint8_t i=0; i<sizeof(soundPressed); i++) {
        if(soundPressed[i] == false && sound[i] > 127) {
            bb::rmt::printf("Playing sound %d\n", i);
            soundPressed[i] = true;
            uint8_t sndCmd[] = {0x27, 0x42, 0x0F, 0x44, 0x44, 0x00, 0x1F, 0x00, 0x27, 0x42, 0x0F, 0x44, 0x44, 0x00, 0x18, i};
            protocol_->transmitCommand(sndCmd, sizeof(sndCmd));
        } else if(sound[i] <= 127) soundPressed[i] = false;
    }

    return true;
}


#endif // !CONFIG_IDF_TARGET_ESP32S2