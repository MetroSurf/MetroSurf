/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#pragma once

#ifndef WEBCORE_EMPTY_CLIENT_FINAL
#if defined(WP81)
#define WEBCORE_EMPTY_CLIENT_FINAL override
#else
#define WEBCORE_EMPTY_CLIENT_FINAL final
#endif
#endif

#include <WebCore/LocalFrameLoaderClient.h>
#include <wtf/Platform.h>

namespace WebCore {

class WEBCORE_EXPORT EmptyFrameLoaderClient : public LocalFrameLoaderClient {
public:
    explicit EmptyFrameLoaderClient(FrameLoader& frameLoader)
        : LocalFrameLoaderClient(frameLoader)
    { }

private:
    Ref<DocumentLoader> createDocumentLoader(ResourceRequest&&, SubstituteData&&, ResourceRequest&&) override;
    Ref<DocumentLoader> createDocumentLoader(ResourceRequest&&, SubstituteData&&) override;

    bool NODELETE hasWebView() const WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE makeRepresentation(DocumentLoader*) WEBCORE_EMPTY_CLIENT_FINAL;

#if PLATFORM(IOS_FAMILY)
    bool forceLayoutOnRestoreFromBackForwardCache() WEBCORE_EMPTY_CLIENT_FINAL;
#endif

    void NODELETE forceLayoutForNonHTML() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE setCopiesOnScroll() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE detachedFromParent2() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE detachedFromParent3() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE convertMainResourceLoadToDownload(DocumentLoader*, const ResourceRequest&, const ResourceResponse&) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE assignIdentifierToInitialRequest(ResourceLoaderIdentifier, DocumentLoader*, const ResourceRequest&) WEBCORE_EMPTY_CLIENT_FINAL;
    bool NODELETE shouldUseCredentialStorage(DocumentLoader*, ResourceLoaderIdentifier) override;
    void NODELETE dispatchWillSendRequest(DocumentLoader*, ResourceLoaderIdentifier, ResourceRequest&, const ResourceResponse&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, ResourceLoaderIdentifier, const AuthenticationChallenge&) WEBCORE_EMPTY_CLIENT_FINAL;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    bool NODELETE canAuthenticateAgainstProtectionSpace(DocumentLoader*, ResourceLoaderIdentifier, const ProtectionSpace&) WEBCORE_EMPTY_CLIENT_FINAL;
#endif

#if PLATFORM(IOS_FAMILY)
    RetainPtr<CFDictionaryRef> connectionProperties(DocumentLoader*, ResourceLoaderIdentifier) WEBCORE_EMPTY_CLIENT_FINAL;
#endif

