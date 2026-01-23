#include "BBRTypes.h"

using namespace bb;
using namespace bb::rmt;

void MaxlenString::zero() {
    for(unsigned int i=0; i<NAME_MAXLEN; i++) buf[i]=0;
}

MaxlenString& MaxlenString::operator=(const std::string& str) {
    unsigned int l = str.size();
    for(unsigned int i=0; i<NAME_MAXLEN; i++) {
        if(i<l) buf[i]=str[i];
        else buf[i]=0;
    }
    return *this;
}

MaxlenString& MaxlenString::operator=(const String& str) {
    unsigned int l = str.length();
    for(unsigned int i=0; i<NAME_MAXLEN; i++) {
        if(i<l) buf[i]=str[i];
        else buf[i]=0;
    }
    return *this;
}

MaxlenString& MaxlenString::operator=(const char *str) {
    unsigned int l = strlen(str);
    for(unsigned int i=0; i<NAME_MAXLEN; i++) {
        if(i<l) buf[i]=str[i];
        else buf[i]=0;
    }
    return *this;
}

MaxlenString::operator std::string() const {
    char retbuf[NAME_MAXLEN+1];
    for(unsigned int i=0; i<NAME_MAXLEN; i++) {
        retbuf[i]=buf[i];
    }
    retbuf[NAME_MAXLEN] = 0;

    return std::string(retbuf);
}

MaxlenString::operator String() const {
    char retbuf[NAME_MAXLEN+1];
    for(unsigned int i=0; i<NAME_MAXLEN; i++) {
        retbuf[i]=buf[i];
    }
    retbuf[NAME_MAXLEN] = 0;

    return String(retbuf);
}

bool MaxlenString::operator==(const std::string& other) const {
    unsigned int l = other.size();
    for(unsigned int i=0; i<NAME_MAXLEN; i++) {
        if(i<l && buf[i]!=other[i]) return false;
        else if(i>=l && buf[i]!=0) return false;
    }
    return true;
}
    
bool MaxlenString::operator==(const String& other) const {
    unsigned int l = other.length();
    for(unsigned int i=0; i<NAME_MAXLEN; i++) {
        if(i<l && buf[i]!=other[i]) return false;
        else if(i>=l && buf[i]!=0) return false; 
    }
    return true;
}

uint32_t NodeAddr::addrHi() const { 
    return byte[4] | (byte[5]<<8) | (byte[6]<<16) | (byte[7])<<24; 
}

uint32_t NodeAddr::addrLo() const { 
    return byte[0] | (byte[1]<<8) | (byte[2]<<16) | (byte[3])<<24; 
}

bool NodeAddr::isZero() const { 
    for(int i=0; i<8; i++) if(byte[i] != 0) return false;
    return true;
}
bool NodeAddr::operator==(const NodeAddr& other) const { 
    for(int i=0; i<8; i++) if(byte[i] != other.byte[i]) return false;
    return true;
}
bool NodeAddr::operator!=(const NodeAddr& other) const { 
    for(int i=0; i<8; i++) if(byte[i] != other.byte[i]) return true;
    return false;
}

void NodeAddr::fromXBeeAddress(uint32_t addrHi, uint32_t addrLo) {
    byte[0] = addrLo & 0xff;
    byte[1] = (addrLo>>8) & 0xff;
    byte[2] = (addrLo>>16) & 0xff;
    byte[3] = (addrLo>>24) & 0xff;
    byte[4] = addrHi & 0xff;
    byte[5] = (addrHi>>8) & 0xff;
    byte[6] = (addrHi>>16) & 0xff;
    byte[7] = (addrHi>>24) & 0xff;
    //for(int i=0; i<8; i++) printf("%x:", byte[i]);
    //printf("\n");
}

void NodeAddr::fromMACAddress(const uint8_t addr[6]) {
    for(int i=0; i<6; i++) byte[i] = addr[i];
    byte[6] = byte[7] = 0;
}

