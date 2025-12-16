#if !defined(BBRFACTORY_H)
#define BBRFACTORY_H

#include "BBRTypes.h"
#include "BBRProtocol.h"
#include <string>

namespace bb {
namespace rmt {

/**
 * Protocol Factory -- creates protocols and handles storing them to / retrieving them from non-volatile memory.
 */
class ProtocolFactory {
public:
    static Protocol* getOrCreateProtocol(ProtocolType type);
    static bool destroyProtocol(Protocol** proto);
    
    static std::vector<std::string> storedProtocolNames();
    static std::string lastUsedProtocolName();
    static bool setLastUsedProtocolName(const std::string& last);
    static Protocol* loadProtocol(const std::string& name);
    static Protocol* loadLastUsedProtocol();
    static bool storeProtocol(const std::string& name, Protocol* proto);
    static bool eraseProtocol(const std::string& name);
    static bool eraseAll();

    static bool commit();

    static void printStorage();

    static void setMemoryReadFunction(std::function<bool(ProtocolStorage&)> readFn);
    static void setMemoryWriteFunction(std::function<bool(const ProtocolStorage&)> writeFn);

    static void addNewProtocolCB(std::function<bool(Protocol*)> newProtoFn);
};

}; // rmt
}; // bb

#endif // BBRFACTORY_H