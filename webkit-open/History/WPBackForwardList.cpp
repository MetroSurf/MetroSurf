/*
 * Copyright (C) 2005, 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WPBackForwardList.h"

#include <WebCore/BackForwardCache.h>

namespace WKWP {

using namespace WebCore;

void WPBackForwardList::addItem(Ref<HistoryItem>&& newItem)
{
    if (m_current != noCurrentItemIndex) {
        unsigned targetSize = m_current + 1;
        while (m_entries.size() > targetSize) {
            Ref item = m_entries.takeLast();
            m_entryHash.remove(item.ptr());
            BackForwardCache::singleton().remove(item);
        }
    }

    if (m_entries.size() == capacity && m_current) {
        Ref item = WTF::move(m_entries[0]);
        m_entries.removeAt(0);
        m_entryHash.remove(item.ptr());
        BackForwardCache::singleton().remove(item);
        --m_current;
    }

    m_entryHash.add(newItem.ptr());
    m_entries.insert(m_current + 1, WTF::move(newItem));
    ++m_current;
}

void WPBackForwardList::goToItem(HistoryItem& item)
{
    for (unsigned index = 0; index < m_entries.size(); ++index) {
        if (m_entries[index].ptr() == &item) {
            m_current = index;
            return;
        }
    }
}

unsigned WPBackForwardList::backListCount() const
{
    return m_current == noCurrentItemIndex ? 0 : m_current;
}

unsigned WPBackForwardList::forwardListCount() const
{
    return m_current == noCurrentItemIndex ? 0 : m_entries.size() - m_current - 1;
}

RefPtr<HistoryItem> WPBackForwardList::itemAtIndex(int index, FrameIdentifier)
{
    if (m_current == noCurrentItemIndex)
        return nullptr;
    if (index < -static_cast<int>(m_current) || index > static_cast<int>(forwardListCount()))
        return nullptr;
    return m_entries[index + m_current].copyRef();
}

bool WPBackForwardList::containsItem(const HistoryItem& entry) const
{
    return m_entryHash.contains(const_cast<HistoryItem*>(&entry));
}

void WPBackForwardList::close()
{
    m_entries.clear();
    m_entryHash.clear();
    m_current = noCurrentItemIndex;
}

}
