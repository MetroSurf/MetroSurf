/*
 * Copyright (C) 2022 Sony Interactive Entertainment Inc.
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
#include "WPWebSocketTask.h"

#include "WPOpenHooks.h"
#include "WPWebSocketChannel.h"
#include <WebCore/CurlContext.h>
#include <WebCore/CurlStreamScheduler.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/SameSiteInfo.h>
#include <WebCore/SharedBuffer.h>
#include <WebCore/WebSocketHandshake.h>
#include <wtf/Borrow.h>
#include <wtf/RunLoop.h>
#include <wtf/text/MakeString.h>

namespace WKWP {

Ref<WPWebSocketTask> WPWebSocketTask::create(WPWebSocketChannel& channel, const WebCore::ResourceRequest& request, const String& protocol)
{
    return adoptRef(*new WPWebSocketTask(channel, request, protocol));
}

WPWebSocketTask::WPWebSocketTask(WPWebSocketChannel& channel, const WebCore::ResourceRequest& request, const String& protocol)
    : m_channel(channel)
    , m_request(request.isolatedCopy())
    , m_protocol(protocol)
    , m_scheduler(WebCore::CurlContext::singleton().streamScheduler())
{
    m_streamID = m_scheduler.createStream(request.url(), *this, WebCore::CurlStream::ServerTrustEvaluation::Enable, WebCore::CurlStream::LocalhostAlias::Disable);
}

WPWebSocketTask::~WPWebSocketTask()
{
    destructStream();
}

void WPWebSocketTask::sendString(std::span<const uint8_t> utf8, CompletionHandler<void()>&& callback)
{
    if (m_state == State::Opened) {
        if (!sendFrame(WebCore::WebSocketFrame::OpCodeText, utf8))
            didFail("Failed to send WebSocket frame."_s);
    }
    callback();
}

void WPWebSocketTask::sendData(std::span<const uint8_t> data, CompletionHandler<void()>&& callback)
{
    if (m_state == State::Opened) {
        if (!sendFrame(WebCore::WebSocketFrame::OpCodeBinary, data))
            didFail("Failed to send WebSocket frame."_s);
    }
    callback();
}

void WPWebSocketTask::close(int32_t code, const String& reason)
{
    if (m_state == State::Closed)
        return;

    if (m_state == State::Connecting || m_state == State::Handshaking) {
        didClose(WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeAbnormalClosure, { });
        return;
    }

    sendClosingHandshakeIfNeeded(code, reason);
}

void WPWebSocketTask::cancel()
{
    m_channel = nullptr;
    m_state = State::Closed;
    destructStream();
}

void WPWebSocketTask::didOpen(WebCore::CurlStreamID)
{
    if (m_state != State::Connecting)
        return;

    m_state = State::Handshaking;

    m_handshake = makeUnique<WebCore::WebSocketHandshake>(m_request.url(), m_protocol, m_request.httpUserAgent(), m_request.httpHeaderField(WebCore::HTTPHeaderName::Origin), m_request.allowCookies(), false);
    m_handshake->reset();
    m_handshake->addExtensionProcessor(m_deflateFramer.createExtensionProcessor());
    m_handshake->setClientHandshakeRequestHeaders(m_request.httpHeaderFields());

    String cookieHeaderField;
    if (m_request.allowCookies()) {
        auto includeSecureCookies = m_request.url().protocolIs("wss"_s) ? WebCore::IncludeSecureCookies::Yes : WebCore::IncludeSecureCookies::No;
        cookieHeaderField = WKWP::Hooks::storageSession().cookieRequestHeaderFieldValue(m_request.firstPartyForCookies(), WebCore::SameSiteInfo::create(m_request), m_request.url(), std::nullopt, std::nullopt, includeSecureCookies, WebCore::ApplyTrackingPrevention::Yes, WebCore::ShouldRelaxThirdPartyCookieBlocking::No, WebCore::IsKnownCrossSiteTracker::No).first;
    }

    CString cookieHeader;
    if (!cookieHeaderField.isEmpty())
        cookieHeader = makeString("Cookie: "_s, cookieHeaderField, "\r\n"_s).utf8();

    auto originalMessage = m_handshake->clientHandshakeMessage();
    auto handshakeMessageLength = originalMessage.length() + cookieHeader.length();
    auto handshakeMessage = makeUniqueArray<uint8_t>(handshakeMessageLength);

    memcpy(handshakeMessage.get(), originalMessage.data(), originalMessage.length());
    if (!cookieHeader.isNull() && cookieHeader.length()) {
        memcpy(handshakeMessage.get() + originalMessage.length() - 2, cookieHeader.data(), cookieHeader.length());
        memcpy(handshakeMessage.get() + handshakeMessageLength - 2, "\r\n", 2);
    }

    m_scheduler.send(m_streamID, WTF::move(handshakeMessage), handshakeMessageLength);

    auto handshakeRequest = m_handshake->clientHandshakeRequest([&cookieHeaderField] (const URL&) {
        return cookieHeaderField;
    });
    auto inspectorRequest = m_request;
    inspectorRequest.setHTTPMethod(handshakeRequest.httpMethod());
    inspectorRequest.setHTTPHeaderFields(handshakeRequest.httpHeaderFields());
    if (RefPtr channel = m_channel.get())
        channel->didSendHandshakeRequest(WTF::move(inspectorRequest));
}

void WPWebSocketTask::didReceiveData(WebCore::CurlStreamID, const WebCore::SharedBuffer& buffer)
{
    if (m_state == State::Connecting || m_state == State::Closed)
        return;

    if (!buffer.size()) {
        didClose(WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeAbnormalClosure, { });
        return;
    }

    if (m_shouldDiscardReceivedData || m_receivedClosingHandshake)
        return;

    if (!appendReceivedBuffer(buffer)) {
        didFail("WebSocket message too big."_s);
        return;
    }

    auto validateResult = validateOpeningHandshake();
    if (!validateResult.has_value()) {
        didFail(String(validateResult.error()));
        return;
    }
    if (!validateResult.value())
        return;

    auto frameResult = receiveFrames([this, weakThis = ThreadSafeWeakPtr { *this }](WebCore::WebSocketFrame::OpCode opCode, std::span<const uint8_t> data) {
        RefPtr protectedThis = weakThis.get();
        if (!protectedThis)
            return;

        switch (opCode) {
        case WebCore::WebSocketFrame::OpCodeText:
            {
                String message = data.size() ? String::fromUTF8(data) : emptyString();
                if (!message.isNull()) {
                    if (RefPtr channel = m_channel.get())
                        channel->didReceiveText(WTF::move(message));
                } else
                    didFail("Could not decode a text frame as UTF-8."_s);
            }
            break;

        case WebCore::WebSocketFrame::OpCodeBinary:
            if (RefPtr channel = m_channel.get())
                channel->didReceiveBinaryData(data);
            break;

        case WebCore::WebSocketFrame::OpCodeClose:
            if (!data.size())
                m_closeEventCode = WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeNoStatusRcvd;
            else if (data.size() == 1) {
                m_closeEventCode = WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeAbnormalClosure;
                didFail("Received a broken close frame containing an invalid size body."_s);
                return;
            } else {
                auto highByte = static_cast<unsigned char>(data[0]);
                auto lowByte = static_cast<unsigned char>(data[1]);
                m_closeEventCode = highByte << 8 | lowByte;
                if (m_closeEventCode == WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeNoStatusRcvd
                    || m_closeEventCode == WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeAbnormalClosure
                    || m_closeEventCode == WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeTLSHandshake) {
                    m_closeEventCode = WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeAbnormalClosure;
                    didFail("Received a broken close frame containing a reserved status code."_s);
                    return;
                }
            }
            if (data.size() >= 3)
                m_closeEventReason = String::fromUTF8(data.subspan(2));
            else
                m_closeEventReason = emptyString();

            m_receivedClosingHandshake = true;
            sendClosingHandshakeIfNeeded(m_closeEventCode, m_closeEventReason);
            didClose(m_closeEventCode, m_closeEventReason);
            break;

        case WebCore::WebSocketFrame::OpCodePing:
            if (!sendFrame(WebCore::WebSocketFrame::OpCodePong, data))
                didFail("Failed to send WebSocket frame."_s);
            break;

        case WebCore::WebSocketFrame::OpCodeContinuation:
        case WebCore::WebSocketFrame::OpCodePong:
        case WebCore::WebSocketFrame::OpCodeInvalid:
            break;
        }
    });

    if (frameResult) {
        didFail(String(*frameResult));
        return;
    }

    if (m_receiveBuffer.capacity() > 64 * KB && m_receiveBuffer.size() < m_receiveBuffer.capacity() / 4)
        m_receiveBuffer.shrinkToFit();
}

void WPWebSocketTask::didFail(WebCore::CurlStreamID, CURLcode errorCode, WebCore::CertificateInfo&&)
{
    if (errorCode == CURLE_PEER_FAILED_VERIFICATION) {
        didFail("WebSocket network error: the server's certificate could not be verified."_s);
        return;
    }
    didFail(makeString("WebSocket network error: error code "_s, static_cast<uint32_t>(errorCode), m_curlErrorMessage.isEmpty() ? ""_s : " ("_s, m_curlErrorMessage, m_curlErrorMessage.isEmpty() ? ""_s : ")"_s));
}

bool WPWebSocketTask::appendReceivedBuffer(const WebCore::SharedBuffer& buffer)
{
    size_t newBufferSize = m_receiveBuffer.size() + buffer.size();
    if (newBufferSize < m_receiveBuffer.size())
        return false;

    if (newBufferSize > maximumMessageSize + 16) {
        m_closeEventCode = WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeMessageTooBig;
        return false;
    }

    m_receiveBuffer.append(buffer.span());
    return true;
}

void WPWebSocketTask::skipReceivedBuffer(size_t length)
{
    memmoveSpan(m_receiveBuffer.mutableSpan(), m_receiveBuffer.subspan(length));
    m_receiveBuffer.shrink(m_receiveBuffer.size() - length);
}

Expected<bool, String> WPWebSocketTask::validateOpeningHandshake()
{
    if (m_didCompleteOpeningHandshake)
        return true;

    if (m_state != State::Handshaking || !m_handshake || m_handshake->mode() != WebCore::WebSocketHandshake::Incomplete) {
        m_handshake = nullptr;
        return makeUnexpected("Unexpected handshakeing condition"_s);
    }

    auto headerLength = m_handshake->readServerHandshake(m_receiveBuffer.span());
    if (headerLength <= 0)
        return false;

    skipReceivedBuffer(headerLength);

    if (m_handshake->mode() != WebCore::WebSocketHandshake::Connected) {
        auto reason = m_handshake->failureReason();
        if (!m_handshake->serverHandshakeResponse().isNull()) {
            if (RefPtr channel = m_channel.get())
                channel->didReceiveHandshakeResponse(WebCore::ResourceResponse(m_handshake->serverHandshakeResponse()));
        }
        m_handshake = nullptr;
        return makeUnexpected(reason);
    }

    auto serverSetCookie = m_handshake->serverSetCookie();
    if (!serverSetCookie.isEmpty())
        WKWP::Hooks::storageSession().setCookiesFromHTTPResponse(m_request.firstPartyForCookies(), m_request.url(), serverSetCookie);

    m_state = State::Opened;
    m_didCompleteOpeningHandshake = true;

    if (RefPtr channel = m_channel.get()) {
        channel->didConnect(m_handshake->serverWebSocketProtocol(), m_handshake->acceptedExtensions());
        channel->didReceiveHandshakeResponse(WebCore::ResourceResponse(m_handshake->serverHandshakeResponse()));
    }
    m_handshake = nullptr;
    return true;
}

std::optional<String> WPWebSocketTask::receiveFrames(Function<void(WebCore::WebSocketFrame::OpCode, std::span<const uint8_t>)>&& callback)
{
    if (m_state != State::Opened && m_state != State::Closing)
        return std::nullopt;

    while (m_receiveBuffer.size() && !m_shouldDiscardReceivedData && !m_receivedClosingHandshake) {
        WebCore::WebSocketFrame frame;
        const uint8_t* frameEnd;
        String errorString;
        auto parseResult = WebCore::WebSocketFrame::parseFrame(m_receiveBuffer.mutableSpan(), frame, frameEnd, errorString);
        if (parseResult == WebCore::WebSocketFrame::FrameIncomplete)
            return std::nullopt;
        if (parseResult == WebCore::WebSocketFrame::FrameError)
            return errorString;

        auto inflateResult = m_deflateFramer.inflate(frame);
        if (!inflateResult->succeeded())
            return inflateResult->failureReason();

        if (auto validateResult = validateFrame(frame))
            return *validateResult;

        if (!frame.final || frame.opCode == WebCore::WebSocketFrame::OpCodeContinuation) {
            if (frame.opCode != WebCore::WebSocketFrame::OpCodeContinuation) {
                m_hasContinuousFrame = true;
                m_continuousFrameOpCode = frame.opCode;
            }

            if (m_continuousFrameData.size() + frame.payload.size() > maximumMessageSize) {
                m_closeEventCode = WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeMessageTooBig;
                return "WebSocket message too big."_s;
            }
            m_continuousFrameData.append(frame.payload);

            if (frame.final) {
                callback(m_continuousFrameOpCode, borrow(m_continuousFrameData)->span());
                m_hasContinuousFrame = false;
                m_continuousFrameData.clear();
            }
        } else {
            if (frame.payload.size() > maximumMessageSize) {
                m_closeEventCode = WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeMessageTooBig;
                return "WebSocket message too big."_s;
            }
            callback(frame.opCode, frame.payload);
        }

        if (!m_receiveBuffer.isEmpty())
            skipReceivedBuffer(frameEnd - m_receiveBuffer.begin());
    }

    return std::nullopt;
}

std::optional<String> WPWebSocketTask::validateFrame(const WebCore::WebSocketFrame& frame)
{
    if (WebCore::WebSocketFrame::isReservedOpCode(frame.opCode))
        return makeString("Unrecognized frame opcode: "_s, static_cast<unsigned>(frame.opCode));

    if (frame.reserved2 || frame.reserved3)
        return makeString("One or more reserved bits are on: reserved2 = "_s, static_cast<unsigned>(frame.reserved2), ", reserved3 = "_s, static_cast<unsigned>(frame.reserved3));

    if (frame.masked)
        return "A server must not mask any frames that it sends to the client."_s;

    if (WebCore::WebSocketFrame::isControlOpCode(frame.opCode) && !frame.final)
        return makeString("Received fragmented control frame: opcode = "_s, static_cast<unsigned>(frame.opCode));

    if (WebCore::WebSocketFrame::isControlOpCode(frame.opCode) && WebCore::WebSocketFrame::needsExtendedLengthField(frame.payload.size()))
        return makeString("Received control frame having too long payload: "_s, frame.payload.size(), " bytes"_s);

    if (m_hasContinuousFrame && frame.opCode != WebCore::WebSocketFrame::OpCodeContinuation && !WebCore::WebSocketFrame::isControlOpCode(frame.opCode))
        return "Received new data frame but previous continuous frame is unfinished."_s;

    if (!m_hasContinuousFrame && frame.opCode == WebCore::WebSocketFrame::OpCodeContinuation)
        return "Received unexpected continuation frame."_s;

    return std::nullopt;
}

void WPWebSocketTask::sendClosingHandshakeIfNeeded(int32_t code, const String& reason)
{
    if (m_didSendClosingHandshake)
        return;

    Vector<uint8_t> buf;
    if (!m_receivedClosingHandshake && code != WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeNotSpecified) {
        unsigned char highByte = static_cast<unsigned short>(code) >> 8;
        unsigned char lowByte = static_cast<unsigned short>(code);
        buf.append(static_cast<char>(highByte));
        buf.append(static_cast<char>(lowByte));
        buf.append(reason.utf8().span());
    }

    if (!sendFrame(WebCore::WebSocketFrame::OpCodeClose, buf.span()))
        didFail("Failed to send WebSocket frame."_s);

    m_state = State::Closing;
    m_didSendClosingHandshake = true;
}

bool WPWebSocketTask::sendFrame(WebCore::WebSocketFrame::OpCode opCode, std::span<const uint8_t> data)
{
    if (m_didSendClosingHandshake)
        return true;

    WebCore::WebSocketFrame frame(opCode, true, false, true, data);

    auto deflateResult = m_deflateFramer.deflate(frame);
    if (!deflateResult->succeeded()) {
        didFail(deflateResult->failureReason());
        return false;
    }

    Vector<uint8_t> frameData;
    frame.makeFrameData(frameData);

    auto buffer = makeUniqueArray<uint8_t>(frameData.size());
    memcpySpan(unsafeMakeSpan(buffer.get(), frameData.size()), frameData.span());

    m_scheduler.send(m_streamID, WTF::move(buffer), frameData.size());
    return true;
}

void WPWebSocketTask::didFail(String&& reason)
{
    if (m_receivedDidFail)
        return;

    m_receivedDidFail = true;

    m_shouldDiscardReceivedData = true;
    if (!m_receiveBuffer.isEmpty())
        skipReceivedBuffer(m_receiveBuffer.size());
    m_deflateFramer.didFail();
    m_hasContinuousFrame = false;
    m_continuousFrameData.clear();

    if (RefPtr channel = m_channel.get())
        channel->didReceiveMessageError(WTF::move(reason));
    auto code = m_closeEventCode == WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeMessageTooBig
        ? m_closeEventCode : WebCore::ThreadableWebSocketChannel::CloseEventCode::CloseEventCodeAbnormalClosure;
    didClose(code, { });
}

void WPWebSocketTask::didClose(int32_t code, const String& reason)
{
    destructStream();

    if (m_state == State::Closed)
        return;

    m_state = State::Closed;

    RunLoop::mainSingleton().dispatch([weakThis = ThreadSafeWeakPtr { *this }, code, reason] {
        RefPtr protectedThis = weakThis.get();
        if (!protectedThis)
            return;
        if (RefPtr channel = protectedThis->m_channel.get())
            channel->didClose(code, String { reason });
    });
}

void WPWebSocketTask::destructStream()
{
    if (isStreamInvalidated())
        return;

    m_scheduler.destroyStream(m_streamID);
    m_streamID = WebCore::invalidCurlStreamID;
}

}
