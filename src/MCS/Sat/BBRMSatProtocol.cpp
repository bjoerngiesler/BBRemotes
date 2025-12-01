#include "BBRMSatProtocol.h"

using namespace bb;
using namespace bb::rmt;

MSatProtocol::MSatProtocol(): serialRecStr_(""), ser_(nullptr) {
}


bool MSatProtocol::init(const std::string& nodeName) {
    return init(&Serial1);
}

bool MSatProtocol::init(HardwareSerial* ser) {
    ser_ = ser;
    if(ser_ == nullptr) return false;
    ser->begin(230400);
    return true;
}

bool MSatProtocol::step() {
    if(ser_ == nullptr) return false;

    if(ser_->available() == false) {
        return false;
    }

	while(ser_->available()) {
		char b = ser_->read();
		serialRecStr_ = serialRecStr_ + b;
		if(b == ']') {
			MPacket packet;
			if(deserializePacket(packet, serialRecStr_)) {
                // bb::rmt::printf("Got packet type %d, primary %d\n", packet.type, packet.type == MPacket::PACKET_TYPE_CONTROL ? packet.payload.control.primary : 0);
                // if(packet.type == MPacket::PACKET_TYPE_CONTROL) {
                //     bb::rmt::printf("Axis 0: %f\n", packet.payload.control.getAxis(0));
                //}
				NodeAddr addr;
				incomingPacket(addr, packet);
			}
			serialRecStr_ = "";
		}
	}	
	return MProtocol::step();
}

void MSatProtocol::printInfo() {

}

bool MSatProtocol::waitForPacket(std::function<bool(const MPacket&, const NodeAddr& )> fn, 
                                 NodeAddr& addr, MPacket& packet, 
                                 bool handleOthers, float timeout) {
    return false;
}

