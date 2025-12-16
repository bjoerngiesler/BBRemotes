#if !defined(BBRMXBPROTOCOL_H)
#define BBRMXBPROTOCOL_H

#include <Arduino.h>
#include <vector>
#include "../BBRMProtocol.h"

// Channel table
// ==============================
//
// For home use, you want to minimize wifi overlap. For conventions, DSSS overlap (Spektrum TXs, may be more important.
// Wifi Overlap info taken from https://docs.digi.com/resources/documentation/digidocs/90001537/references/r_channels_zigbee.htm.
// DSM2 and FASST info taken from https://www.fly2air.com/tipps/TxRx/main-2.4GHz.htm.
// DSM2 (Spektrum, Orange, frsky) uses two channels that are identified at startup as the ones with the least energy, then hops between them.
// DSMx (Spektrum, Orange) uses two channels, but hops through the frequency band with them.
// FASST (Futaba, Orange, frsky) hops through the entire 2.4GHz band.
// According to https://www.flyinggiants.com/forums/showthread.php?t=56339, FASST goes up to 2477MHz or 2479MHz depending on the transmitter,
// and each channel is 2.025MHz wide. XBee channels have 5MHz spacing, so there is a risk of overlap.
// According to https://www.metageek.com/training/resources/why-channels-1-6-11/, Wifi channels go from 2.412GHz (ch 1) to 2.462GHz (ch 11), and
// each channel is 22MHz wide. That means the minimum frequency of channel 1 is 2.401GHz, maximum frequency of channel 11 is 2.473GHz.
// All of this means that channel 0x1A should be *reasonably* safe from all Wifi and RC protocol interference. It is power limited to +8dbM though.
// (some old docs on XBee say 3dbM, the data sheet doesn't seem to think so). Compare this to +20dBm for ESPnow...
// YMMV - and mine too, needs testing.

// Hex  Freq (GHz)  Wifi Overlap	DSM2 Overlap		FASST Overlap	Notes
// 0x0B	2.405GHz	Overlaps Ch 1										Newer XBee only
// 0x0C	2.410GHz	Overlaps Ch 1	 
// 0x0D	2.415GHz	Overlaps Ch 1	 
// 0x0E	2.420GHz	Overlaps Ch 1	 
// 0x0F	2.425GHz	Overlaps Ch 6	 
// 0x10	2.430GHz	Overlaps Ch 6	 
// 0x11	2.435GHz	Overlaps Ch 6	 
// 0x12	2.440GHz	Overlaps Ch 6	 
// 0x13	2.445GHz	Overlaps Ch 6	 
// 0x14	2.450GHz	Overlaps Ch 11	 
// 0x15	2.455GHz	Overlaps Ch 11	 
// 0x16	2.460GHz	Overlaps Ch 11	 
// 0x17	2.465GHz	Overlaps Ch 11	 
// 0x18	2.470GHz	Overlaps Ch 11										Newer XBee only
// 0x19	2.475GHz	No Conflict		No Conflict			No Conflict?	Newer XBee only
// 0x1A	2.480GHz	No Conflict		No Conflict			No Conflict?	Newer non-PRO XBee only

// Best channels for non-overlap with Wifi: 0x0d, 0x13, 0x19, 0x1a
// Best channels for non-overlap with DSSS: 0x17, 0x18, 0x19, 0x1a

// Plan of action:
// - use default channel 0x1a and see if it's universally supported
// - check with frequency analyzer
// - implement channel switching and channel energy display
// - go lower for clear-channel assessment - to 0?
// - check with Arnd's Futaba next opportunity

#define DEFAULT_RCHAN    0x1a
#define DEFAULT_RPAN     0x3332

#define DEFAULT_BPS     9600
	
namespace bb {
namespace rmt {

//! Monaco-over-XBee Protocol
class MXBProtocol: public MProtocol {
public:
    MXBProtocol();

    virtual ProtocolType protocolType() { return MONACO_XBEE; }

	void setChannel(uint16_t channel);
	void setPAN(uint16_t pan);

    virtual bool init(const std::string& nodeName) { return init(nodeName, DEFAULT_RCHAN, DEFAULT_RPAN, &Serial1); }
    virtual bool init(const std::string& nodeName, uint16_t chan, uint16_t pan, HardwareSerial *uart);

	virtual bool acceptsPairingRequests() { return true; }

    //virtual bool discoverNodes(float timeout = 5);

    virtual bool step();

    virtual bool sendPacket(const NodeAddr& addr, MPacket& packet, bool bumpSeqnum=true);
    virtual bool sendBroadcastPacket(MPacket& packet, bool bumpSeqnum=true);

