/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollView.h"

#include "FrameView.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "IntPoint.h"
#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "NotImplemented.h"
#include "Frame.h"
#include "Page.h"
#include "GraphicsContext.h"
#include "Scrollbar.h"
#include "ScrollbarTheme.h"

#include <QDebug>
#include <QWidget>
#include <QPainter>
#include <QApplication>
#include <QPalette>
#include <QStyleOption>

#ifdef Q_WS_MAC
#include <Carbon/Carbon.h>
#endif

#include "qwebframe.h"
#include "qwebpage.h"

// #define DEBUG_SCROLLVIEW

namespace WebCore {

class ScrollView::ScrollViewPrivate : public ScrollbarClient {
public:
    ScrollViewPrivate(ScrollView* view)
      : m_view(view)
      , m_platformWidgets(0)
      , m_scrollbarsSuppressed(false)
      , m_inUpdateScrollbars(false)
      , m_scrollbarsAvoidingResizer(0)
      , m_vScrollbarMode(ScrollbarAuto)
      , m_hScrollbarMode(ScrollbarAuto)
    {
    }

    ~ScrollViewPrivate()
    {
        setHasHorizontalScrollbar(false);
        setHasVerticalScrollbar(false);
    }

    void setHasHorizontalScrollbar(bool hasBar);
    void setHasVerticalScrollbar(bool hasBar);

    virtual void valueChanged(Scrollbar*);
    virtual IntRect windowClipRect() const;
    virtual bool isActive() const;

    void scrollBackingStore(const IntSize& scrollDelta);

