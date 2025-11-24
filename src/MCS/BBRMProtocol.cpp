#include "BBRMProtocol.h"
#include "BBRMReceiver.h"
#include "BBRMTransmitter.h"

#include <limits.h> // for ULONG_MAX

using namespace bb;
using namespace bb::rmt;

#if !defined(WRAPPEDDIFF)
#define WRAPPEDDIFF(a, b, max) ((a>=b) ? a-b : (max-b)+a)
#endif // WRAPPEDDIFF

#if !defined(MAX)
#define MAX(x, y) ((x<y)?y:x)
#endif // MAX

static const uint8_t crc7Table[256] = {
	0x00, 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e,
	0x90, 0x82, 0xb4, 0xa6, 0xd8, 0xca, 0xfc, 0xee,
	0x32, 0x20, 0x16, 0x04, 0x7a, 0x68, 0x5e, 0x4c,
	0xa2, 0xb0, 0x86, 0x94, 0xea, 0xf8, 0xce, 0xdc,
	0x64, 0x76, 0x40, 0x52, 0x2c, 0x3e, 0x08, 0x1a,
	0xf4, 0xe6, 0xd0, 0xc2, 0xbc, 0xae, 0x98, 0x8a,
	0x56, 0x44, 0x72, 0x60, 0x1e, 0x0c, 0x3a, 0x28,
	0xc6, 0xd4, 0xe2, 0xf0, 0x8e, 0x9c, 0xaa, 0xb8,
	0xc8, 0xda, 0xec, 0xfe, 0x80, 0x92, 0xa4, 0xb6,
	0x58, 0x4a, 0x7c, 0x6e, 0x10, 0x02, 0x34, 0x26,
	0xfa, 0xe8, 0xde, 0xcc, 0xb2, 0xa0, 0x96, 0x84,
	0x6a, 0x78, 0x4e, 0x5c, 0x22, 0x30, 0x06, 0x14,
	0xac, 0xbe, 0x88, 0x9a, 0xe4, 0xf6, 0xc0, 0xd2,
	0x3c, 0x2e, 0x18, 0x0a, 0x74, 0x66, 0x50, 0x42,
	0x9e, 0x8c, 0xba, 0xa8, 0xd6, 0xc4, 0xf2, 0xe0,
	0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x70,
	0x82, 0x90, 0xa6, 0xb4, 0xca, 0xd8, 0xee, 0xfc,
	0x12, 0x00, 0x36, 0x24, 0x5a, 0x48, 0x7e, 0x6c,
	0xb0, 0xa2, 0x94, 0x86, 0xf8, 0xea, 0xdc, 0xce,
	0x20, 0x32, 0x04, 0x16, 0x68, 0x7a, 0x4c, 0x5e,
	0xe6, 0xf4, 0xc2, 0xd0, 0xae, 0xbc, 0x8a, 0x98,
	0x76, 0x64, 0x52, 0x40, 0x3e, 0x2c, 0x1a, 0x08,
	0xd4, 0xc6, 0xf0, 0xe2, 0x9c, 0x8e, 0xb8, 0xaa,
	0x44, 0x56, 0x60, 0x72, 0x0c, 0x1e, 0x28, 0x3a,
	0x4a, 0x58, 0x6e, 0x7c, 0x02, 0x10, 0x26, 0x34,
	0xda, 0xc8, 0xfe, 0xec, 0x92, 0x80, 0xb6, 0xa4,
	0x78, 0x6a, 0x5c, 0x4e, 0x30, 0x22, 0x14, 0x06,
	0xe8, 0xfa, 0xcc, 0xde, 0xa0, 0xb2, 0x84, 0x96,
	0x2e, 0x3c, 0x0a, 0x18, 0x66, 0x74, 0x42, 0x50,
	0xbe, 0xac, 0x9a, 0x88, 0xf6, 0xe4, 0xd2, 0xc0,
	0x1c, 0x0e, 0x38, 0x2a, 0x54, 0x46, 0x70, 0x62,
	0x8c, 0x9e, 0xa8, 0xba, 0xc4, 0xd6, 0xe0, 0xf2
};

