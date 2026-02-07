#if !defined(BBRTYPES_H)
#define BBRTYPES_H

#include <sys/types.h>
#include <string>
#include <Arduino.h>

#include "BBRUtils.h"

namespace bb {
namespace rmt {

//! Node address struct. Handles 64bit XBee addresses, 48bit ESPnow or Bluetooth addresses, and 32bit IP addresses.
struct __attribute__ ((packed)) NodeAddr {
    uint8_t byte[8];

    uint32_t addrHi() const;
    uint32_t addrLo() const;

    bool isZero() const;
    bool operator==(const NodeAddr& other) const;
    bool operator!=(const NodeAddr& other) const;

    void fromXBeeAddress(uint32_t addrHi, uint32_t addrLo);
    void fromMACAddress(const uint8_t addr[6]);

    void fromString(const std::string& str);

    std::string toString() const;
};
bool operator<(const NodeAddr& a1, const NodeAddr& a2);

//! Central registry for protocol types.  
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

//! Typedef for input IDs. Valid input IDs go from 0 to 254.
typedef uint8_t InputID;
//! Invalid input ID definition.
static const InputID INPUT_INVALID = 255;

//! Typedef for axis IDs. Valid axis IDs go from 0 to 126.
typedef uint8_t AxisID;
//! Invalid axis ID definition.
static const AxisID AXIS_INVALID = 127;

//! Struct that defines an axis: Name, bitdepth, (current) value.
struct Axis {
    std::string name;
    uint8_t bitDepth;
    uint32_t value;
};

//! Add or multiply the two axis values (post-interpolator), or ignore axis 2 and just juse axis 1
enum MixType {
    MIX_NONE = 0, // don't mix -- input = a1
    MIX_ADD  = 1, // input = map(interp(a1) + interp(a2))
    MIX_MULT = 2, // input = map(p1*a1*a2)
};

//! Just five ints, and a comparator function.
struct __attribute__ ((packed)) Interpolator {
    int8_t i0, i25, i50, i75, i100;
    bool operator==(const Interpolator& i) { return i0 == i.i0 && i25 == i.i25 && i50 == i.i50 && i75 == i.i75 && i100 == i.i100; }
};

//! Preset: Zero interpolator - set all axis values to zero.
static constexpr Interpolator INTERP_ZERO = {0, 0, 0, 0, 0};
//! Preset: Linear interpolator mapping all input values to the range of [0,1].
static constexpr Interpolator INTERP_LIN_POSITIVE = {0, 25, 50, 75, 100};
//! Preset: Inverse linear interpolator mapping all input values to the range of [1,0].
static constexpr Interpolator INTERP_LIN_POSITIVE_INV = {100, 75, 50, 25, 0};
//! Preset: Centered linear interpolator mapping all input values to the range of [-1,1].
static constexpr Interpolator INTERP_LIN_CENTERED = {-100, -50, 0, 50, 100};
//! Preset: Inverse centered linear interpolator mapping all input values to the range of [1,-1].
static constexpr Interpolator INTERP_LIN_CENTERED_INV = {100, 50, 0, -50, -100};

//! Contains all the information (axis IDs, interpolators, mix type) to mix two axes into one input.
struct __attribute__ ((packed)) AxisMix {
public:
    Interpolator interp1 = INTERP_ZERO;  // byte 0..4
    Interpolator interp2 = INTERP_ZERO;  // byte 5..9
    AxisID axis1    : 7;                 // byte 10 bit 0..6
    AxisID axis2    : 7;                 // byte 10 bit 7 to byte 11 bit 0..5
    MixType mixType : 2;                 // byte 11 bit 6..7