    ScrollView* m_view;
    int  m_platformWidgets;
    bool m_scrollbarsSuppressed;
    bool m_inUpdateScrollbars;
    int m_scrollbarsAvoidingResizer;
    ScrollbarMode m_vScrollbarMode;
    ScrollbarMode m_hScrollbarMode;
    RefPtr<Scrollbar> m_vBar;
    RefPtr<Scrollbar> m_hBar;
};

void ScrollView::ScrollViewPrivate::setHasHorizontalScrollbar(bool hasBar)
{
    if (hasBar && !m_hBar) {
        m_hBar = Scrollbar::createNativeScrollbar(this, HorizontalScrollbar, RegularScrollbar);
        m_view->addChild(m_hBar.get());
    } else if (!hasBar && m_hBar) {
        m_view->removeChild(m_hBar.get());
        m_hBar = 0;
    }
}

void ScrollView::ScrollViewPrivate::setHasVerticalScrollbar(bool hasBar)
{
    if (hasBar && !m_vBar) {
        m_vBar = Scrollbar::createNativeScrollbar(this, VerticalScrollbar, RegularScrollbar);
        m_view->addChild(m_vBar.get());
    } else if (!hasBar && m_vBar) {
        m_view->removeChild(m_vBar.get());
        m_vBar = 0;
    }
}

void ScrollView::ScrollViewPrivate::valueChanged(Scrollbar* bar)
{
    // Figure out if we really moved.
    IntSize newOffset = m_view->m_scrollOffset;
    if (bar) {
        if (bar == m_hBar)
            newOffset.setWidth(bar->value());
        else if (bar == m_vBar)
            newOffset.setHeight(bar->value());
    }
    IntSize scrollDelta = newOffset - m_view->m_scrollOffset;
    if (scrollDelta == IntSize())
        return;
    m_view->m_scrollOffset = newOffset;

    if (m_scrollbarsSuppressed)
        return;

    scrollBackingStore(scrollDelta);
    static_cast<FrameView*>(m_view)->frame()->sendScrollEvent();
}

void ScrollView::ScrollViewPrivate::scrollBackingStore(const IntSize& scrollDelta)
{
    // Since scrolling is double buffered, we will be blitting the scroll view's intersection
    // with the clip rect every time to keep it smooth.
    IntRect clipRect = m_view->windowClipRect();
    IntRect scrollViewRect = m_view->convertToContainingWindow(IntRect(0, 0, m_view->visibleWidth(), m_view->visibleHeight()));

    IntRect updateRect = clipRect;
    updateRect.intersect(scrollViewRect);

    if (m_view->canBlitOnScroll() && !m_view->root()->hasNativeWidgets()) {
       m_view->scrollBackingStore(-scrollDelta.width(), -scrollDelta.height(),
                                  scrollViewRect, clipRect);
    } else  {
       // We need to go ahead and repaint the entire backing store.
       m_view->addToDirtyRegion(updateRect);
       m_view->updateBackingStore();
    }

    m_view->geometryChanged();
}

IntRect ScrollView::ScrollViewPrivate::windowClipRect() const
{
    return static_cast<const FrameView*>(m_view)->windowClipRect(false);
}

bool ScrollView::ScrollViewPrivate::isActive() const
{
    Page* page = static_cast<const FrameView*>(m_view)->frame()->page();
    return page && page->focusController()->isActive();
}

ScrollView::ScrollView()
    : m_data(new ScrollViewPrivate(this))
{
    init();
}

ScrollView::~ScrollView()
{
    delete m_data;
}

Scrollbar* ScrollView::horizontalScrollbar() const
{
    return m_data->m_hBar.get();
}

Scrollbar* ScrollView::verticalScrollbar() const
{
    return m_data->m_vBar.get();
}

void ScrollView::updateContents(const IntRect& rect, bool now)
{
    if (rect.isEmpty())
        return;

    IntPoint windowPoint = contentsToWindow(rect.location());
    IntRect containingWindowRect = rect;
    containingWindowRect.setLocation(windowPoint);

    // Cache the dirty spot.
    addToDirtyRegion(containingWindowRect);

    if (now)
        updateBackingStore();
}

void ScrollView::update()
{
    QWidget* native = platformWidget();
    if (native) {
        native->update();
        return;
    }
    updateContents(IntRect(0, 0, width(), height()));
}

void ScrollView::setScrollPosition(const IntPoint& scrollPoint)
{
    IntPoint newScrollPosition = scrollPoint.shrunkTo(maximumScrollPosition());
    newScrollPosition.clampNegativeToZero();

    if (newScrollPosition == scrollPosition())
        return;

    updateScrollbars(IntSize(newScrollPosition.x(), newScrollPosition.y()));
}

void ScrollView::setFrameGeometry(const IntRect& newGeometry)
{
    IntRect oldGeometry = frameGeometry();
    Widget::setFrameGeometry(newGeometry);

    if (newGeometry == oldGeometry)
        return;

    if (newGeometry.width() != oldGeometry.width() || newGeometry.height() != oldGeometry.height()) {
        updateScrollbars(m_scrollOffset);
        static_cast<FrameView*>(this)->setNeedsLayout();
    }

    geometryChanged();
}

void ScrollView::geometryChanged() const
{
    HashSet<Widget*>::const_iterator end = m_children.end();
    for (HashSet<Widget*>::const_iterator current = m_children.begin(); current != end; ++current)
        (*current)->geometryChanged();

    const_cast<ScrollView *>(this)->invalidateScrollbars();
}

IntPoint ScrollView::windowToContents(const IntPoint& windowPoint) const
{
    IntPoint viewPoint = convertFromContainingWindow(windowPoint);
    return viewPoint + scrollOffset();
}

IntPoint ScrollView::contentsToWindow(const IntPoint& contentsPoint) const
{
    IntPoint viewPoint = contentsPoint - scrollOffset();
    return convertToContainingWindow(viewPoint);
}

bool ScrollView::isScrollViewScrollbar(const Widget* child) const
{
    return m_data->m_hBar == child || m_data->m_vBar == child;
}

WebCore::ScrollbarMode ScrollView::hScrollbarMode() const
{
    return m_data->m_hScrollbarMode;
}

WebCore::ScrollbarMode ScrollView::vScrollbarMode() const
{
    return m_data->m_vScrollbarMode;
}

bool ScrollView::isScrollable() 
{ 
    return true; // FIXME : return whether or not the view is scrollable
}

void ScrollView::suppressScrollbars(bool suppressed, bool repaintOnSuppress)
{
    m_data->m_scrollbarsSuppressed = suppressed;
    if (repaintOnSuppress && !suppressed)
        invalidateScrollbars();
}

void ScrollView::invalidateScrollbars()
{
    if (m_data->m_hBar)
        m_data->m_hBar->invalidate();
    if (m_data->m_vBar)
        m_data->m_vBar->invalidate();


    // Invalidate the scroll corner too
    IntRect hCorner;
    if (m_data->m_hBar && width() - m_data->m_hBar->width() > 0) {
        hCorner = IntRect(m_data->m_hBar->width(),
                         height() - m_data->m_hBar->height(),
                         width() - m_data->m_hBar->width(),
                         m_data->m_hBar->height());
       addToDirtyRegion(convertToContainingWindow(hCorner));
    }

    if (m_data->m_vBar && height() - m_data->m_vBar->height() > 0) {
        IntRect vCorner(width() - m_data->m_vBar->width(),
                       m_data->m_vBar->height(),
                       m_data->m_vBar->width(),
                       height() - m_data->m_vBar->height());
        if (vCorner != hCorner)
            addToDirtyRegion(convertToContainingWindow(vCorner));
    }
}

void ScrollView::setHScrollbarMode(ScrollbarMode newMode)
{
    if (m_data->m_hScrollbarMode != newMode) {
        m_data->m_hScrollbarMode = newMode;
        updateScrollbars(m_scrollOffset);
    }
}

void ScrollView::setVScrollbarMode(ScrollbarMode newMode)
{
    if (m_data->m_vScrollbarMode != newMode) {
        m_data->m_vScrollbarMode = newMode;
        updateScrollbars(m_scrollOffset);
    }
}

void ScrollView::setScrollbarsMode(ScrollbarMode newMode)
{
    if (m_data->m_hScrollbarMode != newMode ||
        m_data->m_vScrollbarMode != newMode) {
        m_data->m_hScrollbarMode = m_data->m_vScrollbarMode = newMode;
        updateScrollbars(m_scrollOffset);
    }
}

bool ScrollView::inWindow() const
{
    return true;
}

void ScrollView::updateScrollbars(const IntSize& desiredOffset)
{
    // Don't allow re-entrancy into this function.
    if (m_data->m_inUpdateScrollbars)
        return;

    // FIXME: This code is here so we don't have to fork FrameView.h/.cpp.
    // In the end, FrameView should just merge with ScrollView.
    if (static_cast<const FrameView*>(this)->frame()->prohibitsScrolling())
        return;
    
    m_data->m_inUpdateScrollbars = true;

    bool hasVerticalScrollbar = m_data->m_vBar;
    bool hasHorizontalScrollbar = m_data->m_hBar;
    bool oldHasVertical = hasVerticalScrollbar;
    bool oldHasHorizontal = hasHorizontalScrollbar;
    ScrollbarMode hScroll = m_data->m_hScrollbarMode;
    ScrollbarMode vScroll = m_data->m_vScrollbarMode;

    const int cVerticalWidth = ScrollbarTheme::nativeTheme()->scrollbarThickness();
    const int cHorizontalHeight = ScrollbarTheme::nativeTheme()->scrollbarThickness();

    for (int pass = 0; pass < 2; pass++) {
        bool scrollsVertically;
        bool scrollsHorizontally;

        if (!m_data->m_scrollbarsSuppressed && (hScroll == ScrollbarAuto || vScroll == ScrollbarAuto)) {
            // Do a layout if pending before checking if scrollbars are needed.
            if (hasVerticalScrollbar != oldHasVertical || hasHorizontalScrollbar != oldHasHorizontal)
                static_cast<FrameView*>(this)->layout();
             
            scrollsVertically = (vScroll == ScrollbarAlwaysOn) || (vScroll == ScrollbarAuto && contentsHeight() > height());
            if (scrollsVertically)
                scrollsHorizontally = (hScroll == ScrollbarAlwaysOn) || (hScroll == ScrollbarAuto && contentsWidth() + cVerticalWidth > width());
            else {
                scrollsHorizontally = (hScroll == ScrollbarAlwaysOn) || (hScroll == ScrollbarAuto && contentsWidth() > width());
                if (scrollsHorizontally)
                    scrollsVertically = (vScroll == ScrollbarAlwaysOn) || (vScroll == ScrollbarAuto && contentsHeight() + cHorizontalHeight > height());
            }
        }
        else {
            scrollsHorizontally = (hScroll == ScrollbarAuto) ? hasHorizontalScrollbar : (hScroll == ScrollbarAlwaysOn);
            scrollsVertically = (vScroll == ScrollbarAuto) ? hasVerticalScrollbar : (vScroll == ScrollbarAlwaysOn);
        }
        
        if (hasVerticalScrollbar != scrollsVertically) {
            m_data->setHasVerticalScrollbar(scrollsVertically);
            hasVerticalScrollbar = scrollsVertically;
        }

        if (hasHorizontalScrollbar != scrollsHorizontally) {
            m_data->setHasHorizontalScrollbar(scrollsHorizontally);
            hasHorizontalScrollbar = scrollsHorizontally;
        }
    }
    
    // Set up the range (and page step/line step).
    IntSize maxScrollPosition(contentsWidth() - visibleWidth(), contentsHeight() - visibleHeight());
    IntSize scroll = desiredOffset.shrunkTo(maxScrollPosition);
    scroll.clampNegativeToZero();

    QPoint scrollbarOffset;
#if defined(Q_WS_MAC) && !defined(QT_MAC_USE_COCOA)
    // On Mac, offset the scrollbars so they don't cover the grow box. Check if the window 
    // has a grow box, and then check if the bottom-right corner of the scroll view 
    // intersercts it. The calculations are done in global coordinates.
    QWidget* contentWidget = containingWindow();
    if (contentWidget) {
        QWidget* windowWidget = contentWidget->window();
        if (windowWidget) {
            HIViewRef growBox = 0;
            HIViewFindByID(HIViewGetRoot(HIViewGetWindow(HIViewRef(contentWidget->winId()))), kHIViewWindowGrowBoxID, &growBox);
            const QPoint contentBr = contentWidget->mapToGlobal(QPoint(0,0)) + contentWidget->size();
            const QPoint windowBr = windowWidget->mapToGlobal(QPoint(0,0)) + windowWidget->size();
            const QPoint contentOffset = (windowBr - contentBr);
            const int growBoxSize = 15;
            const bool enableOffset = (growBox != 0 && contentOffset.x() >= 0 && contentOffset. y() >= 0);
            scrollbarOffset = enableOffset ? QPoint(growBoxSize - qMin(contentOffset.x(), growBoxSize),
                                                    growBoxSize - qMin(contentOffset.y(), growBoxSize))
                                           : QPoint(0,0);
        }
    }
#endif

    if (m_data->m_hBar) {
        int clientWidth = visibleWidth();
        m_data->m_hBar->setEnabled(contentsWidth() > clientWidth);
        int pageStep = (clientWidth - PAGE_KEEP);
        if (pageStep < 0) pageStep = clientWidth;
        IntRect oldRect(m_data->m_hBar->frameGeometry());
        IntRect hBarRect = IntRect(0,
                                   height() - m_data->m_hBar->height(),
                                   width() - (m_data->m_vBar ? m_data->m_vBar->width() : scrollbarOffset.x()),
                                   m_data->m_hBar->height());
        m_data->m_hBar->setFrameGeometry(hBarRect);
        if (!m_data->m_scrollbarsSuppressed && oldRect != m_data->m_hBar->frameGeometry())
            m_data->m_hBar->invalidate();

        if (m_data->m_scrollbarsSuppressed)
            m_data->m_hBar->setSuppressInvalidation(true);
        m_data->m_hBar->setSteps(LINE_STEP, pageStep);
        m_data->m_hBar->setProportion(clientWidth, contentsWidth());
        m_data->m_hBar->setValue(scroll.width());
        if (m_data->m_scrollbarsSuppressed)
            m_data->m_hBar->setSuppressInvalidation(false); 
    } 

    if (m_data->m_vBar) {
        int clientHeight = visibleHeight();
        m_data->m_vBar->setEnabled(contentsHeight() > clientHeight);
        int pageStep = (clientHeight - PAGE_KEEP);
        if (pageStep < 0) pageStep = clientHeight;
        IntRect oldRect(m_data->m_vBar->frameGeometry());
        IntRect vBarRect = IntRect(width() - m_data->m_vBar->width(), 
                                   0,
                                   m_data->m_vBar->width(),
                                   height() - (m_data->m_hBar ? m_data->m_hBar->height() : scrollbarOffset.y()));
        m_data->m_vBar->setFrameGeometry(vBarRect);
        if (!m_data->m_scrollbarsSuppressed && oldRect != m_data->m_vBar->frameGeometry())
            m_data->m_vBar->invalidate();

        if (m_data->m_scrollbarsSuppressed)
            m_data->m_vBar->setSuppressInvalidation(true);
        m_data->m_vBar->setSteps(LINE_STEP, pageStep);
        m_data->m_vBar->setProportion(clientHeight, contentsHeight());
        m_data->m_vBar->setValue(scroll.height());
        if (m_data->m_scrollbarsSuppressed)
            m_data->m_vBar->setSuppressInvalidation(false);
    }

    if (oldHasVertical != (m_data->m_vBar != 0) || oldHasHorizontal != (m_data->m_hBar != 0))
        geometryChanged();

    // See if our offset has changed in a situation where we might not have scrollbars.
    // This can happen when editing a body with overflow:hidden and scrolling to reveal selection.
    // It can also happen when maximizing a window that has scrollbars (but the new maximized result
    // does not).
    IntSize scrollDelta = scroll - m_scrollOffset;
    if (scrollDelta != IntSize()) {
       m_scrollOffset = scroll;
       m_data->scrollBackingStore(scrollDelta);
    }

    m_data->m_inUpdateScrollbars = false;
}

Scrollbar* ScrollView::scrollbarUnderMouse(const PlatformMouseEvent& mouseEvent)
{
    IntPoint viewPoint = convertFromContainingWindow(mouseEvent.pos());
    if (m_data->m_hBar && m_data->m_hBar->frameGeometry().contains(viewPoint))
        return m_data->m_hBar.get();
    if (m_data->m_vBar && m_data->m_vBar->frameGeometry().contains(viewPoint))
        return m_data->m_vBar.get();
    return 0;
}

void ScrollView::platformAddChild(Widget* child)
{
    root()->incrementNativeWidgetCount();
}

void ScrollView::platformRemoveChild(Widget* child)
{
    child->hide();
    root()->decrementNativeWidgetCount();
}

static void drawScrollbarCorner(GraphicsContext* context, const IntRect& rect)
{
#if QT_VERSION < 0x040500
    context->fillRect(rect, QApplication::palette().color(QPalette::Normal, QPalette::Window));
#else
    QStyleOption option;
    option.rect = rect;
    QApplication::style()->drawPrimitive(QStyle::PE_PanelScrollAreaCorner,
            &option, context->platformContext(), 0);
#endif
}


void ScrollView::paint(GraphicsContext* context, const IntRect& rect)
{
    // FIXME: This code is here so we don't have to fork FrameView.h/.cpp.
    // In the end, FrameView should just merge with ScrollView.
    ASSERT(isFrameView());

    if (context->paintingDisabled())
        return;

    IntRect documentDirtyRect = rect;

    context->save();

    context->translate(x(), y());
    documentDirtyRect.move(-x(), -y());

    context->translate(-scrollX(), -scrollY());
    documentDirtyRect.move(scrollX(), scrollY());

    documentDirtyRect.intersect(enclosingIntRect(visibleContentRect()));
    context->clip(documentDirtyRect);

    static_cast<const FrameView*>(this)->frame()->paint(context, documentDirtyRect);

    context->restore();

    // Now paint the scrollbars.
    if (!m_data->m_scrollbarsSuppressed && (m_data->m_hBar || m_data->m_vBar)) {
        context->save();
        IntRect scrollViewDirtyRect = rect;
        scrollViewDirtyRect.intersect(frameGeometry());
        context->translate(x(), y());
        scrollViewDirtyRect.move(-x(), -y());
        if (m_data->m_hBar)
            m_data->m_hBar->paint(context, scrollViewDirtyRect);
        if (m_data->m_vBar)
            m_data->m_vBar->paint(context, scrollViewDirtyRect);

        // Fill the scroll corners using the current Qt style
        IntRect hCorner;
        if (m_data->m_hBar && width() - m_data->m_hBar->width() > 0) {
            hCorner = IntRect(m_data->m_hBar->width(),
                              height() - m_data->m_hBar->height(),
                              width() - m_data->m_hBar->width(),
                              m_data->m_hBar->height());
            if (hCorner.intersects(scrollViewDirtyRect))
                drawScrollbarCorner(context, hCorner);
        }

        if (m_data->m_vBar && height() - m_data->m_vBar->height() > 0) {
            IntRect vCorner(width() - m_data->m_vBar->width(),
                            m_data->m_vBar->height(),
                            m_data->m_vBar->width(),
                            height() - m_data->m_vBar->height());
            if (vCorner != hCorner && vCorner.intersects(scrollViewDirtyRect))
                drawScrollbarCorner(context, vCorner);
        }

        context->restore();
    }
}

void ScrollView::wheelEvent(PlatformWheelEvent& e)
{
    float deltaX = e.deltaX();
    float deltaY = e.deltaY();

    PlatformMouseEvent mouseEvent(e.pos(), e.globalPos(), NoButton, MouseEventScroll,
            0, e.shiftKey(), e.ctrlKey(), e.altKey(), e.metaKey(), 0);
    Scrollbar* scrollBar = scrollbarUnderMouse(mouseEvent);

    if (scrollBar && scrollBar == verticalScrollbar()) {
        deltaY = (deltaY == 0 ? deltaX : deltaY);
        deltaX = 0;
    } else if (scrollBar && scrollBar == horizontalScrollbar()) {
        deltaX = (deltaX == 0 ? deltaY : deltaX);
        deltaY = 0;
    }

    // Determine how much we want to scroll.  If we can move at all, we will accept the event.
    IntSize maxScrollDelta = maximumScrollPosition() - scrollPosition();
    if ((deltaX < 0 && maxScrollDelta.width() > 0) ||
        (deltaX > 0 && scrollOffset().width() > 0) ||
        (deltaY < 0 && maxScrollDelta.height() > 0) ||
        (deltaY > 0 && scrollOffset().height() > 0)) {

        e.accept();
        scrollBy(IntSize(int(-deltaX * LINE_STEP), int(-deltaY * LINE_STEP)));
    }
}

bool ScrollView::scroll(ScrollDirection direction, ScrollGranularity granularity)
{
    if (direction == ScrollUp || direction == ScrollDown) {
        if (m_data->m_vBar)
            return m_data->m_vBar->scroll(direction, granularity);
    } else {
        if (m_data->m_hBar)
            return m_data->m_hBar->scroll(direction, granularity);
    }
    return false;
}

IntRect ScrollView::windowResizerRect()
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return IntRect();
    return page->chrome()->windowResizerRect();
}