static uint8_t calcCRC7(const uint8_t *buffer, size_t len) {
	uint8_t crc = 0;
	while(len--) {
		crc = crc7Table[crc ^ *buffer++];
	}
	return crc;
}

uint8_t MPacket::calculateCRC() const {
	return calcCRC7((const uint8_t*)this, sizeof(MPacket)-1);
}

MProtocol::MProtocol() {
	pairingSecret_ = 0xbabeface;
    seqnum_ = 0;
}

bool MProtocol::serialize(StorageBlock& block) {
	block.protocolSpecific[0] = source_;
	block.protocolSpecific[1] = primary_;
	return Protocol::serialize(block);
}
    
bool MProtocol::deserialize(StorageBlock& block) {
	source_ = (MPacket::PacketSource)block.protocolSpecific[0];
	primary_ = (block.protocolSpecific[1] != 0);
	return Protocol::deserialize(block);
}


Transmitter* MProtocol::createTransmitter(uint8_t transmitterType) {
    if(transmitter_ == nullptr) {
        transmitter_ = new MTransmitter(this);
    }
	transmitter_->setPrimary(primary_);
    return transmitter_;
}

Receiver* MProtocol::createReceiver() {
	if(receiver_ == nullptr) {
		receiver_ = new MReceiver;
	}
    return receiver_;
}

bool MProtocol::incomingPacket(const NodeAddr& addr, const MPacket& packet) {
	bool res;
	MConfigPacket::ConfigReplyType reply = packet.payload.config.reply;
	MPacket packet2 = packet;

	switch(packet.type) {
	case MPacket::PACKET_TYPE_CONTROL:
		if(receiver_ == nullptr) {
			printf("Got control packet from %s but we are not a receiver.\n", addr.toString().c_str());
			return false;
		}
		if(packet.payload.control.primary) commHappened();

		return ((MReceiver*)receiver_)->incomingControlPacket(addr, packet.source, packet.seqnum, packet.payload.control);
		break;

	case MPacket::PACKET_TYPE_STATE:
		if(receiver_ == nullptr) {
			printf("Got state packet from %s but we are not a receiver.\n", addr.toString().c_str());
			return false;
		}
		//return ((MReceiver*)receiver_)->incomingStatePacket(addr, packet.source, packet.seqnum, packet.payload.state);
		return incomingStatePacket(addr, packet.source, packet.seqnum, packet.payload.state);
		break;

	case MPacket::PACKET_TYPE_CONFIG:
		printf("Config packet from %s\n", addr.toString().c_str());
		if(reply == MConfigPacket::CONFIG_REPLY_ERROR || reply == MConfigPacket::CONFIG_REPLY_OK) {
			printf("This is a Reply packet! Discarding.\n");
			return false;
		}
		res = incomingConfigPacket(addr, packet.source, packet.seqnum, packet2.payload.config);
		if(res == true) {
			printf("Sending reply with OK flag set\n");
			packet2.payload.config.reply = MConfigPacket::CONFIG_REPLY_OK;
		}
		else {
			printf("Sending reply with ERROR flag set\n");
			packet2.payload.config.reply = MConfigPacket::CONFIG_REPLY_ERROR;
		}

		return sendPacket(addr, packet2);
		break;

	case MPacket::PACKET_TYPE_PAIRING:
		incomingPairingPacket(addr, packet.source, packet.seqnum, packet.payload.pairing);
		break;

	default:
		printf("Error: Unknown packet type %d\n", packet.type);
		return false;
	}

    return true;
}