    void NODELETE dispatchDidReceiveResponse(DocumentLoader*, ResourceLoaderIdentifier, const ResourceResponse&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidReceiveContentLength(DocumentLoader*, ResourceLoaderIdentifier, int) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidFinishLoading(DocumentLoader*, ResourceLoaderIdentifier) WEBCORE_EMPTY_CLIENT_FINAL;
#if ENABLE(DATA_DETECTION)
    void NODELETE dispatchDidFinishDataDetection(NSArray *) WEBCORE_EMPTY_CLIENT_FINAL;
#endif
    void NODELETE dispatchDidFailLoading(DocumentLoader*, ResourceLoaderIdentifier, const ResourceError&) WEBCORE_EMPTY_CLIENT_FINAL;
    bool NODELETE dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE dispatchDidDispatchOnloadEvents() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidReceiveServerRedirectForProvisionalLoad() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidCancelClientRedirect() WEBCORE_EMPTY_CLIENT_FINAL;
    void dispatchWillPerformClientRedirect(const URL&, double, WallTime, LockBackForwardList) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidChangeLocationWithinPage() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidPushStateWithinPage() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidReplaceStateWithinPage() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidPopStateWithinPage() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchWillClose() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidStartProvisionalLoad() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidReceiveTitle(const StringWithDirection&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidCommitLoad(std::optional<HasInsecureContent>, std::optional<UsedLegacyTLS>, std::optional<WasPrivateRelayed>) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidFailProvisionalLoad(const ResourceError&, WillContinueLoading, WillInternallyHandleFailure) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidFailLoad(const ResourceError&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidFinishDocumentLoad() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidFinishLoad() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidReachLayoutMilestone(OptionSet<LayoutMilestone>) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidReachVisuallyNonEmptyState() WEBCORE_EMPTY_CLIENT_FINAL;

    LocalFrame* NODELETE dispatchCreatePage(const NavigationAction&, NewFrameOpenerPolicy, const String&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchShow() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE dispatchDecidePolicyForResponse(const ResourceResponse&, const ResourceRequest&, const String&, FramePolicyFunction&&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDecidePolicyForNewWindowAction(const NavigationAction&, const ResourceRequest&, FormState*, const String&, std::optional<HitTestResult>&&, FramePolicyFunction&&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDecidePolicyForNavigationAction(const NavigationAction&, const ResourceRequest&, const ResourceResponse& redirectResponse, FormState*, const String&, std::optional<NavigationIdentifier>, std::optional<HitTestResult>&&, bool, NavigationUpgradeToHTTPSBehavior, SandboxFlags, PolicyDecisionMode, FramePolicyFunction&&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE updateSandboxFlags(SandboxFlags) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE updateOpener(std::optional<FrameIdentifier>) WEBCORE_EMPTY_CLIENT_FINAL;
    void setPrinting(bool, FloatSize, FloatSize, float, AdjustViewSize) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE cancelPolicyCheck() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE dispatchUnableToImplementPolicy(const ResourceError&) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE dispatchWillSendSubmitEvent(Ref<FormState>&&) WEBCORE_EMPTY_CLIENT_FINAL;
    void dispatchWillSubmitForm(FormState&, URL&& requestURL, String&& method, CompletionHandler<void()>&&) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE revertToProvisionalState(DocumentLoader*) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE setMainDocumentError(DocumentLoader*, const ResourceError&) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE setMainFrameDocumentReady(bool) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE startDownload(const ResourceRequest&, const String&, FromDownloadAttribute = FromDownloadAttribute::No) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE willChangeTitle(DocumentLoader*) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE didChangeTitle(DocumentLoader*) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE willReplaceMultipartContent() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE didReplaceMultipartContent() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE committedLoad(DocumentLoader*, const SharedBuffer&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE finishedLoading(DocumentLoader*) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE loadStorageAccessQuirksIfNeeded() WEBCORE_EMPTY_CLIENT_FINAL;

    bool NODELETE shouldFallBack(const ResourceError&) const WEBCORE_EMPTY_CLIENT_FINAL;

    bool NODELETE canHandleRequest(const ResourceRequest&) const WEBCORE_EMPTY_CLIENT_FINAL;
    bool NODELETE canShowMIMEType(const String&) const WEBCORE_EMPTY_CLIENT_FINAL;
    bool NODELETE canShowMIMETypeAsHTML(const String&) const WEBCORE_EMPTY_CLIENT_FINAL;
    bool NODELETE representationExistsForURLScheme(StringView) const WEBCORE_EMPTY_CLIENT_FINAL;
    String NODELETE generatedMIMETypeForURLScheme(StringView) const WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE frameLoadCompleted() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE restoreViewState() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE provisionalLoadStarted() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE didFinishLoad() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE prepareForDataSourceReplacement() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE updateCachedDocumentLoader(DocumentLoader&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE setTitle(const StringWithDirection&, const URL&) WEBCORE_EMPTY_CLIENT_FINAL;

    String NODELETE userAgent(const URL&) const override;

    void NODELETE savePlatformDataToCachedFrame(CachedFrame*) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE transitionToCommittedFromCachedFrame(CachedFrame*) WEBCORE_EMPTY_CLIENT_FINAL;
#if PLATFORM(IOS_FAMILY)
    void didRestoreFrameHierarchyForCachedFrame() WEBCORE_EMPTY_CLIENT_FINAL;
#endif
    void NODELETE transitionToCommittedForNewPage(InitializingIframe) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE didRestoreFromBackForwardCache() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE updateGlobalHistory() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE updateGlobalHistoryRedirectLinks() WEBCORE_EMPTY_CLIENT_FINAL;
    ShouldGoToHistoryItem NODELETE shouldGoToHistoryItem(HistoryItem&, IsSameDocumentNavigation) const WEBCORE_EMPTY_CLIENT_FINAL;
    bool NODELETE supportsAsyncShouldGoToHistoryItem() const WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE shouldGoToHistoryItemAsync(HistoryItem&, CompletionHandler<void(ShouldGoToHistoryItem)>&&) const WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchGoToBackForwardItemAtIndex(int steps) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchEnqueueHistoryTraversalDelta(int delta) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE saveViewStateToItem(HistoryItem&) WEBCORE_EMPTY_CLIENT_FINAL;
    bool NODELETE canCachePage() const WEBCORE_EMPTY_CLIENT_FINAL;
    RefPtr<LocalFrame> createFrame(const AtomString&, HTMLFrameOwnerElement&) WEBCORE_EMPTY_CLIENT_FINAL;
    RefPtr<Widget> createPlugin(HTMLPlugInElement&, const URL&, const Vector<AtomString>&, const Vector<AtomString>&, const String&, bool) WEBCORE_EMPTY_CLIENT_FINAL;

    ObjectContentType NODELETE objectContentType(const URL&, const String&) WEBCORE_EMPTY_CLIENT_FINAL;
    AtomString NODELETE overrideMediaType() const WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE redirectDataToPlugin(Widget&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE dispatchDidClearWindowObjectInWorld(DOMWrapperWorld&) WEBCORE_EMPTY_CLIENT_FINAL;

#if PLATFORM(COCOA)
    RemoteAXObjectRef NODELETE accessibilityRemoteObject() WEBCORE_EMPTY_CLIENT_FINAL;
    IntPoint NODELETE accessibilityRemoteFrameOffset() WEBCORE_EMPTY_CLIENT_FINAL;
#if ENABLE(ACCESSIBILITY_ISOLATED_TREE)
    void NODELETE setIsolatedTree(Ref<WebCore::AXIsolatedTree>&&) WEBCORE_EMPTY_CLIENT_FINAL;
    RefPtr<WebCore::AXIsolatedTree> isolatedTree() const WEBCORE_EMPTY_CLIENT_FINAL;
#endif
    void willCacheResponse(DocumentLoader*, ResourceLoaderIdentifier, NSCachedURLResponse *, CompletionHandler<void(NSCachedURLResponse *)>&&) const WEBCORE_EMPTY_CLIENT_FINAL;
#endif

    Ref<FrameNetworkingContext> createNetworkingContext() WEBCORE_EMPTY_CLIENT_FINAL;

    bool NODELETE isEmptyFrameLoaderClient() const override;
    void NODELETE prefetchDNS(const String&) WEBCORE_EMPTY_CLIENT_FINAL;
    void sendH2Ping(const URL&, CompletionHandler<void(Expected<Seconds, ResourceError>&&)>&&) WEBCORE_EMPTY_CLIENT_FINAL;

#if USE(QUICK_LOOK)
    RefPtr<LegacyPreviewLoaderClient> createPreviewLoaderClient(const String&, const String&) WEBCORE_EMPTY_CLIENT_FINAL;
#endif

    bool NODELETE hasFrameSpecificStorageAccess() WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE revokeFrameSpecificStorageAccess() WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE dispatchLoadEventToOwnerElementInAnotherProcess() WEBCORE_EMPTY_CLIENT_FINAL;

    RefPtr<HistoryItem> createHistoryItemTree(bool, BackForwardItemIdentifier) const WEBCORE_EMPTY_CLIENT_FINAL;
};

} // namespace WebCore
