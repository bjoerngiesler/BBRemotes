#if !defined(BBRMPACKET_H)
#define BBRMPACKET_H

#include "../BBRTypes.h"

//
// REALTIME PROTOCOL
//
// See https://github.com/bjoerngiesler/Droids/wiki/10-Remote-Control for documentation
//

namespace bb {
namespace rmt {

/*
	Pairing packets
	- Discovery broadcasts are sent by discoverable nodes (i.e. in pairing mode)
	- Pairing requests are sent from configurators to discovered nodes
	- Pairing replies are sent from nodes receiving a pairing request
	- Pairing introductions are sent from paired configurators to nodes to introduce them to other nodes
*/

struct __attribute__ ((packed)) MPairingPacket {
	enum PairingType {
		PAIRING_DISCOVERY_BROADCAST = 0,
		PAIRING_DISCOVERY_REPLY     = 1,
		PAIRING_REQUEST             = 2,
		PAIRING_REPLY               = 3,
		PAIRING_INTRODUCTION        = 4
	};

	enum PairingReplyResult {
		PAIRING_REPLY_OK               = 0,
		PAIRING_REPLY_INVALID_SECRET   = 1,
		PAIRING_REPLY_INVALID_ARGUMENT = 2,
		PAIRING_REPLY_UNKNOWN_NODE     = 3,
		PAIRING_REPLY_ALREADY_PAIRED   = 4,
		PAIRING_REPLY_OTHER_ERROR      = 5
	};

	PairingType      type : 8;          // byte 0

	struct __attribute__ ((packed)) PairingDiscovery {
		uint8_t      builderId;     // byte 1
		uint8_t      stationId;     // byte 2
		uint8_t      stationDetail; // byte 3
		bool		 isTransmitter  : 1; // byte 4
		bool		 isReceiver     : 1; // byte 4
		bool         isConfigurator : 1; // byte 4
		char         name[NAME_MAXLEN];  // byte 5..14
	};

	struct __attribute__ ((packed)) PairingRequest {
		uint32_t pairingSecret;      // byte 1..4
		bool pairAsConfigurator : 1; // byte 5 bit 0
		bool pairAsTransmitter  : 1; // byte 5 bit 1
		bool pairAsReceiver     : 1; // byte 5 bit 2
	};

	struct __attribute__ ((packed)) PairingReply {
		PairingReplyResult res: 8;
	};

	union {
		PairingDiscovery discovery;
		PairingRequest   request;
		PairingReply     reply;
	} pairingPayload;
};

/*
	Control packets
	- 19 axes (5x 10 bit, 5x 8 bit, 8x 1 bit, 1x 5 bit usually reserved for battery status)
*/

struct __attribute__ ((packed)) MControlPacket {
	static const uint8_t BITDEPTH1 = 10;
	static const uint8_t BITDEPTH2 = 8;
	static const uint8_t BITDEPTH3 = 1;
	static const uint8_t BITDEPTH4 = 5;

	uint16_t axis0 : BITDEPTH1; // bit 0..9
	uint16_t axis1 : BITDEPTH1; // bit 10..19
	uint16_t axis2 : BITDEPTH1; // bit 20..29
	uint16_t axis3 : BITDEPTH1; // bit 30..39
	uint16_t axis4 : BITDEPTH1; // bit 40..49
	uint8_t axis5  : BITDEPTH2; // bit 50..57
	uint8_t axis6  : BITDEPTH2; // bit 58..65
	uint8_t axis7  : BITDEPTH2; // bit 66..73
	uint8_t axis8  : BITDEPTH2; // bit 74..81
	uint8_t axis9  : BITDEPTH2; // bit 82..89
	uint8_t axis10 : BITDEPTH3; // bit 90
	uint8_t axis11 : BITDEPTH3; // bit 91
	uint8_t axis12 : BITDEPTH3; // bit 92
	uint8_t axis13 : BITDEPTH3; // bit 93
	uint8_t axis14 : BITDEPTH3; // bit 94
	uint8_t axis15 : BITDEPTH3; // bit 95
	uint8_t axis16 : BITDEPTH3; // bit 96
	uint8_t axis17 : BITDEPTH3; // bit 97
	uint8_t axis18 : BITDEPTH4; // bit 98..102
	bool primary    : 1; // bit 103

