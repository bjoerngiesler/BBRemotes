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

//! Abstract protocol superclass. Cannot be instantiated directly.
class Protocol {
public:
    //! Return the protocol type.
    virtual ProtocolType protocolType() { return INVALID_PROTOCOL; }

    Protocol();
    virtual ~Protocol();

    //! Initialize the protocol. Implement this in your protocol subclass.
    virtual bool init(const std::string& nodeName) = 0;
    //! Deinitialize the protocol. Implement this in your protocol subclass, calling the superclass's `deinit()` from there.
    virtual void deinit();
    //! Return the node name the protocol has been initialized with.
    const std::string& nodeName() const { return nodeName_; } 

    //! This is the main handler function. Call this regularly from your `loop()` function.
    virtual bool step();

    //! Specify the frequency with which `transmit()` methods should be called from `step()`.
    static void setTransmitFrequencyHz(uint8_t transmitFrequencyHz);

    /**
     * \defgroup storage Storing and loading protocols using non-volatile memory
     * @{
     * 
     * To store within non-volatile memory, protocols can __serialize__ themselves into structures of type `StorageBlock`.
     * These capture all relevant info -- paired nodes, receiver mixes, etc. The library does not implement the actual
     * storage process into NVM, because that is highly dependent on the microcontroller and its NVM support library.
     * 
     * You typically do not want to call these functions directly, rather rely on `ProtocolFactory` for that.
     */
    //! Serialize the protocol. Contains everything except the protocol specific block. Implement in your subclass, calling super's `serialize()`.
    virtual bool serialize(StorageBlock& block);
    //! Deserialize the protocol. Contains everything except the protocol specific block. Implement in your subclass, calling super's `deserialize()`.
    virtual bool deserialize(StorageBlock& block);

    //! Return the protocol's storage name.
    const std::string& storageName() const { return storageName_; } 
    //! Set the protocol's storage name.
    void setStorageName(const std::string& name) { storageName_ = name; }
    /**
     * @}
     */

    /**
     * \defgroup tx_rx_handling Transmitter / Receiver handling
     * @{
     * 
     * The protocol creates transmitters and receivers (you cannot create them directly). Each protocol can only have one
     * transmitter and receiver. Remember these are abstractions for concepts in the node you are implementing, so having
     * a transmitter pointer means "I am a transmitter", and having a receiver pointer means "I am a receiver".
     * 
     * A protocol can have multiple transmitter **types** (eg a primary and secondary transmitter). 
     * 
     * **Note:** Not all protocols have transmitters, and not all protocols have receivers. Eg there is no DroidDepot receiver,
     * since that code is proprietary inside the DD droids; and there is no Spektrum DSSS transmitter, since that code is
     * proprietary to Spektrum. So `createTransmitter()` and `createReceiver()` can return `nullptr` if the protocol
     * does not support them.
     */

    //! Return the number of transmitter types a protocol can have
    virtual uint8_t numTransmitterTypes() = 0;
    //! Create a transmitter. Can return `nullptr` if the protocol does not support it.
    virtual Transmitter* createTransmitter(uint8_t transmitterType=0);
    //! Return a transmitter that must have been created before.
    virtual Transmitter* transmitter() { return transmitter_; }

    //! Create a receiver. Can return `nullptr` if the protocol does not support it.
    virtual Receiver* createReceiver();
    //! Return a receiver that must have been created before.
    virtual Receiver* receiver() { return receiver_; }

    /**
     * @}
     */

    /**
     * \defgroup discovery_pairing Discovery and Pairing
     * @{
     */

    virtual bool discoverNodes(float timeout = 5) = 0;
    virtual unsigned int numDiscoveredNodes();
    const NodeDescription& discoveredNode(unsigned int index);

    virtual bool pairWith(const NodeDescription& node);
    virtual bool rePairWithConnected(bool receivers, bool transmitters, bool configurators);

    virtual bool isPaired();
    virtual bool isPaired(const NodeAddr& node);
    virtual bool acceptsPairingRequests() = 0;

    const std::vector<NodeDescription>& pairedNodes() { return pairedNodes_; }

    /**
     * @}
     */

