
#pragma once

#if USE(CURL) && defined(WP81)

#include "CurlRequest.h"
#include "CurlRequestClient.h"
#include "CurlResponse.h"
#include "ResourceHandle.h"
#include "ResourceHandleClient.h"
#include "SharedBuffer.h"

namespace WebCore {

class CurlResourceHandleDelegate final : public RefCounted<CurlResourceHandleDelegate>, public CurlRequestClient {
public:
    static Ref<CurlResourceHandleDelegate> create(ResourceHandle& handle) { return adoptRef(*new CurlResourceHandleDelegate(handle)); }

    void ref() const final { RefCounted::ref(); }
    void deref() const final { RefCounted::deref(); }

    void detach() { m_handle = nullptr; }

private:
    explicit CurlResourceHandleDelegate(ResourceHandle& handle)
        : m_handle(&handle)
    {
    }

    ResourceHandle* activeHandle(CurlRequest&) const;

    void curlDidSendData(CurlRequest&, unsigned long long bytesSent, unsigned long long totalBytesToBeSent) final;
    void curlDidReceiveResponse(CurlRequest&, CurlResponse&&) final;
    void curlDidReceiveData(CurlRequest&, Ref<SharedBuffer>&&) final;
    void curlDidComplete(CurlRequest&, NetworkLoadMetrics&&) final;
    void curlDidFailWithError(CurlRequest&, ResourceError&&, CertificateInfo&&) final;

    static bool shouldStartHTTPRedirection(const ResourceResponse&);
    static bool shouldRedirectAsGET(const ResourceRequest&, const ResourceResponse&, bool crossOrigin);
    void willPerformHTTPRedirection(ResourceHandle&, ResourceResponse&&);

    ResourceHandle* m_handle;
};

}

#endif
