#include <Arduino.h>

#if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_SAMD_MKRWIFI1010

#include "BBRMESPProtocol.h"
#include "../../BBRTypes.h"

using namespace bb;
using namespace bb::rmt;

#if !defined(WRAPPEDDIFF)
#define WRAPPEDDIFF(a, b, max) ((a>=b) ? a-b : (max-b)+a)
#endif // WRAPPEDDIFF

static MESPProtocol *proto = nullptr;

static const NodeAddr broadcastAddr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00};
static esp_now_peer_info broadcastPeer = {};

MESPProtocol::MESPProtocol() {
    memcpy(broadcastPeer.peer_addr, &broadcastAddr, 6);
    broadcastPeer.channel = 0;  
    broadcastPeer.encrypt = false;
    proto = this;
    keepTempPeerMS_ = 10000;
    broadcastAdded_ = false;
}

MESPProtocol::~MESPProtocol() {
    Serial.printf("Shutting down ESP-NOW.\n");
    esp_now_unregister_recv_cb();
    esp_now_unregister_send_cb();
    esp_now_deinit();
    Serial.printf("ESP-NOW shutdown successful.\n");
}

bool MESPProtocol::init(const std::string& nodeName) {
    Serial.printf("Initializing ESP-NOW... ");

    WiFi.mode(WIFI_STA);
    if(esp_now_init() != ESP_OK) {
        Serial.printf("failure.\n");
        return false;
    }
    esp_now_register_send_cb(esp_now_send_cb_t(onDataSent));
    esp_now_register_recv_cb(esp_now_recv_cb_t(onDataReceived));

    Serial.printf("success.\n");
    Serial.printf("MAC address: ");
    Serial.println(WiFi.macAddress());

    seqnum_ = 0;

    nodeName_ = nodeName;

    return true;
}

bool MESPProtocol::deserialize(StorageBlock& block) {
	source_ = (MPacket::PacketSource)block.protocolSpecific[0];
	primary_ = (block.protocolSpecific[1] != 0);
	bool retval = MProtocol::deserialize(block);
    if(retval == false) {
        bb::rmt::printf("Error deserializing\n");
        return false;
    }

    bb::rmt::printf("Adding all node addresses\n");
    for(auto& n: pairedNodes_) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, n.addr.byte, 6);
        peerInfo.channel = 0;  
        peerInfo.encrypt = false;

        if(esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.printf("Failed to add peer %s\n", n.addr.toString().c_str());
        } else {
            Serial.printf("Success adding peer %s\n", n.addr.toString().c_str());
        }
    }
    return retval;
}


bool MESPProtocol::discoverNodes(float timeout) {
    if(!broadcastAdded_) addBroadcastAddress();
    bool res = MProtocol::discoverNodes(timeout);
    removeBroadcastAddress();
    return res;
}

void MESPProtocol::onDataSent(const unsigned char *buf, esp_now_send_status_t status) {
    if(status != ESP_OK) Serial.printf("onDataSent() received error status %d\n", status);
}

void MESPProtocol::onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    uint8_t* mac = info->src_addr;

    if(len != sizeof(MPacket)) {
        Serial.printf("onDataReceived(%02x:%02x:%02x:%02x:%02x:%02x, 0x%p, %d) - invalid size (should be %d)\n", 
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], data, len, sizeof(MPacket));
        return;
    }

    MPacket* packet = (MPacket*)data;
    if(packet->calculateCRC() != packet->crc) {
        Serial.printf("Packet received, but CRC invalid (0x%x, should be 0x%x)\n", packet->crc, packet->calculateCRC());
        return;
    }

    if(proto == nullptr) {
        Serial.printf("Packet received, but proto is NULL\n");
        return;
    }

    NodeAddr addr;
    addr.fromMACAddress(mac);
    proto->enqueuePacket(addr, *packet);
}

bool MESPProtocol::step() {
    enterPairingModeIfNecessary();
    cleanupTempPeers();

    packetQueueMutex_.lock();
    //bb::rmt::printf("%d packets in queue\n", packetQueue_.size());
    std::deque<AddrAndPacket> queue = packetQueue_;
    packetQueue_.clear();
    packetQueueMutex_.unlock();

    while(queue.size()) {
        AddrAndPacket ap = queue.front();
        queue.pop_front();
        //printf("Packet from %s type %d\n", ap.addr.toString().c_str(), ap.packet.type);
        incomingPacket(ap.addr, ap.packet);
    }

    return MProtocol::step();
}

