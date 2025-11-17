#if !defined(BBRMXBPROTOCOL_H)
#define BBRMXBPROTOCOL_H

#include <Arduino.h>
#include <vector>
#include "../BBRMProtocol.h"

#define DEFAULT_CHAN    0x14   // Best channels for non-overlap with Wifi: 0x0d, 0x13, 0x19
#define DEFAULT_PAN     0x3332

#define DEFAULT_BPS     9600
	
namespace bb {
namespace rmt {

class MXBProtocol: public MProtocol {
public:
    MXBProtocol();

    virtual ProtocolType protocolType() { return MONACO_XBEE; }

	void setUart(HardwareSerial* uart);
	void setChannel(uint16_t channel);
	void setPAN(uint16_t pan);

    virtual bool init(const std::string& nodeName) { return init(nodeName, DEFAULT_CHAN, DEFAULT_PAN); }
    virtual bool init(const std::string& nodeName, uint16_t chan, uint16_t pan);

	virtual bool acceptsPairingRequests() { return true; }

    //virtual bool discoverNodes(float timeout = 5);

    virtual bool step();

    virtual bool sendPacket(const NodeAddr& addr, const MPacket& packet, bool bumpSeqnum=true);
    virtual bool sendBroadcastPacket(const MPacket& packet, bool bumpSeqnum=true);

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