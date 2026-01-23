#if !defined(LIBBBREMOTES_H)
#define LIBBBREMOTES_H

#include "MCS/ESP/BBRMESPProtocol.h"
#include "MCS/XBee/BBRMXBProtocol.h"
#include "MCS/Sat/BBRMSatProtocol.h"
#include "CommercialBLE/DroidDepot/BBRDroidDepotProtocol.h"
#include "CommercialBLE/Sphero/BBRSpheroProtocol.h"

#include "BBRTypes.h"
#include "BBRUtils.h"
#include "BBRReceiver.h"
#include "BBRTransmitter.h"
#include "BBRProtocol.h"
#include "BBRProtocolFactory.h"

/**
 * @mainpage
 * 
 * This is the reference documentation for the Bavarian Builders' Remote Control library. The library lives at https://github.com/bjoerngiesler/BBRemotes, 
 * please check there for source code, concept documentation, and examples.
 *
 */

#endif // LIBBBREMOTES_H