bool ScrollView::resizerOverlapsContent() const
{
    return !m_data->m_scrollbarsAvoidingResizer;
}

void ScrollView::adjustOverlappingScrollbarCount(int overlapDelta)
{
    int oldCount = m_data->m_scrollbarsAvoidingResizer;
    m_data->m_scrollbarsAvoidingResizer += overlapDelta;
    if (parent() && parent()->isFrameView())
        static_cast<FrameView*>(parent())->adjustOverlappingScrollbarCount(overlapDelta);
    else if (!m_data->m_scrollbarsSuppressed) {
        // If we went from n to 0 or from 0 to n and we're the outermost view,
        // we need to invalidate the windowResizerRect(), since it will now need to paint
        // differently.
        if (oldCount > 0 && m_data->m_scrollbarsAvoidingResizer == 0 ||
            oldCount == 0 && m_data->m_scrollbarsAvoidingResizer > 0)
            invalidateRect(windowResizerRect());
    }
}

void ScrollView::setParent(ScrollView* parentView)
{
    if (!parentView && m_data->m_scrollbarsAvoidingResizer && parent() && parent()->isFrameView())
        static_cast<FrameView*>(parent())->adjustOverlappingScrollbarCount(false);
    Widget::setParent(parentView);
}

void ScrollView::addToDirtyRegion(const IntRect& containingWindowRect)
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return;
    page->chrome()->addToDirtyRegion(containingWindowRect);
}

void ScrollView::scrollBackingStore(int dx, int dy, const IntRect& scrollViewRect, const IntRect& clipRect)
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return;
    page->chrome()->scrollBackingStore(dx, dy, scrollViewRect, clipRect);
}

void ScrollView::updateBackingStore()
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return;
    page->chrome()->updateBackingStore();
}

void ScrollView::incrementNativeWidgetCount()
{
    ++m_data->m_platformWidgets;
}

void ScrollView::decrementNativeWidgetCount()
{
    --m_data->m_platformWidgets;
}

bool ScrollView::hasNativeWidgets() const
{
    return m_data->m_platformWidgets != 0;
}

}

// vim: ts=4 sw=4 et
