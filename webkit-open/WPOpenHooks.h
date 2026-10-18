/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#pragma once

#include <cstdint>
#include <wtf/Forward.h>

#if defined(WKWP_OPEN_BUILDING)
#define WKWP_OPEN_EXPORT __declspec(dllexport)
#else
#define WKWP_OPEN_EXPORT __declspec(dllimport)
#endif

namespace WebCore {
class NetworkStorageSession;
struct ClientOrigin;
}

namespace WKWP::Hooks {

WKWP_OPEN_EXPORT extern WebCore::NetworkStorageSession& (*storageSession)();

WKWP_OPEN_EXPORT extern String (*indexedDatabaseDirectory)();
WKWP_OPEN_EXPORT extern void (*prepareIndexedDBDirectory)(const String& directory);
WKWP_OPEN_EXPORT extern bool (*requestIndexedDBSpace)(const String& directory, const WebCore::ClientOrigin&, uint64_t spaceRequested);

}