    AxisMix();
    AxisMix(AxisID ax1, Interpolator ip1 = INTERP_LIN_CENTERED, 
            AxisID ax2=AXIS_INVALID, Interpolator ip2 = INTERP_ZERO, 
            MixType mix=MIX_NONE);
    bool isValid();
    float compute(float a1, float min1, float max1, float a2, float min2, float max2) const;
};

struct __attribute__ ((packed)) NodeInputMixMapping {
    NodeAddr addr;
    uint8_t input;
    AxisMix mix;
};

//! Maximum length of protocol, input, or axis names on the wire.
static const uint8_t NAME_MAXLEN = 10;

//! Container class to safely handle strings of a certain maximum length, without wasting space for trailing '\0'.
struct __attribute__ ((packed)) MaxlenString {
    char buf[NAME_MAXLEN];
    void zero();
    MaxlenString& operator=(const std::string& str);
    MaxlenString& operator=(const String& str);
    MaxlenString& operator=(const char *str);
    operator std::string() const;
    operator String() const;
    bool operator==(const std::string& other) const;
    bool operator==(const String& other) const;
};

//! NodeDescription - 20 bytes
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
static const std::string INPUT_NAME_SPEED        = "Speed";      // body speed over ground -- v_x, forward positive
static const std::string INPUT_NAME_TURN_RATE    = "TurnRate";   // body turn rate         -- v_alpha, left positive
static const std::string INPUT_NAME_X            = "X";          // movement in X          -- v_x, forward positive
static const std::string INPUT_NAME_Y            = "Y";          // movement in Y          -- v_y, forward positive
static const std::string INPUT_NAME_DOME_RATE    = "DomeRate";   // dome turn rate         -- left positive
static const std::string INPUT_NAME_DOME_ANGLE   = "DomeAngle";  // dome absolute angle    -- left positive
static const std::string INPUT_NAME_TURN_ANGLE   = "TurnAngle";  // body absolute angle    -- alpha, left positive
static const std::string INPUT_NAME_HEAD_ROLL    = "HeadRoll";   // head roll, i.e. rotation around the axis pointing forward
static const std::string INPUT_NAME_HEAD_PITCH   = "HeadPitch";  // head pitch, i.e. rotation around the axis pointing left
static const std::string INPUT_NAME_HEAD_HEADING = "HeadHeadng"; // head heading, i.e. rotation around the axis pointing up. Calling this "heading" because "yaw" is abbrev'd to "y" which is confusing
static const std::string INPUT_NAME_SPEEDY       = "SpeedY";     // for holonomous droids (e.g. B2EMO) - l/r motion, left positive
static const std::string INPUT_NAME_EMOTE_0      = "Emote0";     // sound or other emotion -- specific one from the 0 group if there are several
static const std::string INPUT_NAME_EMOTE_0_RND  = "Emote0Rnd";  // random sound from the 0 group
static const std::string INPUT_NAME_EMOTE_0_INC  = "Emote0Inc";  // next sound from the 0 group
static const std::string INPUT_NAME_EMOTE_1      = "Emote1";     // sound or other emotion
static const std::string INPUT_NAME_EMOTE_1_RND  = "Emote1Rnd";  // random sound from the 1 group
static const std::string INPUT_NAME_EMOTE_1_INC  = "Emote1Inc";  // next sound from the 1 group
static const std::string INPUT_NAME_EMOTE_2      = "Emote2";     // sound or other emotion
static const std::string INPUT_NAME_EMOTE_2_RND  = "Emote2Rnd";  // random sound from the 2 group
static const std::string INPUT_NAME_EMOTE_2_INC  = "Emote2Inc";  // next sound from the 2 group
static const std::string INPUT_NAME_EMOTE_3      = "Emote3";     // sound or other emotion
static const std::string INPUT_NAME_EMOTE_3_RND  = "Emote3Rnd";  // random sound from the 3 group
static const std::string INPUT_NAME_EMOTE_3_INC  = "Emote3Inc";  // next sound from the 3 group
static const std::string INPUT_NAME_EMOTE_4      = "Emote4";     // sound or other emotion
static const std::string INPUT_NAME_EMOTE_4_RND  = "Emote4Rnd";  // random sound from the 4 group
static const std::string INPUT_NAME_EMOTE_4_INC  = "Emote4Inc";  // next sound from the 4 group

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
    float posX, posY;                    // Unit: meters -- leave 0 if not applicable
};

/**
 * @defgroup storage Typedefs for storing protocol information to non-volatile memory
 * @{
 *  # Storage -- some thoughts on memory. 
 * 
 * If possible we want to be able to store the maximum capability of the remotes. We need
 * to store axis input mappings, paired nodes, the type of protocol we are using, and
 * maybe some protocol specific things (although we have gotten by without them so far)
 * like Wifi or XBee channel, PAM, blablabla.
 * 
 * The number of paired nodes is not limited in the code, so can theoretically grow out of
 * bounds, but it's hard to imagine a system where one node is paired to more than a handful
 * of others. So storing 8 seems like a good practical maximum.
 * 
 * The protocol type is just one byte, it's negligible.
 * 
 * The protocol specific block is set to 100 bytes for now, anything we can immediately
 * think of should fit well into that block (e.g. max length of Wifi SSID is 32 characters,
 * max length of WPA-2 PSK is 63 characters, leaving 5 bytes for channel and other stuff).
 * 
 * The number of axis input mappings is limited by the number of inputs (we chose an uint8 
 * as input id for protocol bandwidth reasons, giving 254 max inputs) because there can only
 * be one mapping per input. If we want to be able to store all 254, this adds up to a CHUNK 
 * of memory. How many we actually need is up to the application, but a typical Monaco remote
 * receiver will get up to 38 axes. With one-to-one mixes only, that's the number of input
 * mappings you need. 
 *   
 * Let's look at what is available on typical microcontrollers.
 *  - The SAMD21, used on the MKR Wifi 1010, has up to 16kb of non-volatile flash. Check. 
 *    We only allow two stored protocols here, but the full set of mappings. ATM these are
 *    used in-droid, which means we typically only store one protocol anyway, if at all.
 *  - The ESP32 can go much higher, typically megabytes in size. Check.
 *    These are also used in the remotes, where we want to store several different configurations,
 *    so we allow for 16 of those, netting roughly 57kb of Flash.
 *  - The ATMEGA Arduinos are much worse, at 4096 bytes for the Mega 2560, 1024 bytes for 
 *    the 328P Uno, and even down to 512 bytes for the Lilypad. For those, we limit the
 *    max node count to 4 and the max number of mappings to 32, netting 597 bytes. Lilypad
 *    is unsupported at the moment. These also only get one stored protocol.
 */

//! Storage block to store information for one protocol.
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

//! Storage block to maintain the full number of protocols a node can hold in NVM.
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

/**
 * @}
 */

}; // rmt
}; // bb


#endif // BBRTYPES_H