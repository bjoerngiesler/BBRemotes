#if !defined(BBRPROTOCOLTYPE_H)
#define BBRPROTOCOLTYPE_H

namespace bb {
namespace rmt {

enum ProtocolType {
    MONACO_XBEE       = 'X',
    MONACO_ESPNOW     = 'E',
    MONACO_BLE        = 'B',
    MONACO_UDP        = 'U',
    SPHERO_BLE        = 'S',
    DROIDDEPOT_BLE    = 'D',
    SPEKTRUM_DSSS     = 'd',
    INVALID_PROTOCOL  = '-'
};

};
};

#endif // BBRPROTOCOLTYPE_H