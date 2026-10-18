/*
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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
#include "ResourceHandle.h"

#if USE(CURL) && defined(WP81)

#include "CertificateInfo.h"
#include "CurlRequest.h"
#include "CurlResourceHandleDelegate.h"
#include "CurlResponse.h"
#include "HTTPHeaderNames.h"
#include "NetworkStorageSession.h"
#include "ResourceHandleInternal.h"
#include "SameSiteInfo.h"
#include "SecurityOrigin.h"
#include "SharedBuffer.h"
#include "SynchronousLoaderClient.h"
#include <wtf/CompletionHandler.h>
#include <wtf/RunLoop.h>
#include <wtf/Threading.h>

namespace WebCore {

ResourceHandle* CurlResourceHandleDelegate::activeHandle(CurlRequest& request) const
{
    if (!m_handle || m_handle->d->m_cancelled || m_handle->d->m_curlRequest.get() != &request || !m_handle->client())
        return nullptr;
    return m_handle;
}

void CurlResourceHandleDelegate::curlDidSendData(CurlRequest& request, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    if (RefPtr handle = activeHandle(request))
        handle->client()->didSendData(handle.get(), bytesSent, totalBytesToBeSent);
}

void CurlResourceHandleDelegate::curlDidReceiveData(CurlRequest& request, Ref<SharedBuffer>&& buffer)
{
    if (RefPtr handle = activeHandle(request))
        handle->client()->didReceiveBuffer(handle.get(), buffer.get(), buffer->size());
}

void CurlResourceHandleDelegate::curlDidComplete(CurlRequest& request, NetworkLoadMetrics&& metrics)
{
    if (RefPtr handle = activeHandle(request))
        handle->client()->didFinishLoading(handle.get(), metrics);
}

void CurlResourceHandleDelegate::curlDidFailWithError(CurlRequest& request, ResourceError&& error, CertificateInfo&&)
{
    if (RefPtr handle = activeHandle(request))
        handle->client()->didFail(handle.get(), error);
}

static NetworkStorageSession* storageSession(ResourceHandle& handle)
{
    auto* context = handle.context();
    return context ? context->storageSession() : nullptr;
}

void CurlResourceHandleDelegate::curlDidReceiveResponse(CurlRequest& request, CurlResponse&& curlResponse)
{
    RefPtr handle = activeHandle(request);
    if (!handle)
        return;

    if (auto* session = storageSession(*handle)) {
        static constexpr auto setCookieHeader = "set-cookie: "_s;
        for (auto& header : curlResponse.headers) {
            if (header.startsWithIgnoringASCIICase(setCookieHeader))
                session->setCookiesFromHTTPResponse(request.resourceRequest().firstPartyForCookies(), curlResponse.url, header.substring(setCookieHeader.length()));
        }
    }

    ResourceResponse response(curlResponse);
    response.setDeprecatedNetworkLoadMetrics(Box<NetworkLoadMetrics>::create(WTF::move(curlResponse.networkLoadMetrics)));

    if (shouldStartHTTPRedirection(response)) {
        willPerformHTTPRedirection(*handle, WTF::move(response));
        return;
    }

    handle->didReceiveResponse(WTF::move(response), [handle, request = Ref { request }] {
        if (!handle->d->m_cancelled && handle->d->m_curlRequest == request.ptr())
            request->completeDidReceiveResponse();
    });
}

bool CurlResourceHandleDelegate::shouldStartHTTPRedirection(const ResourceResponse& response)
{
    auto statusCode = response.httpStatusCode();
    if (statusCode < 300 || statusCode >= 400)
        return false;

    if (statusCode == 300 || statusCode == 304 || statusCode == 305 || statusCode == 306)
        return false;

    return !response.httpHeaderField(HTTPHeaderName::Location).isEmpty();
}

bool CurlResourceHandleDelegate::shouldRedirectAsGET(const ResourceRequest& request, const ResourceResponse& response, bool crossOrigin)
{
    if (request.httpMethod() == "GET"_s || request.httpMethod() == "HEAD"_s)
        return false;

    if (!request.url().protocolIsInHTTPFamily())
        return true;

    if (response.isSeeOther())
        return true;

    if ((response.isMovedPermanently() || response.isFound()) && request.httpMethod() == "POST"_s)
        return true;

    if (crossOrigin && request.httpMethod() == "DELETE"_s)
        return true;

    return false;
}

void CurlResourceHandleDelegate::willPerformHTTPRedirection(ResourceHandle& handle, ResourceResponse&& response)
{
    static constexpr uint16_t maxRedirects = 20;

    auto& d = *handle.d;
    if (handle.redirectCount() >= maxRedirects) {
        handle.client()->didFail(&handle, ResourceError(CURLE_TOO_MANY_REDIRECTS, response.url()));
        return;
    }
    handle.incrementRedirectCount();

    URL redirectedURL { response.url(), response.httpHeaderField(HTTPHeaderName::Location) };
    if (redirectedURL.protocolIsFile()) {
        handle.client()->didFail(&handle, ResourceError(CURLE_FILE_COULDNT_READ_FILE, response.url()));
        return;
    }

    ResourceRequest request = d.m_previousRequest;
    if (!redirectedURL.hasFragmentIdentifier() && request.url().hasFragmentIdentifier())
        redirectedURL.setFragmentIdentifier(request.url().fragmentIdentifier());
    bool isCrossOrigin = !protocolHostAndPortAreEqual(request.url(), redirectedURL);
    request.setURL(WTF::move(redirectedURL));

    if (!request.url().protocolIs("https"_s) && protocolIs(request.httpReferrer(), "https"_s))
        request.clearHTTPReferrer();

    if (!equalLettersIgnoringASCIICase(request.httpMethod(), "get"_s) && shouldRedirectAsGET(request, response, isCrossOrigin)) {
        request.setHTTPMethod("GET"_s);
        request.setHTTPBody(nullptr);
        request.clearHTTPContentType();
    }

    if (isCrossOrigin) {
        request.clearHTTPAuthorization();
        request.clearHTTPOrigin();
        handle.markAsHavingCrossOriginRedirect();
    }
    request.removeCredentials();

    Ref oldRequest = *d.m_curlRequest;
    d.m_curlRequest = nullptr;
    oldRequest->cancel();

    Ref protectedHandle { handle };
    handle.client()->willSendRequestAsync(&handle, WTF::move(request), WTF::move(response), [protectedHandle](ResourceRequest&& newRequest) {
        if (newRequest.isNull() || protectedHandle->d->m_cancelled)
            return;
        protectedHandle->startCurlRequest(WTF::move(newRequest));
    });
}

ResourceHandleInternal::~ResourceHandleInternal()
{
    if (m_delegate)
        m_delegate->detach();
    if (m_curlRequest)
        m_curlRequest->invalidateClient();
}

ResourceHandle::~ResourceHandle() = default;

void ResourceHandle::startCurlRequest(ResourceRequest&& request)
{
    ASSERT(isMainThread());

    d->m_previousRequest = request;
    d->m_lastHTTPMethod = request.httpMethod();

    if (auto* session = storageSession(*this)) {
        auto includeSecureCookies = request.url().protocolIs("https"_s) ? IncludeSecureCookies::Yes : IncludeSecureCookies::No;
        auto cookieHeaderField = session->cookieRequestHeaderFieldValue(request.firstPartyForCookies(), SameSiteInfo::create(request), request.url(), std::nullopt, std::nullopt, includeSecureCookies, ApplyTrackingPrevention::Yes, ShouldRelaxThirdPartyCookieBlocking::No, IsKnownCrossSiteTracker::No).first;
        if (!cookieHeaderField.isEmpty())
            request.addHTTPHeaderField(HTTPHeaderName::Cookie, cookieHeaderField);
    }

    if (!d->m_delegate)
        d->m_delegate = CurlResourceHandleDelegate::create(*this);

    Ref curlRequest = CurlRequest::create(request, *d->m_delegate);
    if (!d->m_user.isEmpty() || !d->m_password.isEmpty()) {
        curlRequest->setUserPass(d->m_user, d->m_password);
        curlRequest->setAuthenticationScheme(ProtectionSpace::AuthenticationScheme::HTTPBasic);
    }
    d->m_curlRequest = curlRequest.copyRef();
    d->m_curlRequestResumed = !d->m_defersLoading;
    if (d->m_curlRequestResumed)
        curlRequest->resume();
}

bool ResourceHandle::start()
{
    ASSERT(isMainThread());

    if (d->m_context && !d->m_context->isValid())
        return false;

    auto request = firstRequest();
    if (!request.url().protocolIsInHTTPFamily() && request.httpMethod() != "GET"_s && request.httpMethod() != "POST"_s) {
        scheduleFailure(InvalidURLFailure);
        return true;
    }

    startCurlRequest(WTF::move(request));
    return true;
}

void ResourceHandle::cancel()
{
    ASSERT(isMainThread());
    d->m_cancelled = true;
    if (RefPtr curlRequest = std::exchange(d->m_curlRequest, nullptr))
        curlRequest->cancel();
}

void ResourceHandle::platformSetDefersLoading(bool defers)
{
    ASSERT(isMainThread());
    d->m_defersLoading = defers;

    if (!defers && d->m_curlRequest && !d->m_curlRequestResumed) {
        d->m_curlRequestResumed = true;
        protect(d->m_curlRequest)->resume();
    }
}

bool ResourceHandle::shouldUseCredentialStorage()
{
    return (!client() || client()->shouldUseCredentialStorage(this)) && firstRequest().url().protocolIsInHTTPFamily();
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge&)
{
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    if (client())
        client()->receivedCancellation(this, challenge);
}

void ResourceHandle::receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&)
{
}

void ResourceHandle::receivedChallengeRejection(const AuthenticationChallenge&)
{
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentialsPolicy storedCredentialsPolicy, SecurityOrigin*, ResourceError& error, ResourceResponse& response, Vector<uint8_t>& data)
{
    ASSERT(isMainThread());
    ASSERT(!request.isEmpty());

    SynchronousLoaderClient client;
    client.setAllowStoredCredentials(storedCredentialsPolicy == StoredCredentialsPolicy::Use);

    Ref handle = adoptRef(*new ResourceHandle(context, request, &client, false, true, ContentEncodingSniffingPolicy::Default, nullptr, false));
    if (handle->d->m_scheduledFailureType != NoFailure) {
        error = ResourceError(CURLE_URL_MALFORMAT, request.url());
        return;
    }
    handle->start();

    while (!client.messageQueue().killed()) {
        RunLoop::cycle();
        if (!client.messageQueue().killed())
            ::Sleep(1);
    }

    error = client.error();
    data.swap(client.mutableData());
    response = client.response();
}

}

#endif
