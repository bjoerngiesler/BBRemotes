#if !defined(BBRMSATPROTOCOL_H)
#define BBRMSATPROTOCOL_H

#include "../BBRMProtocol.h"

namespace bb {
namespace rmt {

class MSatProtocol: public MProtocol {
public:
    MSatProtocol();

    bool init(const std::string& nodeName);
    bool init(HardwareSerial* ser);

    virtual ProtocolType protocolType() { return MONACO_SAT; }

    virtual bool acceptsPairingRequests() { return false; }

    virtual bool serialize(StorageBlock& block) { return false; }
    virtual bool deserialize(StorageBlock& block) { return false; }

    virtual Transmitter* createTransmitter() { return nullptr; }

    virtual bool discoverNodes(float timeout = 5) { return false; }

    virtual bool step();
    virtual bool sendPacket(const NodeAddr& addr, MPacket& packet, bool bumpSeqnum=true) { return false; }
    virtual bool sendBroadcastPacket(MPacket& packet, bool bumpSeqnum=true) { return false; }

    virtual bool waitForPacket(std::function<bool(const MPacket&, const NodeAddr& )> fn, 
                               NodeAddr& addr, MPacket& packet, 
                               bool handleOthers, float timeout);


    virtual void printInfo();

protected:
    std::string serialRecStr_;
    HardwareSerial* ser_;
};

};
};

#endif // BBRMSATPROTOCOL_H