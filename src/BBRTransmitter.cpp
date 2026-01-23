#include "BBRTransmitter.h"

using namespace bb;
using namespace bb::rmt;

static std::string InvalidString = "INVALID";

uint8_t Transmitter::numAxes() {
    return axes_.size();
}

const std::string& Transmitter::axisName(AxisID axis) {
    if(axis >= axes_.size()) return InvalidString;
    return axes_[axis].name;
}

bool Transmitter::setAxisName(AxisID axis, const std::string& name) {
    if(axis >= axes_.size()) return false;
    axes_[axis].name = name;
    return true;
}

uint8_t Transmitter::axisWithName(const std::string& name, bool add, uint8_t bitDepth) {
    for(int i=0; i<numAxes(); i++) {
        if(axisName(i) == name) return i;
    }

    if(add == false || canAddAxes() == false) {
        return AXIS_INVALID;
    }

    return addAxis(name, bitDepth);
}

uint8_t Transmitter::bitDepthForAxis(AxisID axis) {
    if(axis >= axes_.size()) return 0;
    return axes_[axis].bitDepth; 
}

uint8_t Transmitter::addAxis(const std::string& name, uint8_t bitDepth) {
    if(canAddAxes() == false) return AXIS_INVALID;
    Axis axis = {name, bitDepth, 0};
    axes_.push_back(axis);
    return axes_.size()-1;
}

bool Transmitter::setAxisValue(AxisID axis, float value, Unit unit) {
    if(axis >= axes_.size()) return false;
    float maxval = (1<<bitDepthForAxis(axis))-1;
    switch(unit) {
    case UNIT_DEGREES:
        value = (value/360.0f) * maxval;
        break;
    case UNIT_DEGREES_CENTERED:
        value = ((value+180.0f)/360.0f) * maxval;
        break;
    case UNIT_UNITY_CENTERED:
        value = ((value + 1.0f)/2.0f) * maxval;
        break;
    case UNIT_UNITY:
        value = value * maxval;
        break;
    case UNIT_RAW:
        break;
    default:
        break;
    }

    value = constrain(value, 0.0f, maxval);
    //Serial.printf("setAxisValue: %f\n", value);
    setRawAxisValue(axis, value);
    return true;
}

float Transmitter::axisValue(AxisID axis, Unit unit) {
    if(axis >= axes_.size()) return 0;
    uint32_t raw = rawAxisValue(axis);
    if(unit == UNIT_RAW) return raw;

#if 0
    for(unsigned int i=0; i<axes_.size(); i++) {
        Serial.printf("Axis %d: bitdepth %d\n", i, axes_[i].bitDepth);
    }
#endif

    uint32_t maxval = (1<<bitDepthForAxis(axis))-1;
    float normed = float(double(raw)/double(maxval)); // double here because of potentially big numbers.
    //Serial.printf("raw: %d maxval: %d normed: %f axis: %d bitdepth: %d\n", raw, maxval, normed, axis, bitDepthForAxis(axis));
    switch(unit) {
    case UNIT_DEGREES:
        return normed * 360.0f;
        break;
    case UNIT_DEGREES_CENTERED: 
        return (normed * 360.0f)-180.0f;
        break;
    case UNIT_UNITY_CENTERED:
        return (normed-0.5)*2.0f;
        break;
    case UNIT_UNITY:
        return normed;
        break;
    default:
        return 0.0f;
        break;
    }
}

bool Transmitter::setRawAxisValue(AxisID axis, uint32_t value) {
    if(axis >= axes_.size()) return false;
    uint32_t maxval = (1 << axes_[axis].bitDepth)-1;
    if(value > maxval) value = maxval;
    //Serial.printf("setRawAxisValue: %d\n", value);
    axes_[axis].value = value;
    return true;
}

uint32_t Transmitter::rawAxisValue(AxisID axis) {
    if(axis >= axes_.size()) return 0;
    return axes_[axis].value;
}

float Transmitter::computeMix(const AxisMix& mix) {
    float v1 = axisValue(mix.axis1, UNIT_UNITY);
    float v2 = axisValue(mix.axis2, UNIT_UNITY);
    //bb::rmt::printf("Ax1: %d Ax2: %d V1: %f V2: %f\n", mix.axis1, mix.axis2, v1, v2);
    return mix.compute(v1, 0.0f, 1.0f, v2, 0.0f, 1.0f);
}

