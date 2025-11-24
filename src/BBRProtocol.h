#if !defined(BBRPROTOCOL_H)
#define BBRPROTOCOL_H

#include <sys/types.h>
#include <string>
#include <vector>

#include "BBRTransmitter.h"
#include "BBRReceiver.h"
#include "BBRMixManager.h"
#include "BBRTypes.h"

namespace bb {
namespace rmt {

class Transmitter;
class Receiver;
class Configurator;

// Abstract protocol superclass
class Protocol {
public:
    virtual ProtocolType protocolType() { return INVALID_PROTOCOL; }

    Protocol();
    virtual ~Protocol();

    virtual bool init(const std::string& nodeName) = 0;
    virtual void deinit();

    virtual bool serialize(StorageBlock& block);
    virtual bool deserialize(StorageBlock& block);

    virtual uint8_t numTransmitterTypes() = 0;
    virtual Transmitter* createTransmitter(uint8_t transmitterType=0);
    virtual Receiver* createReceiver();

    virtual bool discoverNodes(float timeout = 5) = 0;
    virtual unsigned int numDiscoveredNodes();
    const NodeDescription& discoveredNode(unsigned int index);

    virtual bool pairWith(const NodeDescription& node);
    virtual bool rePairWithConnected(bool receivers, bool transmitters, bool configurators);

    virtual bool isPaired();
    virtual bool isPaired(const NodeAddr& node);
    virtual bool acceptsPairingRequests() = 0;

    virtual bool requiresConnection() { return false; }
    virtual bool isConnected() { if(!requiresConnection()) return true; else return false; }
    virtual bool connect() { return false; }

    virtual bool step();

    virtual bool sendTelemetry(const Telemetry& telem);
    virtual bool sendTelemetry(const NodeAddr& configuratorAddr, const Telemetry& telem);

    const std::vector<NodeDescription>& pairedNodes() { return pairedNodes_; }

    virtual bool receiverSideMixing() { return false; }
    virtual bool retrieveInputs(const NodeDescription& descr) { return false; }
    virtual bool retrieveInputs();
    virtual bool retrieveMixes(const NodeDescription& descr) { return false; }
    virtual bool retrieveMixes();
    virtual bool sendMixes(const NodeDescription& descr) { return false; }
    virtual bool sendMixes();

    const MixManager& mixManager(const NodeAddr& addr);
    const MixManager& mixManager();

    virtual uint8_t numInputs(const NodeAddr& addr);
    virtual const std::string& inputName(const NodeAddr& addr, uint8_t input);
    virtual uint8_t inputWithName(const NodeAddr& addr, const std::string& name);

    // Register a callback this protocol will call as it's being destroyed.
    virtual void addDestroyCB(std::function<void(Protocol*)> fn);

    virtual void printInfo();
    virtual uint8_t seqnum() { return seqnum_; }

    virtual void setCommTimeoutWatchdog(float seconds, std::function<void(Protocol*,float)> commTimeoutWD);
    virtual void setTelemetryReceivedCB(std::function<void(Protocol*, const NodeAddr&, uint8_t seqnum, const Telemetry&)>);
    virtual void telemetryReceived(const NodeAddr& source, uint8_t seqnum, const Telemetry& telem);

    static void setTransmitFrequencyHz(uint8_t transmitFrequencyHz);

protected:
    virtual bool connect(const NodeAddr& addr) { return false; }
    virtual void commHappened();

    std::vector<NodeDescription> discoveredNodes_;
    std::vector<NodeDescription> pairedNodes_;
    Transmitter* transmitter_ = nullptr;
    Receiver* receiver_ = nullptr;
    Configurator* configurator_ = nullptr;
    std::string nodeName_;

    std::map<NodeAddr,std::vector<std::string>> inputs_;
    std::map<NodeAddr,MixManager> mixManagers_;
    std::vector<std::function<void(Protocol*)>> destroyCBs_;

    std::function<void(Protocol*,float)> commTimeoutWD_;
    std::function<void(Protocol*, const NodeAddr&, uint8_t seqnum, const Telemetry&)> telemReceivedCB_;
    float commTimeoutSeconds_;
    unsigned long lastCommHappenedMS_;
    unsigned long usLastTransmit_;

    uint8_t seqnum_;
};

};
};

#endif // BBRPROTOCOL_H