bool MProtocol::incomingConfigPacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, MConfigPacket& packet) {
	if(!isPairedAsConfigurator(addr)) {
		printf("Not accepting config packets from %s because it's not a configurator\n", addr.toString().c_str());
		return false;
	}

	if(packet.type == packet.CONFIG_GET_NUM_INPUTS) {
		if(receiver_ == nullptr) return false;
		printf("Got request for num inputs, replying with %d\n", receiver_->numInputs());
		packet.cfgPayload.count.count = receiver_->numInputs();
		return true;
	}

	if(packet.type == packet.CONFIG_GET_INPUT) {
		if(receiver_ == nullptr) return false;
		if(receiver_->numInputs() <= packet.cfgPayload.name.index) return false;
		const std::string& name = receiver_->inputName(packet.cfgPayload.name.index);
		packet.cfgPayload.name.name = name;
		printf("Got request for input #%d, replying with \"%s\"\n", packet.cfgPayload.name.index, name.c_str());
		return true;
	}

	if(packet.type == packet.CONFIG_GET_MIX) {
		if(receiver_ == nullptr) return false;
		if(receiver_->numInputs() <= packet.cfgPayload.mix.input) return false;
		axisMixToMixPacket(packet.cfgPayload.mix.input, receiver_->mixForInput(packet.cfgPayload.mix.input), packet.cfgPayload.mix);
		printf("Got request for mix #%d, replying with mix", packet.cfgPayload.mix.input);
		return true;
	}

	if(packet.type == packet.CONFIG_SET_MIX) {
		if(receiver_ == nullptr) return false;
		if(receiver_->numInputs() <= packet.cfgPayload.mix.input) return false;
		AxisMix mix;
		InputID input;
		mixPacketToAxisMix(packet.cfgPayload.mix, input, mix);
		receiver_->setMix(input, mix);
		printf("Got request to set mix for #%d", input);
		return true;
	}

	return false;
}

