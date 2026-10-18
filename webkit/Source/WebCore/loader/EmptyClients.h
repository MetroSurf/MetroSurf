/*
 * Copyright (C) 2006 Eric Seidel (eric@webkit.org)
 * Copyright (C) 2008-2025 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#include <WebCore/ChromeClient.h>
#include <WebCore/CryptoClient.h>
#include <WebCore/ExceptionOr.h>
#include <WebCore/FocusOptions.h>
#include <WebCore/PageIdentifier.h>
#include <wtf/CompletionHandler.h>
#include <wtf/Platform.h>
#include <wtf/TZoneMalloc.h>
#include <wtf/UniqueRef.h>

// Empty client classes for use by WebCore.
//
// First created for SVGImage as it had no way to access the current Page (nor should it, since Images are not tied to a page).
// See http://bugs.webkit.org/show_bug.cgi?id=5971 for the original discussion about this file.

namespace PAL {
class SessionID;
}

namespace WebCore {

class DiagnosticLoggingClient;
class EditorClient;
class HTMLImageElement;
class PageConfiguration;
enum class BroadcastFocusedElement : bool;
enum class ContentChange : uint8_t;
struct FocusOptions;

class EmptyChromeClient : public ChromeClient {
    WTF_MAKE_TZONE_ALLOCATED(EmptyChromeClient);

    void chromeDestroyed() override { }

    void setWindowRect(const FloatRect&) WEBCORE_EMPTY_CLIENT_FINAL { }
    FloatRect windowRect() const WEBCORE_EMPTY_CLIENT_FINAL { return FloatRect(); }

    FloatRect pageRect() const WEBCORE_EMPTY_CLIENT_FINAL { return FloatRect(); }

    void focus() WEBCORE_EMPTY_CLIENT_FINAL { }
    void unfocus() WEBCORE_EMPTY_CLIENT_FINAL { }

    bool canTakeFocus(FocusDirection) const WEBCORE_EMPTY_CLIENT_FINAL { return false; }
    void takeFocus(FocusDirection) WEBCORE_EMPTY_CLIENT_FINAL { }

    void focusedElementChanged(Element*, LocalFrame*, FocusOptions, BroadcastFocusedElement) WEBCORE_EMPTY_CLIENT_FINAL { }
    void focusedFrameChanged(Frame*) WEBCORE_EMPTY_CLIENT_FINAL { }

    RefPtr<Page> createWindow(LocalFrame&, const String&, const WindowFeatures&, const NavigationAction&) WEBCORE_EMPTY_CLIENT_FINAL;
    void show() WEBCORE_EMPTY_CLIENT_FINAL { }

    bool canRunModal() const WEBCORE_EMPTY_CLIENT_FINAL { return false; }
    void runModal() WEBCORE_EMPTY_CLIENT_FINAL { }

    bool isPopup() const WEBCORE_EMPTY_CLIENT_FINAL { return false; }

    void setResizable(bool) WEBCORE_EMPTY_CLIENT_FINAL { }

    void addMessageToConsole(JSC::MessageSource, JSC::MessageLevel, const String&, unsigned, unsigned, const String&) WEBCORE_EMPTY_CLIENT_FINAL { }

    bool canRunBeforeUnloadConfirmPanel() WEBCORE_EMPTY_CLIENT_FINAL { return false; }
    bool runBeforeUnloadConfirmPanel(String&&, LocalFrame&) WEBCORE_EMPTY_CLIENT_FINAL { return true; }

    void closeWindow() WEBCORE_EMPTY_CLIENT_FINAL { }

    void rootFrameAdded(const LocalFrame&) WEBCORE_EMPTY_CLIENT_FINAL { }
    void rootFrameRemoved(const LocalFrame&) WEBCORE_EMPTY_CLIENT_FINAL { }

    void runJavaScriptAlert(LocalFrame&, const String&) WEBCORE_EMPTY_CLIENT_FINAL { }
    bool runJavaScriptConfirm(LocalFrame&, const String&) WEBCORE_EMPTY_CLIENT_FINAL { return false; }
    bool runJavaScriptPrompt(LocalFrame&, const String&, const String&, String&) WEBCORE_EMPTY_CLIENT_FINAL { return false; }

    RefPtr<PopupMenu> createPopupMenu(PopupMenuClient&) const WEBCORE_EMPTY_CLIENT_FINAL;
    RefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient&) const WEBCORE_EMPTY_CLIENT_FINAL;

    KeyboardUIMode keyboardUIMode() WEBCORE_EMPTY_CLIENT_FINAL { return KeyboardAccessDefault; }

    bool hasAccessoryMousePointingDevice() const WEBCORE_EMPTY_CLIENT_FINAL { return false; }
    bool hoverSupportedByPrimaryPointingDevice() const WEBCORE_EMPTY_CLIENT_FINAL { return false; };
    bool hoverSupportedByAnyAvailablePointingDevice() const WEBCORE_EMPTY_CLIENT_FINAL { return false; }
    std::optional<PointerCharacteristics> pointerCharacteristicsOfPrimaryPointingDevice() const WEBCORE_EMPTY_CLIENT_FINAL { return std::nullopt; };
    OptionSet<PointerCharacteristics> pointerCharacteristicsOfAllAvailablePointingDevices() const WEBCORE_EMPTY_CLIENT_FINAL { return { }; }

    void invalidateRootView(const IntRect&) WEBCORE_EMPTY_CLIENT_FINAL { }
    void invalidateContentsAndRootView(const IntRect&) override { }
    void invalidateContentsForSlowScroll(const IntRect&) WEBCORE_EMPTY_CLIENT_FINAL { }
    void scroll(const IntSize&, const IntRect&, const IntRect&) WEBCORE_EMPTY_CLIENT_FINAL { }

    IntPoint screenToRootView(const IntPoint& p) const WEBCORE_EMPTY_CLIENT_FINAL { return p; }
    IntPoint rootViewToScreen(const IntPoint& p) const WEBCORE_EMPTY_CLIENT_FINAL { return p; }
    IntRect rootViewToScreen(const IntRect& r) const WEBCORE_EMPTY_CLIENT_FINAL { return r; }
    IntPoint accessibilityScreenToRootView(const IntPoint& p) const WEBCORE_EMPTY_CLIENT_FINAL { return p; };
    IntRect rootViewToAccessibilityScreen(const IntRect& r) const WEBCORE_EMPTY_CLIENT_FINAL { return r; };
#if PLATFORM(IOS_FAMILY)
    void relayAccessibilityNotification(String&&, RetainPtr<NSData>&&) const WEBCORE_EMPTY_CLIENT_FINAL { };
    void relayAriaNotifyNotification(AriaNotifyData&&) const WEBCORE_EMPTY_CLIENT_FINAL { };
    void relayLiveRegionNotification(LiveRegionAnnouncementData&&) const WEBCORE_EMPTY_CLIENT_FINAL { };
#endif

    void didFinishLoadingImageForElement(HTMLImageElement&) WEBCORE_EMPTY_CLIENT_FINAL { }

    PlatformPageClient platformPageClient() const WEBCORE_EMPTY_CLIENT_FINAL { return 0; }
    void contentsSizeChanged(LocalFrame&, const IntSize&) const WEBCORE_EMPTY_CLIENT_FINAL { }
    void intrinsicContentsSizeChanged(const IntSize&) const WEBCORE_EMPTY_CLIENT_FINAL { }

    void mouseDidMoveOverElement(const HitTestResult&, OptionSet<PlatformEventModifier>, const String&, TextDirection) WEBCORE_EMPTY_CLIENT_FINAL { }

    void print(LocalFrame&, const StringWithDirection&) WEBCORE_EMPTY_CLIENT_FINAL { }

    void exceededDatabaseQuota(LocalFrame&, const String&, DatabaseDetails) WEBCORE_EMPTY_CLIENT_FINAL { }

    RefPtr<ColorChooser> createColorChooser(ColorChooserClient&, const Color&) WEBCORE_EMPTY_CLIENT_FINAL;

    RefPtr<DataListSuggestionPicker> createDataListSuggestionPicker(DataListSuggestionsClient&) WEBCORE_EMPTY_CLIENT_FINAL;
    bool canShowDataListSuggestionLabels() const WEBCORE_EMPTY_CLIENT_FINAL { return false; }

    RefPtr<DateTimeChooser> createDateTimeChooser(DateTimeChooserClient&) WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE setTextIndicator(RefPtr<TextIndicator>&&) const WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE updateTextIndicator(RefPtr<TextIndicator>&&) const WEBCORE_EMPTY_CLIENT_FINAL;

    DisplayRefreshMonitorFactory* NODELETE displayRefreshMonitorFactory() const WEBCORE_EMPTY_CLIENT_FINAL;

    void NODELETE runOpenPanel(LocalFrame&, FileChooser&) WEBCORE_EMPTY_CLIENT_FINAL;
    void NODELETE showShareSheet(ShareDataWithParsedURL&&, CompletionHandler<void(bool)>&&) WEBCORE_EMPTY_CLIENT_FINAL;
    void loadIconForFiles(const Vector<String>&, FileIconLoader&) WEBCORE_EMPTY_CLIENT_FINAL { }

    void elementDidFocus(Element&, const FocusOptions&) WEBCORE_EMPTY_CLIENT_FINAL { }
    void elementDidBlur(Element&) WEBCORE_EMPTY_CLIENT_FINAL { }

    void setCursor(const Cursor&) WEBCORE_EMPTY_CLIENT_FINAL { }
    void setCursorHiddenUntilMouseMoves(bool) WEBCORE_EMPTY_CLIENT_FINAL { }

    void scrollContainingScrollViewsToRevealRect(const IntRect&) const WEBCORE_EMPTY_CLIENT_FINAL { }
    void scrollMainFrameToRevealRect(const IntRect&) const WEBCORE_EMPTY_CLIENT_FINAL { }

    void attachRootGraphicsLayer(LocalFrame&, GraphicsLayer*) WEBCORE_EMPTY_CLIENT_FINAL { }
    void attachViewOverlayGraphicsLayer(GraphicsLayer*) WEBCORE_EMPTY_CLIENT_FINAL { }
    void setNeedsOneShotDrawingSynchronization() WEBCORE_EMPTY_CLIENT_FINAL { }
    void triggerRenderingUpdate() WEBCORE_EMPTY_CLIENT_FINAL { }

#if PLATFORM(WIN)
    void AXStartFrameLoad() WEBCORE_EMPTY_CLIENT_FINAL { }
    void AXFinishFrameLoad() WEBCORE_EMPTY_CLIENT_FINAL { }
#endif

#if PLATFORM(PLAYSTATION) || PLATFORM(HAIKU)
    void postAccessibilityNotification(AccessibilityObject&, AXNotification) WEBCORE_EMPTY_CLIENT_FINAL { }
    void postAccessibilityNodeTextChangeNotification(AccessibilityObject*, AXTextChange, unsigned, const String&) WEBCORE_EMPTY_CLIENT_FINAL { }
    void postAccessibilityFrameLoadingEventNotification(AccessibilityObject*, AXLoadingEvent) WEBCORE_EMPTY_CLIENT_FINAL { }
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
    void didPreventDefaultForEvent() WEBCORE_EMPTY_CLIENT_FINAL { }
#endif

#if PLATFORM(IOS_FAMILY)
    void didReceiveMobileDocType(bool) WEBCORE_EMPTY_CLIENT_FINAL { }
    void setNeedsScrollNotifications(LocalFrame&, bool) WEBCORE_EMPTY_CLIENT_FINAL { }
    void notifyRevealedSelectionByScrollingFrame(LocalFrame&) WEBCORE_EMPTY_CLIENT_FINAL { }
    void didLayout(LayoutType) WEBCORE_EMPTY_CLIENT_FINAL { }
    void didStartOverflowScroll() WEBCORE_EMPTY_CLIENT_FINAL { }
    void didEndOverflowScroll() WEBCORE_EMPTY_CLIENT_FINAL { }

    void suppressFormNotifications() WEBCORE_EMPTY_CLIENT_FINAL { }
    void restoreFormNotifications() WEBCORE_EMPTY_CLIENT_FINAL { }

    void addOrUpdateScrollingLayer(Node*, PlatformLayer*, PlatformLayer*, const IntSize&, bool, bool) WEBCORE_EMPTY_CLIENT_FINAL { }
    void removeScrollingLayer(Node*, PlatformLayer*, PlatformLayer*) WEBCORE_EMPTY_CLIENT_FINAL { }

    void webAppOrientationsUpdated() WEBCORE_EMPTY_CLIENT_FINAL { }
    void showPlaybackTargetPicker(bool, RouteSharingPolicy, const String&) WEBCORE_EMPTY_CLIENT_FINAL { }

    bool showDataDetectorsUIForElement(const Element&, const Event&) WEBCORE_EMPTY_CLIENT_FINAL { return false; }
#endif // PLATFORM(IOS_FAMILY)

#if ENABLE(CONTENT_CHANGE_OBSERVER)
    void didFinishContentChangeObserving(LocalFrame&, ContentChange) WEBCORE_EMPTY_CLIENT_FINAL { }
#endif

#if ENABLE(ORIENTATION_EVENTS)
    IntDegrees deviceOrientation() const WEBCORE_EMPTY_CLIENT_FINAL { return 0; }
#endif

#if PLATFORM(IOS_FAMILY)
    bool isStopping() WEBCORE_EMPTY_CLIENT_FINAL { return false; }
#endif

    void wheelEventHandlersChanged(bool) WEBCORE_EMPTY_CLIENT_FINAL { }
    
    bool isEmptyChromeClient() const WEBCORE_EMPTY_CLIENT_FINAL { return true; }

    void didAssociateFormControls(const Vector<Ref<Element>>&, LocalFrame&) WEBCORE_EMPTY_CLIENT_FINAL { }
    bool shouldNotifyOnFormChanges() WEBCORE_EMPTY_CLIENT_FINAL { return false; }

    RefPtr<Icon> createIconForFiles(const Vector<String>& ) WEBCORE_EMPTY_CLIENT_FINAL;
};

DiagnosticLoggingClient& NODELETE emptyDiagnosticLoggingClient();
WEBCORE_EXPORT PageConfiguration pageConfigurationWithEmptyClients(std::optional<PageIdentifier>, PAL::SessionID);

class EmptyCryptoClient: public CryptoClient {
    WTF_MAKE_TZONE_ALLOCATED(EmptyCryptoClient);
};

}
