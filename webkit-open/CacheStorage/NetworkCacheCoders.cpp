/*
 * Copyright (C) 2011, 2014-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetworkCacheCoders.h"

#include "NetworkCacheKey.h"
#include <wtf/persistence/PersistentCoders.h>
#include <wtf/persistence/PersistentDecoder.h>
#include <wtf/persistence/PersistentEncoder.h>

namespace WTF::Persistence {

void Coder<WebKit::NetworkCache::Key>::encodeForPersistence(WTF::Persistence::Encoder& encoder, const WebKit::NetworkCache::Key& instance)
{
    encoder << instance.partition();
    encoder << instance.type();
    encoder << instance.identifier();
    encoder << instance.range();
    encoder << instance.hash();
    encoder << instance.partitionHash();
}

std::optional<WebKit::NetworkCache::Key> Coder<WebKit::NetworkCache::Key>::decodeForPersistence(WTF::Persistence::Decoder& decoder)
{
    WebKit::NetworkCache::Key key;

    std::optional<String> partition;
    decoder >> partition;
    if (!partition)
        return std::nullopt;
    key.m_partition = WTF::move(*partition);

    std::optional<String> type;
    decoder >> type;
    if (!type)
        return std::nullopt;
    key.m_type = WTF::move(*type);

    std::optional<String> identifier;
    decoder >> identifier;
    if (!identifier)
        return std::nullopt;
    key.m_identifier = WTF::move(*identifier);

    std::optional<String> range;
    decoder >> range;
    if (!range)
        return std::nullopt;
    key.m_range = WTF::move(*range);

    std::optional<WebKit::NetworkCache::Key::HashType> hash;
    decoder >> hash;
    if (!hash)
        return std::nullopt;
    key.m_hash = WTF::move(*hash);

    std::optional<WebKit::NetworkCache::Key::HashType> partitionHash;
    decoder >> partitionHash;
    if (!partitionHash)
        return std::nullopt;
    key.m_partitionHash = WTF::move(*partitionHash);

    return { WTF::move(key) };
}



}