    /**
     * \defgroup connection_handling Connection handling
     * @{
     * 
     * Some protocols require __connections__, i.e. a stateful link between nodes. Not all do, however -- 
     * the Monaco protocol is intentionally stateless. For protocols that do, `requiresConnection()` will
     * return true, and you need to check whether the protocol is connected, and connect if it isn't.
     */
    virtual bool requiresConnection() { return false; }
    virtual bool isConnected() { if(!requiresConnection()) return true; else return false; }
    virtual bool connect() { return false; }
    /**
     * @}
     */

    /**
     * \defgroup mix_handling Axis/Input mix handling
     * @{ 
     * 
     * Protocols can do their Axis / Input mix handling either on the receiver side (preferred) or on the
     * transmitter side (if receiver side mixing is not possible, eg for Droid Depot or Sphero Protocols).
     * 
     * If mixes reside on the receiver side, and in order to list, inspect, and change them on the transmitter,
     * the protocol shoud implement communication paradigms that retrieve the mixes and inputs from the receiver,
     * and sends them back once done.
     * 
     * The methods in this group are used by subclasses for retrieval / sending. The protocol keeps one 
     * instance of `MixManager` per paired node, that can be used to work with the retrieved mixes, and one list of
     * inputs per paired node that can be used to inspect the retrieved list of inputs.
     */

    //! Returns `true` if the protocol does receiver side mixing.
    virtual bool receiverSideMixing() { return false; }

    //! Retrieves the full list of mixes from the given node.
    virtual bool retrieveMixes(const NodeDescription& descr) { return false; }
    //! Retrieves the full list of mixes from the first paired node (for convenience).
    virtual bool retrieveMixes();
    //! Sends the full list of mixes to the given node.
    virtual bool sendMixes(const NodeDescription& descr) { return false; }
    //! Sends the full list of mixes to the first paired node (for convenience).
    virtual bool sendMixes();

    //! Retrieves the full list of inputs from the given node.
    virtual bool retrieveInputs(const NodeDescription& descr) { return false; }
    //! Retrieves the full list of inputs from the first paired node (for convenience).
    virtual bool retrieveInputs();
    //! Returns the number of inputs retrieved for the given node.
    virtual uint8_t numInputs(const NodeAddr& addr);
    //! Returns the name of the given input retrieved for the given node.
    virtual const std::string& inputName(const NodeAddr& addr, InputID input);
    //! Returns the ID of the given input retrieved for the given node.
    virtual InputID inputWithName(const NodeAddr& addr, const std::string& name);

    //! Returns the mix manager for the given node.
    MixManager& mixManager(const NodeAddr& addr);
    //! Returns the mix manager for the first paired node (for convenience).
    MixManager& mixManager();
    /**
     * @}
     */

    /**
     * @defgroup telemetry Telemetry handling
     * @{
     */
    virtual bool sendTelemetry(const Telemetry& telem);
    virtual bool sendTelemetry(const NodeAddr& configuratorAddr, const Telemetry& telem);
    virtual void setTelemetryReceivedCB(std::function<void(Protocol*, const NodeAddr&, uint8_t seqnum, const Telemetry&)>);
    virtual void telemetryReceived(const NodeAddr& source, uint8_t seqnum, const Telemetry& telem);
    /**
     * @}
     */

    //! Register a watchdog to be called if communication has timed out.
    virtual void setCommTimeoutWatchdog(float seconds, std::function<void(Protocol*,float)> commTimeoutWD);

    //! Register a callback this protocol will call as it's being destroyed.
    virtual void addDestroyCB(std::function<void(Protocol*)> fn);

    //! Print protocol info.
    virtual void printInfo();

    //! Return the current packet sequence number.
    virtual uint8_t seqnum() { return seqnum_; }

protected:
    virtual bool connect(const NodeAddr& addr) { return false; }
    virtual void commHappened();

    std::vector<NodeDescription> discoveredNodes_;
    std::vector<NodeDescription> pairedNodes_;
    Transmitter* transmitter_ = nullptr;
    Receiver* receiver_ = nullptr;
    Configurator* configurator_ = nullptr;
    std::string nodeName_, storageName_;

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