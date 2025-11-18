#if !defined(BBRFACTORY_H)
#define BBRFACTORY_H

#include "BBRTypes.h"
#include "BBRProtocol.h"
#include <string>

namespace bb {
namespace rmt {

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
    // Memory use: 3573 bytes
#elif defined(ARDUINO_AVR_LILYPAD) || defined(ARDUINO_AVR_LILYPAD_USB)
    static const uint8_t MAX_NUM_NODES = 2;
    static const uint8_t MAX_NUM_MAPPINGS = 16;
    // Memory use: 359 bytes
#else
    static const uint8_t MAX_NUM_NODES = 4;
    static const uint8_t MAX_NUM_MAPPINGS = 48;
    // Memory use: 815 bytes
#endif

    char name[NAME_MAXLEN]="?";
    ProtocolType type = INVALID_PROTOCOL;
    NodeDescription pairedNodes[MAX_NUM_NODES]; // 20 bytes each = 160 bytes
    InputMixMapping mapping[MAX_NUM_MAPPINGS];  // 13 bytes each = 3302 bytes
    uint8_t protocolSpecific[100];              // 100 bytes
};

struct __attribute__ ((packed)) ProtocolStorage {
    uint8_t num;
#if defined(ARDUINO_XIAO_ESP32S3) || defined(ARDUINO_XIAO_ESP32C3) || defined(ARDUINO_ADAFRUIT_QTPY_ESP32S2) 
    static const uint8_t MAX_NUM_PROTOCOLS = 16;
#elif defined(ARDUINO_SAMD_MKRWIFI1010)
    static const uint8_t MAX_NUM_PROTOCOLS = 2;
#else
    static const uint8_t MAX_NUM_PROTOCOLS = 1;
#endif
    StorageBlock blocks[MAX_NUM_PROTOCOLS];
};


class ProtocolFactory {
public:
    static Protocol* createProtocol(ProtocolType type);
    
    static std::vector<std::string> storedProtocols();
    static Protocol* loadProtocol(const std::string& name);
    static bool storeProtocol(const std::string& name, Protocol* proto);
    static bool eraseProtocol(const std::string& name);

    static void setMemoryReadFunction(std::function<bool(ProtocolStorage&)> readFn);
    static void setMemoryWriteFunction(std::function<bool(const ProtocolStorage&)> writeFn);
};

}; // rmt
}; // bb

#endif // BBRFACTORY_H