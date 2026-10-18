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


#include "config.h"
#include "WPWebSocketChannel.h"

#include "WPWebSocketTask.h"
#include <JavaScriptCore/ConsoleTypes.h>
#include <WebCore/Blob.h>
#include <WebCore/Document.h>
#include <WebCore/DocumentInlines.h>
#include <WebCore/ExceptionCode.h>
#include <WebCore/WebSocketChannelClient.h>
#include <WebCore/WebTransportSession.h>
#include <wtf/CheckedArithmetic.h>
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/RunLoop.h>
#include <wtf/text/MakeString.h>

namespace WKWP {
using namespace WebCore;

static size_t s_channelCount;

Ref<WPWebSocketChannel> WPWebSocketChannel::create(Document& document, WebSocketChannelClient& client, IsInitiatedByDedicatedWorker isInitiatedByDedicatedWorker)
{
    return adoptRef(*new WPWebSocketChannel(document, client, isInitiatedByDedicatedWorker));
}

void WPWebSocketChannel::notifySendFrame(WebSocketFrame::OpCode opCode, std::span<const uint8_t> data)
{
    WebSocketFrame frame(opCode, true, false, true, data);
    m_inspector.didSendWebSocketFrame(frame);
}

Ref<NetworkSendQueue> WPWebSocketChannel::createMessageQueue(Document& document, WPWebSocketChannel& channel)
{
    return NetworkSendQueue::create(document, [weakChannel = WeakPtr { channel }](auto& utf8String) {
        RefPtr channel = weakChannel.get();
        if (!channel)
            return;
        auto data = byteCast<uint8_t>(utf8String.span());
        channel->notifySendFrame(WebSocketFrame::OpCode::OpCodeText, data);
        size_t byteLength = utf8String.length();
        if (RefPtr task = channel->m_task) {
            task->sendString(data, [channel, byteLength] {
                channel->decreaseBufferedAmount(byteLength);
            });
        } else
            channel->decreaseBufferedAmount(byteLength);
    }, [weakChannel = WeakPtr { channel }](auto span) {
        RefPtr channel = weakChannel.get();
        if (!channel)
            return;
        channel->notifySendFrame(WebSocketFrame::OpCode::OpCodeBinary, span);
        size_t byteLength = span.size();
        if (RefPtr task = channel->m_task) {
            task->sendData(span, [channel, byteLength] {
                channel->decreaseBufferedAmount(byteLength);
            });
        } else
            channel->decreaseBufferedAmount(byteLength);
    }, [weakChannel = WeakPtr { channel }](ExceptionCode exceptionCode) {
        RefPtr channel = weakChannel.get();
        if (!channel)
            return NetworkSendQueue::Continue::No;
        auto code = static_cast<int>(exceptionCode);
        channel->fail(makeString("Failed to load Blob: exception code = "_s, code));
        return NetworkSendQueue::Continue::No;
    });
}

WPWebSocketChannel::WPWebSocketChannel(Document& document, WebSocketChannelClient& client, IsInitiatedByDedicatedWorker isInitiatedByDedicatedWorker)
    : m_document(document)
    , m_client(client)
    , m_messageQueue(createMessageQueue(document, *this))
    , m_inspector(document)
    , m_isInitiatedByDedicatedWorker(isInitiatedByDedicatedWorker)
    , m_errorTimer(*this, &WPWebSocketChannel::sendDelayedError)
{
    ++s_channelCount;
}

WPWebSocketChannel::~WPWebSocketChannel()
{
    closeTask(ThreadableWebSocketChannel::CloseEventCodeGoingAway, { });
    --s_channelCount;
}

void WPWebSocketChannel::closeTask(int32_t code, const String& reason)
{
    RefPtr task = std::exchange(m_task, nullptr);
    if (!task)
        return;
    task->close(code, reason);
    task->cancel();
}

String WPWebSocketChannel::subprotocol()
{
    return m_subprotocol.isNull() ? emptyString() : m_subprotocol;
}

String WPWebSocketChannel::extensions()
{
    return m_extensions.isNull() ? emptyString() : m_extensions;
}

WPWebSocketChannel::ConnectStatus WPWebSocketChannel::connect(const URL& url, const String& protocol)
{
    RefPtr document = m_document.get();
    if (!document)
        return ConnectStatus::KO;

    if (s_channelCount > maximumSocketCount) {
        auto reason = "Connection failed: Insufficient resources"_s;
        logErrorMessage(reason);
        if (RefPtr client = m_client.get())
            client->didReceiveMessageError(String { reason });
        return ConnectStatus::KO;
    }

    auto request = webSocketConnectRequest(*document, url);
    if (!request)
        return ConnectStatus::KO;

    if (request->url() != url) {
        if (RefPtr client = m_client.get())
            client->didUpgradeURL();
    }

    m_inspector.didCreateWebSocket(url);
    m_url = request->url();
    m_inspector.willSendWebSocketHandshakeRequest(*request);

    m_task = WPWebSocketTask::create(*this, *request, protocol);
    return ConnectStatus::OK;
}

bool WPWebSocketChannel::increaseBufferedAmount(size_t byteLength)
{
    if (!byteLength)
        return true;

    CheckedSize checkedNewBufferedAmount = m_bufferedAmount;
    checkedNewBufferedAmount += byteLength;
    if (checkedNewBufferedAmount.hasOverflowed()) [[unlikely]] {
        fail("Failed to send WebSocket frame: buffer has no more space"_s);
        return false;
    }

    m_bufferedAmount = checkedNewBufferedAmount;
    if (RefPtr client = m_client.get())
        client->didUpdateBufferedAmount(m_bufferedAmount);
    return true;
}

void WPWebSocketChannel::decreaseBufferedAmount(size_t byteLength)
{
    if (!byteLength)
        return;

    ASSERT(m_bufferedAmount >= byteLength);
    m_bufferedAmount -= std::min(byteLength, m_bufferedAmount);
    if (RefPtr client = m_client.get())
        client->didUpdateBufferedAmount(m_bufferedAmount);
}

void WPWebSocketChannel::send(CString&& message)
{
    if (!increaseBufferedAmount(message.length()))
        return;

    m_messageQueue->enqueue(WTF::move(message));
}

void WPWebSocketChannel::send(const JSC::ArrayBuffer& binaryData, unsigned byteOffset, unsigned byteLength)
{
    if (!increaseBufferedAmount(byteLength))
        return;

    m_messageQueue->enqueue(binaryData, byteOffset, byteLength);
}

void WPWebSocketChannel::send(Blob& blob)
{
    auto byteLength = blob.size();
    if (!blob.size())
        return send(JSC::ArrayBuffer::create(byteLength, 1), 0, 0);

    if (!increaseBufferedAmount(byteLength))
        return;

    m_messageQueue->enqueue(blob);
}

void WPWebSocketChannel::close(int code, const String& reason)
{
    Ref protectedThis { *this };

    m_isClosing = true;
    if (RefPtr client = m_client.get())
        client->didStartClosingHandshake();

    ASSERT(code >= 0 || code == ThreadableWebSocketChannel::CloseEventCodeNotSpecified);

    WebSocketFrame closingFrame(WebSocketFrame::OpCodeClose, true, false, true);
    m_inspector.didSendWebSocketFrame(closingFrame);

    if (RefPtr task = m_task)
        task->close(code, reason);
}

void WPWebSocketChannel::fail(String&& reason)
{
    Ref protectedThis { *this };

    logErrorMessage(reason);
    if (RefPtr client = m_client.get())
        client->didReceiveMessageError(String { reason });

    if (m_isClosing)
        return;

    closeTask(ThreadableWebSocketChannel::CloseEventCodeGoingAway, reason);
    deliverDidClose(ThreadableWebSocketChannel::CloseEventCodeAbnormalClosure, { });
}

void WPWebSocketChannel::disconnect()
{
    m_client = nullptr;
    m_document = nullptr;
    m_messageQueue->clear();
    m_errorTimer.stop();

    m_inspector.didCloseWebSocket();

    closeTask(ThreadableWebSocketChannel::CloseEventCodeGoingAway, { });
}

void WPWebSocketChannel::didConnect(String&& subprotocol, String&& extensions)
{
    RunLoop::mainSingleton().dispatch([weakThis = WeakPtr { *this }, subprotocol = WTF::move(subprotocol), extensions = WTF::move(extensions)]() mutable {
        if (RefPtr protectedThis = weakThis.get())
            protectedThis->deliverDidConnect(WTF::move(subprotocol), WTF::move(extensions));
    });
}

void WPWebSocketChannel::didReceiveText(String&& message)
{
    RunLoop::mainSingleton().dispatch([weakThis = WeakPtr { *this }, message = WTF::move(message)]() mutable {
        if (RefPtr protectedThis = weakThis.get())
            protectedThis->deliverText(WTF::move(message));
    });
}

void WPWebSocketChannel::didReceiveBinaryData(std::span<const uint8_t> data)
{
    RunLoop::mainSingleton().dispatch([weakThis = WeakPtr { *this }, data = Vector<uint8_t> { data }]() mutable {
        if (RefPtr protectedThis = weakThis.get())
            protectedThis->deliverBinaryData(WTF::move(data));
    });
}

void WPWebSocketChannel::didClose(unsigned short code, String&& reason)
{
    if (m_errorTimer.isActive()) {
        m_closeInfo = std::make_pair(code, WTF::move(reason));
        return;
    }
    RunLoop::mainSingleton().dispatch([weakThis = WeakPtr { *this }, code, reason = WTF::move(reason)]() mutable {
        if (RefPtr protectedThis = weakThis.get())
            protectedThis->deliverDidClose(code, WTF::move(reason));
    });
}

void WPWebSocketChannel::didReceiveMessageError(String&& errorMessage)
{
    m_errorMessage = WTF::move(errorMessage);
    m_errorTimer.startOneShot(10_ms + Seconds { cryptographicallyRandomUnitInterval() * (100_ms).value() });
}

void WPWebSocketChannel::sendDelayedError()
{
    Ref protectedThis { *this };
    deliverMessageError(std::exchange(m_errorMessage, { }));
    if (auto closeInfo = std::exchange(m_closeInfo, std::nullopt))
        deliverDidClose(closeInfo->first, WTF::move(closeInfo->second));
}

void WPWebSocketChannel::didSendHandshakeRequest(ResourceRequest&& request)
{
    m_inspector.didSendWebSocketHandshakeRequest(request);
    m_handshakeRequest = WTF::move(request);
}

void WPWebSocketChannel::didReceiveHandshakeResponse(ResourceResponse&& response)
{
    response.sanitizeHTTPHeaderFields(ResourceResponse::SanitizationType::CrossOriginSafe);
    m_inspector.didReceiveWebSocketHandshakeResponse(response);
    m_handshakeResponse = WTF::move(response);
}

void WPWebSocketChannel::deliverDidConnect(String&& subprotocol, String&& extensions)
{
    if (m_isClosing)
        return;

    RefPtr client = m_client.get();
    if (!client)
        return;

    m_subprotocol = WTF::move(subprotocol);
    m_extensions = WTF::move(extensions);
    client->didConnect();
}

void WPWebSocketChannel::deliverText(String&& message)
{
    if (m_isClosing)
        return;

    if (RefPtr client = m_client.get())
        client->didReceiveMessage(WTF::move(message));
}

void WPWebSocketChannel::deliverBinaryData(Vector<uint8_t>&& data)
{
    if (m_isClosing)
        return;

    if (RefPtr client = m_client.get())
        client->didReceiveBinaryData(WTF::move(data));
}

void WPWebSocketChannel::deliverDidClose(unsigned short code, String&& reason)
{
    if (RefPtr task = std::exchange(m_task, nullptr))
        task->cancel();

    RefPtr client = m_client.get();
    if (!client)
        return;

    Ref protectedThis { *this };

    bool receivedClosingHandshake = code != ThreadableWebSocketChannel::CloseEventCodeAbnormalClosure;
    if (receivedClosingHandshake)
        client->didStartClosingHandshake();

    client->didClose(m_bufferedAmount, (m_isClosing || receivedClosingHandshake) ? WebSocketChannelClient::ClosingHandshakeComplete : WebSocketChannelClient::ClosingHandshakeIncomplete, code, reason);
}

void WPWebSocketChannel::logErrorMessage(const String& errorMessage)
{
    RefPtr document = m_document.get();
    if (!document)
        return;

    String consoleMessage;
    if (!m_url.isNull())
        consoleMessage = makeString("WebSocket connection to '"_s, m_url.string(), "' failed: "_s, errorMessage);
    else
        consoleMessage = makeString("WebSocket connection failed: "_s, errorMessage);
    document->addConsoleMessage(MessageSource::Network, MessageLevel::Error, consoleMessage);
}

void WPWebSocketChannel::deliverMessageError(String&& errorMessage)
{
    RefPtr client = m_client.get();
    if (!client)
        return;

    logErrorMessage(errorMessage);
    client->didReceiveMessageError(WTF::move(errorMessage));
}

void WPWebSocketChannel::suspend()
{
}

void WPWebSocketChannel::resume()
{
}

RefPtr<ThreadableWebSocketChannel> WPSocketProvider::createWebSocketChannel(Document& document, WebSocketChannelClient& client, IsInitiatedByDedicatedWorker isInitiatedByDedicatedWorker)
{
    return WPWebSocketChannel::create(document, client, isInitiatedByDedicatedWorker);
}

std::pair<RefPtr<WebTransportSession>, Ref<WebTransportSessionPromise>> WPSocketProvider::initializeWebTransportSession(ScriptExecutionContext&, WebTransportSessionClient&, const URL&, const WebTransportOptions&)
{
    return { nullptr, WebTransportSessionPromise::createAndReject() };
}

}