void NodeAddr::fromString(const std::string& str) {
    if(str.length() == 17 && 
        str[2] == ':' && str[5] == ':' && str[8] == ':' && str[11] == ':' && str[14] == ':') { // MAC address
        uint8_t m[6];
        sscanf(str.c_str(), "%x:%x:%x:%x:%x:%x", 
                (unsigned int*)&m[0], (unsigned int*)&m[1], (unsigned int*)&m[2], 
                (unsigned int*)&m[3], (unsigned int*)&m[4], (unsigned int*)&m[5]);
        fromMACAddress(m);
    } else if(str.length() == 17 && str[8] == ':' && str[2] != ':') {
        uint32_t hi, lo;
        sscanf(str.c_str(), "%lx:%lx", &hi, &lo);
        fromXBeeAddress(hi, lo);
    } else {
        for(int i=0; i<8; i++) byte[i] = 0;
    }
}

std::string NodeAddr::toString() const {
    char buf[20] = "";
    if(byte[6] == 0 && byte[7] == 0) {
        sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", byte[0], byte[1], byte[2], byte[3], byte[4], byte[5]);
    } else {
        sprintf(buf, "%lx:%lx", addrHi(), addrLo());
    }
    return buf;
}

bool bb::rmt::operator<(const NodeAddr& a1, const NodeAddr& a2) {
    for(uint8_t i = 0; i<8; i++) {
        if(a1.byte[i] < a2.byte[i]) return true;
    }
    return false;
}

AxisMix::AxisMix() { 
    interp1 = INTERP_ZERO;
    interp2 = INTERP_ZERO;
    axis1 = AXIS_INVALID; 
    axis2 = AXIS_INVALID;
    mixType = MIX_NONE;
}

AxisMix::AxisMix(AxisID ax1, Interpolator ip1, AxisID ax2, Interpolator ip2, MixType mix) { 
    axis1 = ax1; interp1 = ip1;
    axis2 = ax2; interp2 = ip2;
    mixType = mix;
}

bool AxisMix::isValid() { 
    return axis1 != AXIS_INVALID && axis2 != AXIS_INVALID && mixType != MIX_NONE; 
}

float AxisMix::compute(float a1, float min1, float max1, float a2, float min2, float max2) const {
    float i0=0, i1=0, frac=0, b1=0, b2=0;
    
    if(axis1 == AXIS_INVALID && axis2 == AXIS_INVALID) return 0;

    frac = (a1-min1)/(max1-min1);
    if(frac >= 0 && frac <= 0.25) {
        i0 = float(interp1.i0)/100.0f;
        i1 = float(interp1.i25)/100.0f;
        frac = frac * 4;
    } else if(frac > 0.25 && frac <= 0.5) {
        i0 = float(interp1.i25)/100.0f;
        i1 = float(interp1.i50)/100.0f;
        frac = (frac-0.25)*4;
    } else if(frac > 0.5 && frac <= 0.75) {
        i0 = float(interp1.i50)/100.0f;
        i1 = float(interp1.i75)/100.0f;
        frac = (frac-0.5)*4;
    } else if(frac > 0.75 && frac <= 1) {
        i0 = float(interp1.i75)/100.0f;
        i1 = float(interp1.i100)/100.0f;
        frac = (frac-0.75)*4;
    }
    b1 = i0 + (i1-i0)*frac;

    frac = (a2-min2)/(max2-min2);
    if(frac >= 0 && frac <= 0.25) {
        i0 = float(interp2.i0)/100.0f;
        i1 = float(interp2.i25)/100.0f;
        frac = frac * 4;
    } else if(frac > 0.25 && frac <= 0.5) {
        i0 = float(interp2.i25)/100.0f;
        i1 = float(interp2.i50)/100.0f;
        frac = (frac-0.25)*4;
    } else if(frac > 0.5 && frac <= 0.75) {
        i0 = float(interp2.i50)/100.0f;
        i1 = float(interp2.i75)/100.0f;
        frac = (frac-0.5)*4;
    } else if(frac > 0.75 && frac <= 1) {
        i0 = float(interp2.i75)/100.0f;
        i1 = float(interp2.i100)/100.0f;
        frac = (frac-0.75)*4;
    }
    b2 = i0 + (i1-i0)*frac;

    // we capture the case of both invalid at the beginning of the function
    if(axis1 == AXIS_INVALID) return b2;
    if(axis2 == AXIS_INVALID) return b1;

    switch(mixType) {
    case MIX_ADD:
        return b1+b2;
        break;
    case MIX_MULT:
        return b1*b2;
        break;
    case MIX_NONE:
    default:
        return b1;
        break;
    }
}

