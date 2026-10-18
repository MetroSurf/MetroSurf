#pragma once
#include <wtf/ObjectIdentifier.h>

namespace IPC {
struct Connection {
    enum class UniqueIDType { };
    using UniqueID = AtomicObjectIdentifier<UniqueIDType, uint64_t>;
};
}