    virtual bool waitForPacket(std::function<bool(const MPacket&, const NodeAddr&)> fn, 
                               NodeAddr& addr, MPacket& packet, 
                               bool handleOthers, float timeout);

	const NodeAddr& hwAddress() { return hwAddress_; }

	typedef enum {
		DEBUG_SILENT = 0,
		DEBUG_PROTOCOL   = 0x01,
		DEBUG_XBEE_COMM  = 0x02
	} DebugFlags;

	void setDebugFlags(DebugFlags);

protected:
	bool enterATModeIfNecessary();
	bool leaveATMode(); // incurs a mandatory delay of 1000ms!
	bool isInATMode();
	bool changeBPSTo(uint32_t bps, bool stayInAT=false);
	int getCurrentBPS() { return currentBPS_; }
	bool setConnectionInfo(uint8_t chan, uint16_t pan, bool stayInAT=false);
	bool getConnectionInfo(uint8_t& chan, uint16_t& pan, bool stayInAT=false);

	bool available();
	bool receive();

	bool setAPIMode(bool onoff);
	bool sendAPIModeATCommand(uint8_t frameID, const char* cmd, uint32_t& argument, bool request=false);
	bool receiveAPIMode(NodeAddr& srcAddr, uint8_t& rssi, MPacket& packet);

protected:
	DebugFlags debug_;
	int timeout_;
	unsigned long atmode_millis_, atmode_timeout_;
	bool atmode_, stayInAT_;
	HardwareSerial* uart_;
	int currentBPS_;
	bool apiMode_;
	uint8_t chan_, pan_;
	bool initialized_;

	NodeAddr hwAddress_;

	uint8_t packetBuf_[255];
	size_t packetBufPos_;

	class APIFrame {
	public:
		APIFrame();
		APIFrame(const uint8_t *data, uint16_t dataLength);
		APIFrame(uint16_t length);
		APIFrame(const APIFrame& frame);
		~APIFrame();

		APIFrame& operator=(const APIFrame& frame);
		
		virtual uint8_t *data() const { return data_; }
		virtual uint16_t length() const { return length_; }
		virtual uint8_t checksum() const { return checksum_; }

		void calcChecksum();
		bool verifyChecksum(uint8_t checksum);

		static APIFrame atRequest(uint8_t frameID, uint16_t command);
		bool isATRequest();
		bool isATResponse();
		bool is16BitRXPacket();
		bool is64BitRXPacket();
		bool isRXPacket() { return is16BitRXPacket() || is64BitRXPacket(); }

		enum Type {
			TRANSMITLEGACY  = 0x00,
			ATREQUEST 		= 0x08,
			TRANSMITREQUEST = 0x10,
			RECEIVE64BIT    = 0x80,
			RECEIVE16BIT	= 0x81,
			ATRESPONSE		= 0x88
		};



		bool unpackATResponse(uint8_t &frameID, uint16_t &command, uint8_t &status, uint8_t** data, uint16_t &length);

		class __attribute__ ((packed)) EndianInt16 {
			uint16_t value;
		public:
			inline operator uint16_t() const {
				return ((value & 0xff) << 8) | (value >> 8);
			}
		};
		class __attribute__ ((packed)) EndianInt32 {
			uint32_t value;
		public:
			inline operator uint32_t() const {
				return ((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24);
			}
		};
		class __attribute__ ((packed)) EndianInt64 {
			uint64_t value;
		public:
			inline operator uint64_t() const {
				return ((value & 0x00000000000000ff) << 56) | ((value & 0x000000000000ff00) << 40) | ((value & 0x0000000000ff0000) << 24) | ((value & 0x00000000ff000000) <<  8) | 
					   ((value & 0x000000ff00000000) >>  8) | ((value & 0x0000ff0000000000) >> 24) | ((value & 0x00ff000000000000) >> 40) | ((value & 0xff00000000000000) >> 56);
			}
		};

		static const uint8_t ATResponseNDMinLength = 11;
		struct __attribute__ ((packed)) ATResponseND {
			EndianInt16 my;
			EndianInt32 addrHi, addrLo;
			uint8_t rssi;
			char name[20];
		};

	protected:
		uint8_t *data_;
		uint16_t length_;
		uint8_t checksum_;
	};

	String sendStringAndWaitForResponse(const String& str, int predelay=0, bool cr=true);
	bool sendStringAndWaitForOK(const String& str, int predelay=0, bool cr=true);
	bool readString(String& str, unsigned char terminator='\r');

	bool send(const APIFrame& frame);
	bool receive(APIFrame& frame);
};

}; // rmt
}; // bb

#endif // BB8XBEE_H