bool MProtocol::incomingPairingPacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, const MPairingPacket& packet) {
	if(packet.type == MPairingPacket::PAIRING_DISCOVERY_BROADCAST) {
		if(!acceptsPairingRequests()) {
			return false;
		}

		// FIXME filter for builder ID

		printf("Replying to %s (\"%s\") with broadcast pairing packet\n", 
			          addr.toString().c_str(), std::string(packet.pairingPayload.discovery.name).c_str());
		MPacket reply;
		reply.source = source_;
		reply.type = MPacket::PACKET_TYPE_PAIRING;
		MPairingPacket& p = reply.payload.pairing;
		p.type = p.PAIRING_DISCOVERY_REPLY;
		p.pairingPayload.discovery.builderId = builderId_;
		p.pairingPayload.discovery.stationId = stationId_;
		p.pairingPayload.discovery.stationDetail = stationDetail_;
		p.pairingPayload.discovery.isTransmitter = (transmitter_ != nullptr);
		p.pairingPayload.discovery.isReceiver = (receiver_ != nullptr);
		p.pairingPayload.discovery.isConfigurator = (configurator_ != nullptr);
		p.pairingPayload.discovery.name = nodeName_;

		reply.seqnum = seqnum_;
		seqnum_ = (seqnum_ + 1) % MAX_SEQUENCE_NUMBER;
		sendBroadcastPacket(reply);
		return true;
	}

	if(packet.type == MPairingPacket::PAIRING_DISCOVERY_REPLY) {
		for(auto& n: discoveredNodes_) {
			if(n.addr == addr) {
				return true;
			}
		}

		NodeDescription descr;
		descr.addr = addr;
		descr.isConfigurator = packet.pairingPayload.discovery.isConfigurator;
		descr.isTransmitter = packet.pairingPayload.discovery.isTransmitter;
		descr.isReceiver = packet.pairingPayload.discovery.isReceiver;
		descr.name = packet.pairingPayload.discovery.name;
		descr.protoSpecific = 0x0;

		if(!descr.isConfigurator && !descr.isTransmitter && !descr.isReceiver) {
			bb::rmt::printf("Node \"%s\" at %s is neither configurator nor receiver nor transmitter. Ignoring.\n",
			                std::string(descr.name).c_str(), addr.toString().c_str());
			return false;
		} 

		bb::rmt::printf("Discovered \"%s\" at %s (configurator: %s receiver: %s transmitter: %s).\n",
					std::string(descr.name).c_str(), addr.toString().c_str(),
					descr.isConfigurator ? "yes" : "no",
					descr.isReceiver ? "yes" : "no",
					descr.isTransmitter ? "yes" : "no");
		discoveredNodes_.push_back(descr);
		return true;
	}

	if(packet.type == MPairingPacket::PAIRING_REQUEST) {
		if(!acceptsPairingRequests()) {
			printf("Received pairing request but not in pairing mode. Ignoring.\n");
			return false;
		}

		const MPairingPacket::PairingRequest& r = packet.pairingPayload.request;
		
		MPacket reply;
		reply.source = source_;
		reply.type = MPacket::PACKET_TYPE_PAIRING;
		reply.payload.pairing.type = MPairingPacket::PAIRING_REPLY;
		
		// Secret invalid? ==> error
		if(r.pairingSecret != pairingSecret_) {
			printf("Invalid secret.\n");
			reply.payload.pairing.pairingPayload.reply.res = MPairingPacket::PAIRING_REPLY_INVALID_SECRET;
			sendPacket(addr, reply);
			return true;
		}

		// Pairing request nonsensical? ==> error
		if(!r.pairAsConfigurator && !r.pairAsReceiver && !r.pairAsTransmitter) {
			printf("Received pairing request but as neither configurator nor receiver nor transmitter.\n");
			reply.payload.pairing.pairingPayload.reply.res = MPairingPacket::PAIRING_REPLY_INVALID_ARGUMENT;
			sendPacket(addr, reply);
			return true;
		}

		// Already have this node? ==> error
		for(auto& n: pairedNodes_) {
			if(n.addr == addr) {
				printf("Already paired to %s.\n", addr.toString().c_str());
				reply.payload.pairing.pairingPayload.reply.res = MPairingPacket::PAIRING_REPLY_ALREADY_PAIRED;
				sendPacket(addr, reply);
				return true;
			}
		}

		NodeDescription descr;
		descr.addr = addr;
		descr.name = packet.pairingPayload.request.name;
		descr.isConfigurator = packet.pairingPayload.request.pairAsConfigurator;
		descr.isReceiver = packet.pairingPayload.request.pairAsReceiver;
		descr.isTransmitter = packet.pairingPayload.request.pairAsTransmitter;
		printf("Adding %s as configurator: %s receiver: %s transmitter: %s\n", addr.toString().c_str(), 
					packet.pairingPayload.request.pairAsConfigurator ? "yes" : "no", 
					packet.pairingPayload.request.pairAsReceiver ? "yes" : "no", 
					packet.pairingPayload.request.pairAsTransmitter ? "yes" : "no");
		pairedNodes_.push_back(descr);
		reply.payload.pairing.pairingPayload.reply.res = MPairingPacket::PAIRING_REPLY_OK;
		sendPacket(addr, reply);
		return true;
	}

	printf("Unknown pairing packet type %d\n", packet.type);
	return false;
}

bool MProtocol::incomingStatePacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, const MStatePacket& s) {
	Telemetry telem;

	telem.batteryStatus = s.battStatus;
	telem.driveStatus = s.driveStatus;
	telem.servoStatus = s.servoStatus;
	telem.overallStatus = s.droidStatus;
	telem.driveMode = s.driveMode;

	telem.speed = float(s.speed)/1000.0f;
	telem.imuPitch = float(s.pitch)*360.0f/1024.0f;
	telem.imuRoll = float(s.roll)*360.0f/1024.0f;
	telem.imuHeading = float(s.heading)*360.0f/1024.0f;

	telem.batteryCurrent = float(s.battCurrent)*0.1;
	telem.batteryVoltage = 3+float(s.battVoltage)*0.1;

	telemetryReceived(addr, seqnum, telem);

	return true;
}