bool MESPProtocol::sendPacket(const NodeAddr& addr, MPacket& packet, bool bumpS) {
    packet.seqnum = seqnum_;
    packet.source = source_;
    packet.crc = packet.calculateCRC();

    //bb::rmt::printf("Sending packet to %s\n", addr.toString().c_str());

    esp_err_t error = esp_now_send(addr.byte, (uint8_t*)&packet, sizeof(packet));
    if(error == ESP_OK) {
        if(bumpS) bumpSeqnum();
        return true;
    } else {
        switch(error) {
        case ESP_ERR_ESPNOW_NOT_INIT:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_NOT_INIT)\n", error);
            break;
        case ESP_ERR_ESPNOW_ARG:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_ARG)\n", error);
            break;
        case ESP_ERR_ESPNOW_NO_MEM:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_NO_MEM)\n", error);
            break;
        case ESP_ERR_ESPNOW_FULL:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_FULL)\n", error);
            break;
        case ESP_ERR_ESPNOW_NOT_FOUND:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_NOT_FOUND)\n", error);
            break;
        case ESP_ERR_ESPNOW_INTERNAL:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_INTERNAL)\n", error);
            break;
        case ESP_ERR_ESPNOW_EXIST:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_EXIST)\n", error);
            break;
        case ESP_ERR_ESPNOW_IF:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_IF)\n", error);
            break;
        case ESP_ERR_ESPNOW_CHAN:
            bb::rmt::printf("esp_now_send() returns error 0x%x (ESP_ERR_ESPNOW_CHAN)\n", error);
            break;
        default:
            bb::rmt::printf("esp_now_send() returns unknown error 0x%x\n", error);
        }
    }
    return false;
}

bool MESPProtocol::sendBroadcastPacket(MPacket& packet, bool bumpS) {
    if(sendPacket(broadcastAddr, packet, bumpS) == true) {
        return true;
    } else {
        Serial.printf("Failed to send broadcast packet\n");
        return false;
    }
}

bool MESPProtocol::acceptsPairingRequests() {
    bool isConfigurator = (configurator_ == nullptr);
    for(auto& n: pairedNodes_) {
        if(n.isConfigurator) isConfigurator = true;
    }
    return isConfigurator;
}

void MESPProtocol::enterPairingModeIfNecessary() {
    if(acceptsPairingRequests()) addBroadcastAddress();
    else removeBroadcastAddress();
}

bool MESPProtocol::incomingPairingPacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, const MPairingPacket& packet) {
    if((packet.type == packet.PAIRING_DISCOVERY_BROADCAST || 
        packet.type == packet.PAIRING_DISCOVERY_REPLY) &&
        acceptsPairingRequests()) {       
        //Serial.printf("Received discovery broadcast. Temporarily adding %s as a peer.\n", addr.toString().c_str());
        addTempPeer(addr);
    } else if((packet.type == packet.PAIRING_REQUEST)) {
        Serial.printf("Received pairing packet. Temporarily adding %s as a peer.\n", addr.toString().c_str());
        addTempPeer(addr);
    }

    return MProtocol::incomingPairingPacket(addr, source, seqnum, packet);
}

void MESPProtocol::addBroadcastAddress() {
    if(broadcastAdded_ == true) return;
    if(esp_now_add_peer(&broadcastPeer) != ESP_OK) {
        Serial.printf("Failed to add peer\n");
    } else {
        broadcastAdded_ = true;
    }
}

void MESPProtocol::removeBroadcastAddress() {
    if(broadcastAdded_ == false) return;
    if(esp_now_del_peer(broadcastAddr.byte) != ESP_OK) {
        Serial.printf("Failed to remove peer\n");
    } else {
        broadcastAdded_ = false;
    }
}

void MESPProtocol::addTempPeer(const NodeAddr& addr) {
    for(auto& t: tempPeers_) {
        if(t.addr == addr) {
            t.msAdded = millis();
            return;
        }
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, addr.byte, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    if(esp_now_add_peer(&peerInfo) != ESP_OK) {
        bb::rmt::printf("Failed to add peer\n");
    } else bb::rmt::printf("Added temp peer %s\n", addr.toString().c_str());

    tempPeers_.push_back({addr, millis()});
}

void MESPProtocol::cleanupTempPeers() {
    std::vector<TempPeer> tempTempPeers;
    for(auto& p: tempPeers_) {
        if(WRAPPEDDIFF(millis(), p.msAdded, ULONG_MAX) > keepTempPeerMS_) {
            if(isPaired(p.addr) && isDiscovered(p.addr)) {
                Serial.printf("Removing %s from temp peer list but not from ESP-NOW, we're paired or have discovered it.\n", p.addr.toString().c_str());
            } else {
                Serial.printf("Removing %s from temp peer list\n", p.addr.toString().c_str());
                //esp_now_del_peer(p.addr.byte);
            }
        } else {
            tempTempPeers.push_back(p);
        }
    }
    tempPeers_ = tempTempPeers;
}

void MESPProtocol::enqueuePacket(const NodeAddr& addr, const MPacket& packet) {
    packetQueueMutex_.lock();
    packetQueue_.push_back({addr, packet});
    packetQueueMutex_.unlock();
}

bool MESPProtocol::waitForPacket(std::function<bool(const MPacket&, const NodeAddr&)> fn, 
                                 NodeAddr& addr, MPacket& packet, 
                                 bool handleOthers, float timeout) {
    bool retval = false;

    while(true) {
        packetQueueMutex_.lock();
        while(packetQueue_.size()) {
            AddrAndPacket ap = packetQueue_.front();
            packetQueue_.pop_front();

            if(fn(ap.packet, ap.addr) == true) {
                addr = ap.addr;
                packet = ap.packet;
                retval = true;
            } else if(handleOthers == true) {
                incomingPacket(ap.addr, ap.packet);
            }
        }
        packetQueueMutex_.unlock();
        if(retval == true) return true;
        timeout -= .01;
        if(timeout < 0) break;
        delay(10);
    }
    return false;
}

#endif // #if !CONFIG_IDF_TARGET_ESP32S2 && !ARDUINO_ARCH_SAMD
