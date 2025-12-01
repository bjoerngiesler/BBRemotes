#include "BBRMPacket.h"
#include <string>

using namespace bb::rmt;

std::string bb::rmt::serializePacket(const MPacket& packet) {
    char buf[3];
    memset(buf, 0, 3);
    std::string retval = "[";
    for(unsigned int i=0; i<sizeof(packet); i++) {
        sprintf(buf, "%02x", ((uint8_t*)(&packet))[i]);
        retval += buf;
    }
    retval += "]";
    Serial.print("Serialized Packet: "); Serial.println(retval.c_str());
    return retval;
}

bool bb::rmt::deserializePacket(MPacket &packet, const std::string& str) {
    if(str.size() != 2*sizeof(packet)+2) {
        Serial.print("Error deserializing packet: Wrong size string - expected ");
        Serial.print(sizeof(packet));
        Serial.print(" characters, got ");
        Serial.println(str.size());
        return false;
    }
    if(str.rfind('[', 0) != 0) {
        Serial.println("Error deserializing packet: Packet doesn't start with '['\n");
        return false;
    }
    if(str.rfind(']', str.size()-1) != str.size()-1) {
        Serial.println("Error deserializing packet: Packet doesn't end with ']'\n");
        return false;
    }
    uint8_t* p = ((uint8_t*)&packet);
    const char* s = str.c_str()+1;
    //bb::rmt::printf("%s -- ", str.c_str());
    for(unsigned int i=0; i<sizeof(packet); i++) {
        int a;
        sscanf(s, "%02x", &a);
        *p = (uint8_t)a;
        s += 2;
        p += 1;
    }
    //bb::rmt::printf("%s\n", serializePacket(packet).c_str());

    uint8_t crc = packet.calculateCRC();
    if(crc != packet.crc) {
        Serial.println("Error deserializing packet: Wrong CRC\n");
        return false;
    }
    return true;
}
