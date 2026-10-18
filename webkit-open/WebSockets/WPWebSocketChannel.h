/*
 * Copyright (C) 2019-2025 Apple Inc. All rights reserved.
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


#pragma once

#include <WebCore/NetworkSendQueue.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/SocketProvider.h>
#include <WebCore/ThreadableWebSocketChannel.h>
#include <WebCore/Timer.h>
#include <WebCore/WebSocketChannelInspector.h>
#include <WebCore/WebSocketFrame.h>
#include <wtf/ThreadSafeWeakPtr.h>
#include <wtf/WeakPtr.h>

namespace WebCore {
class WeakPtrImplWithEventTargetData;
}

namespace WKWP {

class WPWebSocketTask;

class WPWebSocketChannel final : public WebCore::ThreadableWebSocketChannel, public RefCounted<WPWebSocketChannel>, public CanMakeWeakPtr<WPWebSocketChannel> {
public:
    static Ref<WPWebSocketChannel> create(WebCore::Document&, WebCore::WebSocketChannelClient&, WebCore::IsInitiatedByDedicatedWorker = WebCore::IsInitiatedByDedicatedWorker::No);
    ~WPWebSocketChannel();

    void ref() const final { RefCounted::ref(); }
    void deref() const final { RefCounted::deref(); }

    void didConnect(String&& subprotocol, String&& extensions);
    void didReceiveText(String&&);
    void didReceiveBinaryData(std::span<const uint8_t>);
    void didClose(unsigned short code, String&&);
    void didReceiveMessageError(String&&);
    void didSendHandshakeRequest(WebCore::ResourceRequest&&);
    void didReceiveHandshakeResponse(WebCore::ResourceResponse&&);

    static constexpr size_t maximumSocketCount = 60;

private:
    WPWebSocketChannel(WebCore::Document&, WebCore::WebSocketChannelClient&, WebCore::IsInitiatedByDedicatedWorker);

    static Ref<WebCore::NetworkSendQueue> createMessageQueue(WebCore::Document&, WPWebSocketChannel&);

    ConnectStatus connect(const URL&, const String& protocol) final;
    String subprotocol() final;
    String extensions() final;
    void send(CString&&) final;
    void send(const JSC::ArrayBuffer&, unsigned byteOffset, unsigned byteLength) final;
    void send(WebCore::Blob&) final;
    void close(int code, const String& reason) final;
    void fail(String&& reason) final;
    void disconnect() final;
    void suspend() final;
    void resume() final;

    void notifySendFrame(WebCore::WebSocketFrame::OpCode, std::span<const uint8_t> data);
    void logErrorMessage(const String&);

    void deliverDidConnect(String&& subprotocol, String&& extensions);
    void deliverText(String&&);
    void deliverBinaryData(Vector<uint8_t>&&);
    void deliverDidClose(unsigned short code, String&&);
    void deliverMessageError(String&&);
    void sendDelayedError();
    void closeTask(int32_t code, const String& reason);

    bool increaseBufferedAmount(size_t);
    void decreaseBufferedAmount(size_t);

    const WebCore::WebSocketChannelInspector* channelInspector() const LIFETIME_BOUND final { return &m_inspector; }
    WebCore::WebSocketChannelIdentifier progressIdentifier() const final { return m_inspector.progressIdentifier(); }
    bool hasCreatedHandshake() const final { return !m_url.isNull(); }
    bool isConnected() const final { return !m_handshakeResponse.isNull(); }
    WebCore::ResourceRequest clientHandshakeRequest(const CookieGetter&) const final { return m_handshakeRequest; }
    const WebCore::ResourceResponse& serverHandshakeResponse() const LIFETIME_BOUND final { return m_handshakeResponse; }

    WeakPtr<WebCore::Document, WebCore::WeakPtrImplWithEventTargetData> m_document;
    ThreadSafeWeakPtr<WebCore::WebSocketChannelClient> m_client;
    URL m_url;
    String m_subprotocol;
    String m_extensions;
    size_t m_bufferedAmount { 0 };
    bool m_isClosing { false };
    const Ref<WebCore::NetworkSendQueue> m_messageQueue;
    WebCore::WebSocketChannelInspector m_inspector;
    WebCore::ResourceRequest m_handshakeRequest;
    WebCore::ResourceResponse m_handshakeResponse;
    WebCore::IsInitiatedByDedicatedWorker m_isInitiatedByDedicatedWorker { WebCore::IsInitiatedByDedicatedWorker::No };

    RefPtr<WPWebSocketTask> m_task;
    WebCore::Timer m_errorTimer;
    String m_errorMessage;
    std::optional<std::pair<unsigned short, String>> m_closeInfo;
};

class WPSocketProvider final : public WebCore::SocketProvider {
public:
    static Ref<WPSocketProvider> create() { return adoptRef(*new WPSocketProvider); }
    RefPtr<WebCore::ThreadableWebSocketChannel> createWebSocketChannel(WebCore::Document&, WebCore::WebSocketChannelClient&, WebCore::IsInitiatedByDedicatedWorker) final;
    std::pair<RefPtr<WebCore::WebTransportSession>, Ref<WebCore::WebTransportSessionPromise>> initializeWebTransportSession(WebCore::ScriptExecutionContext&, WebCore::WebTransportSessionClient&, const URL&, const WebCore::WebTransportOptions&) final;
};

}