	void setAxis(uint8_t num, float value, Unit unit=UNIT_UNITY_CENTERED) {
		uint16_t multiplier = 0;
		if(num < 5) multiplier = (1<<BITDEPTH1)-1;
		else if(num<10) multiplier = (1<<BITDEPTH2)-1;
		else if(num<18) multiplier = (1<<BITDEPTH3)-1;
		else if(num == 18) multiplier = (1<<BITDEPTH4)-1;

		switch(unit) {
		case UNIT_DEGREES:
			value = constrain(value, 0, 360.0);
			value = (value / 360.0) * multiplier;
			break;
		case UNIT_DEGREES_CENTERED:
			value = constrain(value, -180.0, 180.0);
			value = ((value + 180.0)/360.0) * multiplier;
			break;
		case UNIT_UNITY:
			value = constrain(value, 0.0, 1.0);
			value *= multiplier;
			break;
		case UNIT_UNITY_CENTERED:
			value = constrain(value, -1.0, 1.0);
			value = ((value + 1.0)/2.0) * multiplier;
			break;
		case UNIT_RAW:
		default:
			value = constrain(value, 0, multiplier);
			break;
		}

		switch(num) {
		case 0: axis0 = value; break;
		case 1: axis1 = value; break;
		case 2: axis2 = value; break;
		case 3: axis3 = value; break;
		case 4: axis4 = value; break;
		case 5: axis5 = value; break;
		case 6: axis6 = value; break;
		case 7: axis7 = value; break;
		case 8: axis8 = value; break;
		case 9: axis9 = value; break;
		case 10: axis10 = value; break;
		case 11: axis11 = value; break;
		case 12: axis12 = value; break;
		case 13: axis13 = value; break;
		case 14: axis14 = value; break;
		case 15: axis15 = value; break;
		case 16: axis16 = value; break;
		case 17: axis17 = value; break;
		case 18:
		default:
			axis18 = value; break;
		}
	}

	float getAxis(uint8_t num, Unit unit = UNIT_UNITY_CENTERED) const {
		float multiplier = 0;
		if(num < 5) multiplier = (1<<BITDEPTH1)-1;
		else if(num<10) multiplier = (1<<BITDEPTH2)-1;
		else if(num<18) multiplier = (1<<BITDEPTH3)-1;
		else if(num == 18) multiplier = (1<<BITDEPTH4)-1;

		float value;
		switch(num) {
		case 0: value = axis0; break;
		case 1: value = axis1; break;
		case 2: value = axis2; break;
		case 3: value = axis3; break;
		case 4: value = axis4; break;
		case 5: value = axis5; break;
		case 6: value = axis6; break;
		case 7: value = axis7; break;
		case 8: value = axis8; break;
		case 9: value = axis9; break;
		case 10: value = axis10; break;
		case 11: value = axis11; break;
		case 12: value = axis12; break;
		case 13: value = axis13; break;
		case 14: value = axis14; break;
		case 15: value = axis15; break;
		case 16: value = axis16; break;
		case 17: value = axis17; break;
		case 18: 
		default:
			value = axis18; break;
		}

		switch(unit) {
		case UNIT_DEGREES:
			value = (value / multiplier) * 360.0;
			break;
		case UNIT_DEGREES_CENTERED:
			value = ((value / multiplier) * 360.0) - 180.0;
			break;
		case UNIT_UNITY:
			value = value / multiplier;
			break;
		case UNIT_UNITY_CENTERED:
			value = ((value / multiplier) * 2.0) - 1.0;
			break;
		case UNIT_RAW:
		default:
			break;
		}

		return value;
	}

	void print() const { for(int i=0; i<19; i++) printf("%d:%.1f ", i, getAxis(i, UNIT_RAW)); printf("\n"); }
};     // 13 bytes long

struct __attribute__ ((packed)) MStatePacket {
	enum StatusType {
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
		DRIVE_AUTONOMOUS = 4
	};

	StatusType battStatus 	: 2; // bit 0..1
	StatusType driveStatus 	: 2; // bit 2..3
	StatusType servoStatus  : 2; // bit 4..5
	StatusType droidStatus  : 2; // bit 6..7

	// byte 2
	DriveMode driveMode     : 3; // bit 8..10
	uint8_t reserved1       : 5; // bit 11..15

	// byte 3
	int16_t speed           : 12; // bit 16..27, unit mm/s, range -4095..4095 or -14.7kph..14.7kph.
	
	uint16_t pitch          : 10; // bit 28..37, in 360/1024 deg steps
	uint16_t roll           : 10; // bit 38..47, in 360/1024 deg steps
	uint16_t heading        : 10; // bit 48..57, in 360/1024 deg steps
	uint8_t battCurrent     : 6;  // bit 58..63 - in 100mA steps starting at 0, so this goes up to 6.3A 
	uint8_t battVoltage     : 8;  // bit 64..71 - in 0.1V steps starting at a base voltage of 3V, so this goes up to 28.5V
};

struct __attribute__ ((packed)) MConfigPacket {
	static const uint64_t MAGIC = 0xbadeaffebabeface;

