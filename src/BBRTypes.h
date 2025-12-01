#if !defined(BBRTYPES_H)
#define BBRTYPES_H

#include <sys/types.h>
#include <string>
#include <stdarg.h>
#include <Arduino.h>

namespace bb {
namespace rmt {

// Sigh. That we need to do this every. single. time.
static int printfFinal(const char* str) {
    Serial.print(str);
    return strlen(str);
}

static int vprintf(const char* format, va_list args) {
    int len = vsnprintf(NULL, 0, format, args) + 1;
    char *buf = new char[len];
    vsnprintf(buf, len, format, args);
    printfFinal(buf);
    free(buf);
    return len;
}

static int printf(const char* format, ...) {
    int retval;
    va_list args;
    va_start(args, format);
    retval = vprintf(format, args);
    va_end(args);
    return retval;
}

  
enum ProtocolType {
    MONACO_XBEE       = 'X',
    MONACO_ESPNOW     = 'E',
    MONACO_BLE        = 'B',
    MONACO_UDP        = 'U',
    MONACO_SAT        = 's',
    SPHERO_BLE        = 'S',
    DROIDDEPOT_BLE    = 'D',
    SPEKTRUM_DSSS     = 'd',
    INVALID_PROTOCOL  = '-'
};

static const uint8_t NAME_MAXLEN = 10;

struct __attribute__ ((packed)) MaxlenString {
    char buf[NAME_MAXLEN];
    void zero() {
        for(unsigned int i=0; i<NAME_MAXLEN; i++) buf[i]=0;
    }
    MaxlenString& operator=(const std::string& str) {
        unsigned int l = str.size();
        for(unsigned int i=0; i<NAME_MAXLEN; i++) {
            if(i<l) buf[i]=str[i];
            else buf[i]=0;
        }
        return *this;
    }
    MaxlenString& operator=(const String& str) {
        unsigned int l = str.length();
        for(unsigned int i=0; i<NAME_MAXLEN; i++) {
            if(i<l) buf[i]=str[i];
            else buf[i]=0;
        }
        return *this;
    }
    MaxlenString& operator=(const char *str) {
        unsigned int l = strlen(str);
        for(unsigned int i=0; i<NAME_MAXLEN; i++) {
            if(i<l) buf[i]=str[i];
            else buf[i]=0;
        }
        return *this;
    }
    operator std::string() const {
        char retbuf[NAME_MAXLEN+1];
        for(unsigned int i=0; i<NAME_MAXLEN; i++) {
            retbuf[i]=buf[i];
        }
        retbuf[NAME_MAXLEN] = 0;

        return std::string(retbuf);
    }
    operator String() const {
        char retbuf[NAME_MAXLEN+1];
        for(unsigned int i=0; i<NAME_MAXLEN; i++) {
            retbuf[i]=buf[i];
        }
        retbuf[NAME_MAXLEN] = 0;

        return String(retbuf);
    }

    bool operator==(const std::string& other) const {
        unsigned int l = other.size();
        for(unsigned int i=0; i<NAME_MAXLEN; i++) {
            if(i<l && buf[i]!=other[i]) return false;
            else if(i>=l && buf[i]!=0) return false;
        }
        return true;
    }
    bool operator==(const String& other) const {
        unsigned int l = other.length();
        for(unsigned int i=0; i<NAME_MAXLEN; i++) {
            if(i<l && buf[i]!=other[i]) return false;
            else if(i>=l && buf[i]!=0) return false; 
        }
        return true;
    }
};

typedef uint8_t InputID;
typedef uint8_t AxisID;
static const InputID INPUT_INVALID = 255;
static const AxisID AXIS_INVALID = 127;


enum Unit {
    UNIT_DEGREES          = 0, // [0..360)
    UNIT_DEGREES_CENTERED = 1, // (-180..180)
    UNIT_UNITY            = 2, // [0..1]
    UNIT_UNITY_CENTERED   = 3, // [-1..1]
    UNIT_RAW              = 4  // dependent on wire interface
};

struct __attribute__ ((packed)) NodeAddr {
    uint8_t byte[8];

    uint32_t addrHi() const { 
        return byte[4] | (byte[5]<<8) | (byte[6]<<16) | (byte[7])<<24; 
    }

    uint32_t addrLo() const { 
        return byte[0] | (byte[1]<<8) | (byte[2]<<16) | (byte[3])<<24; 
    }

    bool isZero() const { 
        for(int i=0; i<8; i++) if(byte[i] != 0) return false;
        return true;
    }
    bool operator==(const NodeAddr& other) const { 
        for(int i=0; i<8; i++) if(byte[i] != other.byte[i]) return false;
        return true;
    }
    bool operator!=(const NodeAddr& other) const { 
        for(int i=0; i<8; i++) if(byte[i] != other.byte[i]) return true;
        return false;
    }

