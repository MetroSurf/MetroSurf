
#include "config.h"
#include "WebResourceLoadScheduler.h"

#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>

using namespace WebCore;

static constexpr auto webKitErrorDomain = "WebKitErrorDomain"_s;

enum {
    WebKitErrorCannotShowMIMEType = 100,
    WebKitErrorCannotShowURL = 101,
    WebKitErrorFrameLoadInterruptedByPolicyChange = 102,
    WebKitErrorCannotUseRestrictedPort = 103,
    WebKitErrorFrameLoadBlockedByContentBlocker = 104,
    WebKitErrorFrameLoadBlockedByContentFilter = 105,
    WebKitErrorFileDoesNotExist = 106,
    WebKitErrorPlugInWillHandleLoad = 204,
    WebKitErrorHTTPSUpgradeRedirectLoop = 304,
    WebKitErrorHTTPNavigationWithHTTPSOnlyError = 305,
    WebKitErrorCancelled = 1000,
};

static ResourceError webKitError(int code, const URL& url, ASCIILiteral description, ResourceError::Type type = ResourceError::Type::General)
{
    return ResourceError(webKitErrorDomain, code, url, description, type);
}

ResourceError WebResourceLoadScheduler::pluginWillHandleLoadError(const ResourceResponse& response) const
{
    return pluginWillHandleLoadErrorFromResponse(response);
}

ResourceError WebResourceLoadScheduler::pluginWillHandleLoadErrorFromResponse(const ResourceResponse& response)
{
    return webKitError(WebKitErrorPlugInWillHandleLoad, response.url(), "Plug-in handled load"_s);
}

ResourceError WebResourceLoadScheduler::cancelledError(const ResourceRequest& request) const
{
    return webKitError(WebKitErrorCancelled, request.url(), "Load cancelled"_s, ResourceError::Type::Cancellation);
}

ResourceError WebResourceLoadScheduler::blockedError(const ResourceRequest& request) const
{
    return blockedErrorFromRequest(request);
}

ResourceError WebResourceLoadScheduler::blockedErrorFromRequest(const ResourceRequest& request)
{
    return webKitError(WebKitErrorCannotUseRestrictedPort, request.url(), "Not allowed to use restricted network port"_s);
}

bool WebResourceLoadScheduler::isBlockedError(const ResourceError& error) const
{
    return error.domain() == webKitErrorDomain && error.errorCode() == WebKitErrorCannotUseRestrictedPort;
}

ResourceError WebResourceLoadScheduler::blockedByContentBlockerError(const ResourceRequest& request) const
{
    return webKitError(WebKitErrorFrameLoadBlockedByContentBlocker, request.url(), "The URL was blocked by a content blocker"_s);
}

ResourceError WebResourceLoadScheduler::cannotShowURLError(const ResourceRequest& request) const
{
    return webKitError(WebKitErrorCannotShowURL, request.url(), "The URL can't be shown"_s);
}

ResourceError WebResourceLoadScheduler::interruptedForPolicyChangeError(const ResourceRequest& request) const
{
    return webKitError(WebKitErrorFrameLoadInterruptedByPolicyChange, request.url(), "Frame load interrupted"_s);
}

#if ENABLE(CONTENT_FILTERING)
ResourceError WebResourceLoadScheduler::blockedByContentFilterError(const ResourceRequest& request) const
{
    return webKitError(WebKitErrorFrameLoadBlockedByContentFilter, request.url(), "The URL was blocked by a content filter"_s);
}
#endif

ResourceError WebResourceLoadScheduler::cannotShowMIMETypeError(const ResourceResponse& response) const
{
    return webKitError(WebKitErrorCannotShowMIMEType, response.url(), "Content with specified MIME type can't be shown"_s);
}

ResourceError WebResourceLoadScheduler::fileDoesNotExistError(const ResourceResponse& response) const
{
    return webKitError(WebKitErrorFileDoesNotExist, response.url(), "The file does not exist"_s);
}

ResourceError WebResourceLoadScheduler::httpsUpgradeRedirectLoopError(const ResourceRequest& request) const
{
    return webKitError(WebKitErrorHTTPSUpgradeRedirectLoop, request.url(), "HTTPS upgrade redirect loop detected"_s);
}

ResourceError WebResourceLoadScheduler::httpNavigationWithHTTPSOnlyError(const ResourceRequest& request) const
{
    return webKitError(WebKitErrorHTTPNavigationWithHTTPSOnlyError, request.url(), "Navigation failed because the request was for an HTTP URL with HTTPS-Only enabled"_s);
}

bool WebResourceLoadScheduler::isHttpNavigationWithHTTPSOnlyError(const ResourceError& error) const
{
    return error.domain() == webKitErrorDomain && error.errorCode() == WebKitErrorHTTPNavigationWithHTTPSOnlyError;
}