	enum ConfigType {
		CONFIG_GET_NUM_INPUTS           = 4, // CountPacket receiver <-- sender
		CONFIG_GET_INPUT                = 5, // NamePacket receiver <-- sender
		CONFIG_GET_MIX                  = 6, // MixPacket receiver <-- sender 
		CONFIG_SET_MIX                  = 7, // MixPacket sender --> receiver
		CONFIG_FACTORY_RESET            = 63  // L->R - parameter: MAGIC
	};

	enum ConfigReplyType {
		CONFIG_TRANSMIT_NOREPLY       = 0, // No reply requested
		CONFIG_TRANSMIT_REPLY         = 1, // Reply requested
		CONFIG_REPLY_OK               = 2, // Reply OK
		CONFIG_REPLY_ERROR            = 3  // Something went wrong
	};

	struct __attribute__ ((packed)) CountPacket {
		uint8_t count;
	};

	struct __attribute__ ((packed)) NamePacket {
		uint8_t index;
		char name[NAME_MAXLEN];
	};

	struct __attribute__ ((packed)) MixPacket {
		uint8_t input;
		int8_t i1_0, i1_25, i1_50, i1_75, i1_100;
		int8_t i2_0, i2_25, i2_50, i2_75, i2_100;
		AxisID a1 : 7;
		AxisID a2 : 7;
		MixType m : 2;
	};

	ConfigType      type  : 6;
	ConfigReplyType reply : 2;
	union {
		CountPacket count;
		NamePacket name;
		MixPacket mix;
	} cfgPayload;
};

static void axisMixToMixPacket(InputID input, const AxisMix& mix, MConfigPacket::MixPacket& mp) {
	mp.input = input;
	mp.i1_0 = mix.interp1.i0;
	mp.i1_25 = mix.interp1.i25;
	mp.i1_50 = mix.interp1.i50;
	mp.i1_75 = mix.interp1.i75;
	mp.i1_100 = mix.interp1.i100;
	mp.i2_0 = mix.interp2.i0;
	mp.i2_25 = mix.interp2.i25;
	mp.i2_50 = mix.interp2.i50;
	mp.i2_75 = mix.interp2.i75;
	mp.i2_100 = mix.interp2.i100;
	mp.a1 = mix.axis1;
	mp.a2 = mix.axis2;
	mp.m = mix.mixType;
}

static void mixPacketToAxisMix(const MConfigPacket::MixPacket& mp, InputID& input, AxisMix& mix) {
	input = mp.input;
	mix.interp1 = {mp.i1_0, mp.i1_25, mp.i1_50, mp.i1_75, mp.i1_100};
	mix.interp2 = {mp.i2_0, mp.i2_25, mp.i2_50, mp.i2_75, mp.i2_100};
	mix.axis1 = mp.a1;
	mix.axis2 = mp.a2;
	mix.mixType = mp.m;
}

struct __attribute__ ((packed)) MPacket {

	enum PacketType {
		PACKET_TYPE_CONTROL  = 0,
		PACKET_TYPE_STATE    = 1,
		PACKET_TYPE_CONFIG   = 2,
		PACKET_TYPE_PAIRING  = 3
	};

	enum PacketSource {
		PACKET_SOURCE_LEFT_REMOTE    = 0,
		PACKET_SOURCE_RIGHT_REMOTE   = 1,
		PACKET_SOURCE_DROID          = 2,
		PACKET_SOURCE_TEST_ONLY      = 3
	};

	PacketType type             : 2;
	PacketSource source         : 2;
	mutable uint8_t seqnum      : 3; // automatically set by Runloop
	uint8_t reserved            : 1; 

	union {
		MControlPacket control;
		MStatePacket state;
		MConfigPacket config;
		MPairingPacket pairing;
	} payload;

	mutable uint8_t crc : 8;

	MPacket(PacketType t, PacketSource s, unsigned long seq) {
		type = t;
		source = s;
		seqnum = seq%8;
		reserved = 0;
	}
	MPacket() { }
	uint8_t calculateCRC() const;
};

static const uint8_t MAX_SEQUENCE_NUMBER = 8;

struct MPacketFrame {
	MPacket packet;
	uint8_t crc;
};

}; // namespace rmt
}; // namespace bb

#endif // BBRMPACKET_H