    void fromXBeeAddress(uint32_t addrHi, uint32_t addrLo) {
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

    void fromMACAddress(const uint8_t addr[6]) {
        for(int i=0; i<6; i++) byte[i] = addr[i];
        byte[6] = byte[7] = 0;
    }

    void fromString(const std::string& str) {
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

    std::string toString() const {
        char buf[20] = "";
        if(byte[6] == 0 && byte[7] == 0) {
            sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", byte[0], byte[1], byte[2], byte[3], byte[4], byte[5]);
        } else {
            sprintf(buf, "%lx:%lx", addrHi(), addrLo());
        }
        return buf;
    }
};

static bool operator<(const NodeAddr& a1, const NodeAddr& a2) {
    for(uint8_t i = 0; i<8; i++) {
        if(a1.byte[i] < a2.byte[i]) return true;
    }
    return false;
}

// NodeDescription - 20 bytes
struct __attribute__ ((packed)) NodeDescription {
    NodeAddr addr;               // byte 0..7
    MaxlenString name;           // byte 8..17
    bool isTransmitter     : 1;  // byte 18 bit 0
    bool isReceiver        : 1;  // byte 18 bit 1
    bool isConfigurator    : 1;  // byte 18 bit 2
    uint16_t protoSpecific : 13; // byte 18 bit 3..7, byte 19
};

// Some standard input names. Of course you can define your own but these help to programmatically decide what to map to what.
// Note that these are truncated to 10 chars in MCP packets, so don't make them too long.
static const std::string INPUT_SPEED        = "Speed";     // body speed over ground -- v_x, forward positive
static const std::string INPUT_TURN_RATE    = "TurnRate";  // body turn rate         -- v_alpha, left positive
static const std::string INPUT_DOME_RATE    = "DomeRate";  // dome turn rate         -- left positive
static const std::string INPUT_DOME_ANGLE   = "DomeAngle"; // dome absolute angle    -- left positive
static const std::string INPUT_TURN_ANGLE   = "TurnAngle"; // body absolute angle    -- alpha, left positive
static const std::string INPUT_HEAD_ROLL    = "HeadRoll";
static const std::string INPUT_HEAD_PITCH   = "HeadPitch";
static const std::string INPUT_HEAD_HEADING = "HeadHeadng"; // calling this "heading" because "yaw" is abbrev'd to "y" which is confusing
static const std::string INPUT_V_Y          = "v_y";        // for holonomous droids (e.g. B2EMO) - l/r motion, left positive
static const std::string INPUT_EMOTE_0      = "Emote0";     // sound or other emotion -- specific one from the 0 group if there are several
static const std::string INPUT_EMOTE_0_RND  = "Emote0Rnd";  // random sound from the 0 group
static const std::string INPUT_EMOTE_0_INC  = "Emote0Inc";  // next sound from the 0 group
static const std::string INPUT_EMOTE_1      = "Emote1";     // sound or other emotion
static const std::string INPUT_EMOTE_1_RND  = "Emote1Rnd";  // random sound from the 1 group
static const std::string INPUT_EMOTE_1_INC  = "Emote1Inc";  // next sound from the 1 group
static const std::string INPUT_EMOTE_2      = "Emote2";     // sound or other emotion
static const std::string INPUT_EMOTE_2_RND  = "Emote2Rnd";  // random sound from the 2 group
static const std::string INPUT_EMOTE_2_INC  = "Emote2Inc";  // next sound from the 2 group
static const std::string INPUT_EMOTE_3      = "Emote3";     // sound or other emotion
static const std::string INPUT_EMOTE_3_RND  = "Emote3Rnd";  // random sound from the 3 group
static const std::string INPUT_EMOTE_3_INC  = "Emote3Inc";  // next sound from the 3 group
static const std::string INPUT_EMOTE_4      = "Emote4";     // sound or other emotion
static const std::string INPUT_EMOTE_4_RND  = "Emote4Rnd";  // random sound from the 4 group
static const std::string INPUT_EMOTE_4_INC  = "Emote4Inc";  // next sound from the 4 group

struct Axis {
    std::string name;
    uint8_t bitDepth;
    uint32_t value;
};

enum MixType {
    MIX_NONE = 0, // don't mix -- input = a1
    MIX_ADD  = 1, // input = map(interp(a1) + interp(a2))
    MIX_MULT = 2, // input = map(p1*a1*a2)
};

struct __attribute__ ((packed)) Interpolator {
    int8_t i0, i25, i50, i75, i100;
};
static constexpr Interpolator INTERP_ZERO = {0, 0, 0, 0, 0};
static constexpr Interpolator INTERP_LIN_POSITIVE = {0, 25, 50, 75, 100};
static constexpr Interpolator INTERP_LIN_POSITIVE_INV = {100, 75, 50, 25, 0};
static constexpr Interpolator INTERP_LIN_CENTERED = {-100, -50, 0, 50, 100};
static constexpr Interpolator INTERP_LIN_CENTERED_INV = {100, 50, 0, -50, -100};

// FIXME bit of a problem - AxisMix is 12 bytes (input, 2x 5-byte interpolator, 2x 7-bit axis, 2 bits mix type). 
// To transport on the wire, we need to add the input byte, which adds a 13th byte.
// We really don't want to enlarge the config packet format, it's at 12 bytes max now, so we have to save 1 byte?
// Options --
// 1. don't care, just make it bigger.
// 2. steal bits from Interpolator structure by going to half resolution (2 instead of 1) - would save 10 bits, but at the expense of accuracy?
// 3. reduce number of possible inputs and axes - could maybe save 2 bits here? not enough.
struct __attribute__ ((packed)) AxisMix {
public:
    Interpolator interp1 = INTERP_ZERO;  // byte 0..4
    Interpolator interp2 = INTERP_ZERO;  // byte 5..9
    AxisID axis1    : 7;                 // byte 10 bit 0..6
    AxisID axis2    : 7;                 // byte 10 bit 7 to byte 11 bit 0..5
    MixType mixType : 2;                 // byte 11 bit 6..7

    AxisMix() { 
        interp1 = INTERP_ZERO;
        interp2 = INTERP_ZERO;
        axis1 = AXIS_INVALID; 
        axis2 = AXIS_INVALID;
        mixType = MIX_NONE;
    }

    AxisMix(AxisID ax1, Interpolator ip1 = INTERP_LIN_CENTERED, 
            AxisID ax2=AXIS_INVALID, Interpolator ip2 = INTERP_ZERO, 
            MixType mix=MIX_NONE) { 
        axis1 = ax1; interp1 = ip1;
        axis2 = ax2; interp2 = ip2;
        mixType = mix;
    }

    bool isValid() { return axis1 != AXIS_INVALID && axis2 != AXIS_INVALID && mixType != MIX_NONE; }

    float compute(float a1, float min1, float max1, float a2, float min2, float max2) const {
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
};

struct __attribute__ ((packed)) NodeInputMixMapping {
    NodeAddr addr;
    uint8_t input;
    AxisMix mix;
};

//! normed goes from 0 to 1, and gets transformed into [0..360], [-180..180], [-1..1] depending on Unit value.
static float normedToUnit(float normed, Unit unit) {
    normed = constrain(normed, 0.0f, 1.0f);
    switch(unit) {
    case UNIT_DEGREES:
        return normed * 360.0f;
    case UNIT_DEGREES_CENTERED:
        return (normed-0.5)*360.0f;
    case UNIT_UNITY_CENTERED:
        return (normed-0.5)*2.0f;
    default:
        break;
    }
    return normed;
}

//! val goes from [0..360], [-180..180], [-1..1] depending on Unit value. Returns value in range [0..1].
static float unitToNormed(float val, Unit unit) {
    switch(unit) {
    case UNIT_DEGREES:
        val = constrain(val, 0.0f, 360.0f);
        return val / 360.0f;
    case UNIT_DEGREES_CENTERED:
        val = constrain(val, -180.0f, 180.0f);
        return (val / 360.0f) + 0.5;
    case UNIT_UNITY_CENTERED:
        val = constrain(val, -1.0f, 1.0f);
        return (val / 2.0f) + 0.5;
    case UNIT_UNITY:
        return constrain(val, 0.0f, 1.0f);
        break;
    default:
        break;
    }
    return val;
}

struct Telemetry {
    enum SubsysStatus {
		STATUS_OK		= 0,
		STATUS_DEGRADED	= 1,
		STATUS_ERROR	= 2,
		STATUS_NA       = 3 // not applicable - doesn't exist
    };
	enum DriveMode {
		DRIVE_OFF        = 0,
		DRIVE_VEL        = 1,
		DRIVE_AUTO_POS   = 2,
		DRIVE_POS        = 3,
		DRIVE_AUTONOMOUS = 4,
        DRIVE_CUSTOM1    = 5,
        DRIVE_CUSTOM2    = 6,
        DRIVE_CUSTOM3    = 7
	};

    SubsysStatus batteryStatus, driveStatus, servoStatus, overallStatus;
    DriveMode driveMode;
    float speed;                         // Unit: m/s
    float imuPitch, imuRoll, imuHeading; // Unit: degrees -- leave 0 if not applicable
    float batteryCurrent;                // Unit: amps
    float batteryVoltage;                // Unit: volts
};

/*
    Storage -- some thoughts on memory. 
    If possible we want to be able to store the maximum capability of the remotes. We need
    to store axis input mappings, paired nodes, the type of protocol we are using, and
    maybe some protocol specific things (although we have gotten by without them so far)
    like Wifi or XBee channel, PAM, blablabla.

    The number of paired nodes is not limited in the code, so can theoretically grow out of
    bounds, but it's hard to imagine a system where one node is paired to more than a handful
    of others. So storing 8 seems like a good practical maximum.

    The protocol type is just one byte, it's negligible.

    The protocol specific block is set to 100 bytes for now, anything we can immediately
    think of should fit well into that block (e.g. max length of Wifi SSID is 32 characters,
    max length of WPA-2 PSK is 63 characters, leaving 5 bytes for channel and other stuff).

    The number of axis input mappings is limited by the number of inputs (we chose an uint8 
    as input id for protocol bandwidth reasons, giving 254 max inputs) because there can only
    be one mapping per input. If we want to be able to store all 254, this adds up to a CHUNK 
    of memory. How many we actually need is up to the application, but a typical Monaco remote
    receiver will get up to 38 axes. With one-to-one mixes only, that's the number of input
    mappings you need. 
     
    Let's look at what is available on typical microcontrollers.
    - The SAMD21, used on the MKR Wifi 1010, has up to 16kb of non-volatile flash. Check. 
      We only allow two stored protocols here, but the full set of mappings. ATM these are
      used in-droid, which means we typically only store one protocol anyway, if at all.
    - The ESP32 can go much higher, typically megabytes in size. Check.
      These are also used in the remotes, where we want to store several different configurations,
      so we allow for 16 of those, netting roughly 57kb of Flash.
    - The ATMEGA Arduinos are much worse, at 4096 bytes for the Mega 2560, 1024 bytes for 
      the 328P Uno, and even down to 512 bytes for the Lilypad. For those, we limit the
      max node count to 4 and the max number of mappings to 32, netting 597 bytes. Lilypad
      is unsupported at the moment. These also only get one stored protocol.
*/
struct __attribute__ ((packed)) StorageBlock {
#if defined(ARDUINO_XIAO_ESP32S3) || defined(ARDUINO_XIAO_ESP32C3) || defined(ARDUINO_ADAFRUIT_QTPY_ESP32S2) || defined(ARDUINO_SAMD_MKRWIFI1010)
    static const uint8_t MAX_NUM_NODES = 8;
    static const uint8_t MAX_NUM_MAPPINGS = 254;
    // Memory use: 5617 bytes
#elif defined(ARDUINO_AVR_LILYPAD) || defined(ARDUINO_AVR_LILYPAD_USB)
    static const uint8_t MAX_NUM_NODES = 2;
    static const uint8_t MAX_NUM_MAPPINGS = 16;
    // Memory use: 359 bytes
#else
    static const uint8_t MAX_NUM_NODES = 4;
    static const uint8_t MAX_NUM_MAPPINGS = 48;
    // Memory use: 815 bytes
#endif

    MaxlenString storageName;
    MaxlenString nodeName;
    ProtocolType type;
    uint8_t numPairedNodes;
    NodeDescription pairedNodes[MAX_NUM_NODES]; // 20 bytes each = 160 bytes
    uint8_t numMappings;
    NodeInputMixMapping mapping[MAX_NUM_MAPPINGS];  // 21 bytes each = 5334 bytes
    uint8_t protocolSpecific[100];              // 100 bytes
}; 

struct __attribute__ ((packed)) ProtocolStorage {
    uint8_t num;
    MaxlenString last;
#if defined(ARDUINO_XIAO_ESP32S3) || defined(ARDUINO_XIAO_ESP32C3) || defined(ARDUINO_ADAFRUIT_QTPY_ESP32S2) 
    static const uint8_t MAX_NUM_PROTOCOLS = 16; // 44936 bytes
#elif defined(ARDUINO_SAMD_MKRWIFI1010)
    static const uint8_t MAX_NUM_PROTOCOLS = 2;
#else
    static const uint8_t MAX_NUM_PROTOCOLS = 1;
#endif
    StorageBlock blocks[MAX_NUM_PROTOCOLS];
};

}; // rmt
}; // bb


#endif // BBRTYPES_H