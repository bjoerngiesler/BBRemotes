#include "BBRMReceiver.h"
#include "BBRTypes.h"

using namespace bb;
using namespace bb::rmt;

bool MReceiver::incomingControlPacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, const MControlPacket& packet) {
    if(dataReceivedCB_ != nullptr) dataReceivedCB_(addr, seqnum, &packet, sizeof(packet));
    //if(packet.primary) bb::rmt::printf("Primary\n"); else bb::rmt::printf("Secondary\n");
    for(uint8_t i = 0; i<inputs_.size(); i++) {
        if(!hasMixForInput(i)) continue;
        Input& inp = inputs_[i];

        const AxisMix& mix = mixForInput(i);
        AxisID axis1 = mix.axis1;
        AxisID axis2 = mix.axis2;

        if(packet.primary && ((axis1 >= SECONDARY_ADD && axis1 != AXIS_INVALID) || (axis2 >= SECONDARY_ADD && axis2 != AXIS_INVALID))) {
            //bb::rmt::printf("Secondary axis, but primary packet - not for us\n");
            continue;
        } else if(!packet.primary && (axis1 < SECONDARY_ADD || axis2 < SECONDARY_ADD)) {
            //bb::rmt::printf("Primary axis, but secondary packet - not for us\n");
            continue;
        }

        if(axis1 >= SECONDARY_ADD && axis1 != AXIS_INVALID) {
            axis1 -= SECONDARY_ADD;
        }
        if(axis2 >= SECONDARY_ADD && axis2 != AXIS_INVALID) {
            axis2 -= SECONDARY_ADD;
        }

        float val1, val2;

        val1 = packet.getAxis(axis1, UNIT_UNITY);
        val2 = packet.getAxis(axis2, UNIT_UNITY);
        //bb::rmt::printf("axis1: %d axis2: %d val1: %f val2: %f\n", axis1, axis2, val1, val2);

        float out = mix.compute(val1, 0, 1, val2, 0, 1);

        inp.callback(out);
    }
    if(dataFinishedCB_ != nullptr) dataFinishedCB_(addr, seqnum);
    return true;
}

bool MReceiver::incomingStatePacket(const NodeAddr& addr, MPacket::PacketSource source, uint8_t seqnum, const MStatePacket& packet) {
    printf("Incoming state packet from %s, source %d, seqnum %d!\n",
           addr.toString().c_str(), source, seqnum);
    return true;
}