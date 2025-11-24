#if !defined(BBRMPROTOCOL_H)
#define BBRMPROTOCOL_H

#include "../BBRProtocol.h"
#include "BBRMPacket.h"

namespace bb {
namespace rmt {

class MProtocol: public Protocol {
public:
    enum TransmitterTypes {
        LEFT_TX = 0,
        RIGHT_TX = 1
    };

    MProtocol();

    virtual ProtocolType protocolType() { return INVALID_PROTOCOL; }

    virtual bool serialize(StorageBlock& block);
    virtual bool deserialize(StorageBlock& block);

    virtual Transmitter* createTransmitter(uint8_t transmitterType=0);
    virtual Receiver* createReceiver();

    virtual uint8_t numTransmitterTypes() { return 2; }
    virtual uint8_t numChannels(uint8_t transmitterType) { return 19; }

    virtual bool discoverNodes(float timeout = 5);
    bool pairWith(const NodeDescription& descr);

    virtual bool receiverSideMixing() { return true; }
    virtual bool retrieveInputs(const NodeDescription& descr);
    virtual bool retrieveMixes(const NodeDescription& descr);

    virtual bool step();
    virtual bool sendPacket(const NodeAddr& addr, MPacket& packet, bool bumpSeqnum=true) = 0;
    virtual bool sendBroadcastPacket(MPacket& packet, bool bumpSeqnum=true) = 0;
    virtual void bumpSeqnum();
    virtual uint8_t seqnum() { return seqnum_; }

    void setPacketSource(MPacket::PacketSource src) { source_ = src; }
	MPacket::PacketSource packetSource() { return source_; }

    bool sendTelemetry(const Telemetry& telem);
    bool sendTelemetry(const NodeAddr& configuratorAddr, const Telemetry& telem);

    void setTransmittersArePrimary(bool p) { primary_ = p; if(transmitter_ != nullptr) transmitter_->setPrimary(p); }
    bool areTransmittersPrimary() { return primary_; }

	void setBuilderId(uint8_t builderId) { builderId_ = builderId; }
	uint8_t builderId() { return builderId_; }
	void setStationId(uint8_t stationId) { stationId_ = stationId; }
	uint8_t stationId() { return stationId_; }
	void setStationDetail(uint8_t stationDetail) { stationDetail_ = stationDetail; }
	uint8_t stationDetail() { return stationDetail_; }

    void setPairingSecret(uint32_t secret) { pairingSecret_ = secret; }

    virtual bool incomingPacket(const NodeAddr& addr, const MPacket& packet);
	virtual bool incomingConfigPacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, MConfigPacket& packet);
	virtual bool incomingPairingPacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, const MPairingPacket& packet);
	virtual bool incomingStatePacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, const MStatePacket& packet);
    virtual bool waitForPacket(std::function<bool(const MPacket&, const NodeAddr& )> fn, 
                               NodeAddr& addr, MPacket& packet, 
                               bool handleOthers, float timeout) = 0;

    virtual void printInfo();

protected:
    bool isPairedAsConfigurator(const NodeAddr& addr);

    uint8_t builderId_, stationId_, stationDetail_;
    uint32_t pairingSecret_;
	MPacket::PacketSource source_;
    bool primary_;
};

}; // rmt
}; // bb

#endif // BBRMPROTOCOL_H