bool MProtocol::discoverNodes(float timeout) {
	discoveredNodes_.clear();

	MPacket packet;
	packet.source = source_;
	packet.type = MPacket::PACKET_TYPE_PAIRING;
	MPairingPacket& p = packet.payload.pairing;

	p.type = p.PAIRING_DISCOVERY_BROADCAST;
	p.pairingPayload.discovery.builderId = builderId_;
	p.pairingPayload.discovery.stationId = stationId_;
	p.pairingPayload.discovery.stationDetail = stationDetail_;
	p.pairingPayload.discovery.isTransmitter = (transmitter_ != nullptr);
	p.pairingPayload.discovery.isReceiver = (receiver_ != nullptr);
	p.pairingPayload.discovery.isConfigurator = (configurator_ != nullptr);
	p.pairingPayload.discovery.name = nodeName_;
	sendBroadcastPacket(packet);

	unsigned long msSinceDiscoveryPacket = millis();
	msSinceDiscoveryPacket = millis();

	while(timeout > 0) {
		step();

		if(WRAPPEDDIFF(millis(), msSinceDiscoveryPacket, ULONG_MAX) > 1000) {
			sendBroadcastPacket(packet);
			msSinceDiscoveryPacket = millis();
		}
		delay(10);
		timeout -= .01;
	}

	return true;
}

bool MProtocol::pairWith(const NodeDescription& descr) {
	MPacket packet;
	packet.source = source_;
	packet.type = MPacket::PACKET_TYPE_PAIRING;
	MPairingPacket& p = packet.payload.pairing;

	p.type = p.PAIRING_REQUEST;
	p.pairingPayload.request.pairingSecret = pairingSecret_;
	p.pairingPayload.request.pairAsConfigurator = true;
	p.pairingPayload.request.pairAsReceiver = (receiver_ != nullptr);
	p.pairingPayload.request.pairAsTransmitter = (transmitter_ != nullptr);
	p.pairingPayload.request.name = nodeName_;

	printf("Sending packet of size %d to %s\n", sizeof(packet), descr.addr.toString().c_str());
	sendPacket(descr.addr, packet);

	MPacket pairingReplyPacket;
	NodeAddr pairingReplyAddr;
	NodeAddr addr = descr.addr;
	std::function<bool(const MPacket&, const NodeAddr&)> fn = [addr](const MPacket& p, const NodeAddr& a) {
			return p.type == p.PACKET_TYPE_PAIRING &&
					p.payload.pairing.type == MPairingPacket::PAIRING_REPLY &&
					a == addr; 
	};
	if(waitForPacket(fn, pairingReplyAddr, pairingReplyPacket, true, 0.5) == false) {
		printf("Timed out waiting for pairing reply.\n");
		return false;
	}

	printf("Received pairing reply.\n");
	const MPairingPacket::PairingReply& r = pairingReplyPacket.payload.pairing.pairingPayload.reply;

	if(r.res == MPairingPacket::PAIRING_REPLY_INVALID_SECRET) {
		printf("Pairing reply: Invalid secret.\n");
		return true;
	} else if(r.res == MPairingPacket::PAIRING_REPLY_OTHER_ERROR) {
		printf("Pairing reply: Other error.\n");
		return true;
	}

	for(auto& n: discoveredNodes_) {
		if(n.addr == addr) {
			if(!n.isConfigurator && !n.isReceiver && !n.isTransmitter) {
				printf("Huh. This node is neither configurator nor receiver nor transmitter. Ignoring.\n");
				return false;
			}

			printf("Pairing with %s (configurator: %d receiver: %d transmitter: %d)\n",
			n.addr.toString().c_str(), n.isConfigurator, n.isReceiver, n.isTransmitter);
			return Protocol::pairWith(descr);
		}
	}

	printf("Node sent a pairing reply but is not in our discovery list. Ignoring.\n");
	return false;
}

