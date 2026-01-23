#include <Arduino.h>

#if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_SAMD_MKRWIFI1010

#include "BBRSpheroTransmitter.h"
#include "BBRSpheroProtocol.h"

#include <string>
#include <vector>
#include <map>

using namespace bb;
using namespace bb::rmt;

static const SpheroProtocol::Command CMD_WAKEUP = {0x0a, 0x13, 0x0d}; // no args
static const SpheroProtocol::Command CMD_SLEEP = {0x0a, 0x13, 0x01};  // no args
static const SpheroProtocol::Command CMD_MOVE = {0x0a, 0x16, 0x07};   // 6 bytes: uint8_t speed, float32 degree, 0x00
static const SpheroProtocol::Command CMD_ANIM = {0x0a, 0x17, 0x05};   // 2 bytes: 0x00, num
static const SpheroProtocol::Command CMD_232 = {0x0a, 0x17, 0x0d};    // 0x01 -- 3-leg mode, 0x02 -- 2-leg mode
static const SpheroProtocol::Command CMD_DOME = {0x0a, 0x17, 0x0f};   // 4 bytes: float32 degree
static const SpheroProtocol::Command CMD_HOLO = {0x0a, 0x1a, 0x0e};   // 3 bytes: 0x00, 0x80, intensity


static const std::string EmptyString = "";
static std::string invalidAxisName = "INVALID";

SpheroTransmitter::SpheroTransmitter(SpheroProtocol *proto): TransmitterBase<SpheroProtocol>(proto) {
    lastEmote0_ = 0;
    lastEmote1_ = 0;
    lastEmote2_ = 0;
    lastEmote3_ = 0;
    lastEmote4_ = 0;

    sleeping_ = true;
    connected_ = false;
    turnAngle_ = 0;
}

bool SpheroTransmitter::transmit() {
    float dome = 0;
    const MixManager& mgr = protocol_->mixManager();
    if(protocol_->pairedNodes().size() == 0) return false;

    const NodeDescription& descr = protocol_->pairedNodes()[0];
    const NodeAddr& addr = descr.addr;
    AxisMix mix;

    if((mix = mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_DOME_ANGLE))).isValid()) {
        dome = computeMix(mix) * 180.0f;
    } else if((mix = mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_DOME_RATE))).isValid()) {
        domeAngle_ += computeMix(mix) * 1.8f;
        if(domeAngle_ > 100) domeAngle_ = 100;
        if(domeAngle_ < -100) domeAngle_ = -100;
        dome = domeAngle_;
    }
    uint8_t floatBuf[4];
    floatToBuf(dome, floatBuf);
    protocol_->transmitCommand(CMD_DOME, floatBuf, 4);

    float speed = computeMix(mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_SPEED))) * 255.0f;
    float turn = 0;
    if((mix = mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_TURN_ANGLE))).isValid()) {
        turn = computeMix(mix) * 360.0f;
    } else if((mix = mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_TURN_RATE))).isValid()) {
        turnAngle_ += computeMix(mix) * 3.6f;
        if(turnAngle_ > 360) turnAngle_ -= 360;
        if(turnAngle_ < 0) turnAngle_ += 360;
        turn = turnAngle_;
    }
    uint8_t moveBuf[4];
    moveBuf[0] = uint8_t(fabs(speed));
    if(turn > 255) {
        moveBuf[1] = 1;
        turn -= 256;
    } else moveBuf[1] = 0;
    moveBuf[2] = uint8_t(fabs(turn));
    moveBuf[3] = speed < 0 ? 1 : 0;
    protocol_->transmitCommand(CMD_MOVE, moveBuf, 4);

    uint8_t emoteBuf[2];
    float emote;

    emote = computeMix(mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_EMOTE_0)));
    if(emote > 0.5 && emote != lastEmote0_) {
        emoteBuf[0] = 0; 
        emoteBuf[1] = 0;
        protocol_->transmitCommand(CMD_ANIM, emoteBuf, 2);
    }
    lastEmote0_ = emote;

    emote = computeMix(mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_EMOTE_1)));
    if(emote > 0.5 && emote != lastEmote1_) {
        emoteBuf[0] = 0; 
        emoteBuf[1] = 1;
        protocol_->transmitCommand(CMD_ANIM, emoteBuf, 2);
    }
    lastEmote1_ = emote;

    emote = computeMix(mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_EMOTE_2)));
    if(emote > 0.5 && emote != lastEmote2_) {
        emoteBuf[0] = 0; 
        emoteBuf[1] = 2;
        protocol_->transmitCommand(CMD_ANIM, emoteBuf, 2);
    }
    lastEmote2_ = emote;

    emote = computeMix(mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_EMOTE_3)));
    if(emote > 0.5 && emote != lastEmote3_) {
        emoteBuf[0] = 0; 
        emoteBuf[1] = 0;
        protocol_->transmitCommand(CMD_ANIM, emoteBuf, 2);
    }
    lastEmote3_ = emote;

    emote = computeMix(mgr.mixForInput(protocol_->inputWithName(addr, INPUT_NAME_EMOTE_4)));
    if(emote > 0.5 && emote != lastEmote4_) {
        emoteBuf[0] = 0; 
        emoteBuf[1] = 0;
        protocol_->transmitCommand(CMD_ANIM, emoteBuf, 2);
    }
    lastEmote4_ = emote;
    
    return true;
}

bool SpheroTransmitter::sleep(bool onoff) {
    SpheroProtocol* p = (SpheroProtocol*)protocol_;

    if(onoff) {
        p->transmitCommand(CMD_SLEEP, nullptr, 0);
        sleeping_ = true;
    } else {
        p->transmitCommand(CMD_WAKEUP, nullptr, 0);
        sleeping_ = false;
    }
    
    return true;
}

void SpheroTransmitter::floatToBuf(float f, uint8_t *buf) {
    buf[0] = ((uint8_t*)&f)[3];
    buf[1] = ((uint8_t*)&f)[2];
    buf[2] = ((uint8_t*)&f)[1];
    buf[3] = ((uint8_t*)&f)[0];
}

#endif // !CONFIG_IDF_TARGET_ESP32S2