bool MProtocol::retrieveInputs(const NodeDescription& descr) {
	MPacket packet;
	packet.source = source_;
	packet.type = MPacket::PACKET_TYPE_CONFIG;
	MConfigPacket& c = packet.payload.config;
	c.type = MConfigPacket::CONFIG_GET_NUM_INPUTS;
	c.reply = MConfigPacket::CONFIG_TRANSMIT_REPLY;
	c.cfgPayload.count.count = 0;

	printf("MProtocol: Retrieve Inputs in %s\n", descr.addr.toString().c_str());

	sendPacket(descr.addr, packet);

	MPacket replyPacket;
	NodeAddr replyAddr;
	NodeAddr addr = descr.addr;
	std::function<bool(const MPacket&, const NodeAddr&)> fn = [addr](const MPacket& p, const NodeAddr& a) {
			return a == addr && p.type == p.PACKET_TYPE_CONFIG && p.payload.config.type == MConfigPacket::CONFIG_GET_NUM_INPUTS;
	};
	if(waitForPacket(fn, replyAddr, replyPacket, true, 0.5) == false) {
		printf("Timed out waiting for num inputs reply.\n");
		return false;
	}

	printf("Received num inputs reply -- %d inputs\n", replyPacket.payload.config.cfgPayload.count.count);

	return true;
}

bool MProtocol::retrieveMixes(const NodeDescription& descr) {
	printf("Mix retrieval not implemented\n");
	return false;
}


bool MProtocol::step() {
    return Protocol::step();
}

void MProtocol::bumpSeqnum() {
	seqnum_ = (seqnum_ + 1) % MAX_SEQUENCE_NUMBER;
}

bool MProtocol::sendTelemetry(const Telemetry& telem) {
	return Protocol::sendTelemetry(telem);
}

bool MProtocol::sendTelemetry(const NodeAddr& configuratorAddr, const Telemetry& telem) {
	MPacket packet;
	packet.source = source_;
	packet.type = MPacket::PACKET_TYPE_STATE;
	MStatePacket& s = packet.payload.state;

	s.battStatus = telem.batteryStatus;
	s.driveStatus = telem.driveStatus;
	s.servoStatus = telem.servoStatus;
	s.droidStatus = telem.overallStatus;
	s.driveMode = telem.driveMode;

	s.speed = int16_t(telem.speed*1000);
	s.pitch = uint16_t(telem.imuPitch < 0     ? uint16_t(((telem.imuPitch+360)*1024.0)/360.0) : uint16_t((telem.imuPitch*1024.0)/360.0));
	s.roll = uint16_t(telem.imuRoll < 0       ? uint16_t(((telem.imuRoll+360)*1024.0)/360.0) : uint16_t((telem.imuRoll*1024.0)/360.0));
	s.heading = uint16_t(telem.imuHeading < 0 ? uint16_t(((telem.imuHeading+360)*1024.0)/360.0) : uint16_t((telem.imuHeading*1024.0)/360.0));

	if(telem.batteryCurrent < 0) s.battCurrent = 0;
	else if(telem.batteryCurrent > 6.3) s.battCurrent = 63;
	else s.battCurrent = uint8_t(telem.batteryCurrent / 0.1);

	if(telem.batteryVoltage < 0) s.battVoltage = 0;
	else if(telem.batteryVoltage > 255) s.battVoltage = 255;
	else s.battVoltage = uint8_t((telem.batteryVoltage-3) / 0.1);

	return sendPacket(configuratorAddr, packet);
}

bool MProtocol::isPairedAsConfigurator(const NodeAddr& addr) {
	for(auto& d: pairedNodes_) {
		if(d.addr == addr && d.isConfigurator == true) return true;
	}
	return false;
}

void MProtocol::printInfo() {
	Protocol::printInfo();
	if(primary_) bb::rmt::printf("This protocol is primary.\n");
	else bb::rmt::printf("This protocol is secondary.\n");
}
