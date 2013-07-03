/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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

#include "core/rendering/RenderLayerCompositor.h"

#include "CSSPropertyNames.h"
#include "HTMLNames.h"
#include "core/dom/NodeList.h"
#include "core/dom/WebCoreMemoryInstrumentation.h"
#include "core/html/HTMLCanvasElement.h"
#include "core/html/HTMLIFrameElement.h"
#include "core/html/canvas/CanvasRenderingContext.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/page/Chrome.h"
#include "core/page/ChromeClient.h"
#include "core/page/Frame.h"
#include "core/page/FrameView.h"
#include "core/page/Page.h"
#include "core/page/Settings.h"
#include "core/page/animation/AnimationController.h"
#include "core/page/scrolling/ScrollingConstraints.h"
#include "core/page/scrolling/ScrollingCoordinator.h"
#include "core/platform/HistogramSupport.h"
#include "core/platform/Logging.h"
#include "core/platform/ScrollbarTheme.h"
#include "core/platform/chromium/TraceEvent.h"
#include "core/platform/graphics/GraphicsLayer.h"
#include "core/platform/graphics/transforms/TransformState.h"
#include "core/rendering/HitTestResult.h"
#include "core/rendering/RenderApplet.h"
#include "core/rendering/RenderEmbeddedObject.h"
#include "core/rendering/RenderFullScreen.h"
#include "core/rendering/RenderGeometryMap.h"
#include "core/rendering/RenderIFrame.h"
#include "core/rendering/RenderLayerBacking.h"
#include "core/rendering/RenderReplica.h"
#include "core/rendering/RenderVideo.h"
#include "core/rendering/RenderView.h"
#include "wtf/MemoryInstrumentationHashMap.h"
#include "wtf/MemoryInstrumentationHashSet.h"
#include "wtf/TemporaryChange.h"

#if !LOG_DISABLED
#include "wtf/CurrentTime.h"
#endif

#ifndef NDEBUG
#include "core/rendering/RenderTreeAsText.h"
#endif

#define WTF_USE_COMPOSITING_FOR_SMALL_CANVASES 1

static const int canvasAreaThresholdRequiringCompositing = 50 * 100;

namespace WebCore {

using namespace HTMLNames;

class OverlapMapContainer {
public:
    void add(const IntRect& bounds)
    {
        m_layerRects.append(bounds);
        m_boundingBox.unite(bounds);
    }

    bool overlapsLayers(const IntRect& bounds) const
    {
        // Checking with the bounding box will quickly reject cases when
        // layers are created for lists of items going in one direction and
        // never overlap with each other.
        if (!bounds.intersects(m_boundingBox))
            return false;
        for (unsigned i = 0; i < m_layerRects.size(); i++) {
            if (m_layerRects[i].intersects(bounds))
                return true;
        }
        return false;
    }

    void unite(const OverlapMapContainer& otherContainer)
    {
        m_layerRects.append(otherContainer.m_layerRects);
        m_boundingBox.unite(otherContainer.m_boundingBox);
    }
private:
    Vector<IntRect> m_layerRects;
    IntRect m_boundingBox;
};

class RenderLayerCompositor::OverlapMap {
    WTF_MAKE_NONCOPYABLE(OverlapMap);
public:
    OverlapMap()
        : m_geometryMap(UseTransforms)
    {
        // Begin assuming the root layer will be composited so that there is
        // something on the stack. The root layer should also never get an
        // popCompositingContainer call.
        pushCompositingContainer();
    }

    void add(const RenderLayer* layer, const IntRect& bounds)
    {
        // Layers do not contribute to overlap immediately--instead, they will
        // contribute to overlap as soon as their composited ancestor has been
        // recursively processed and popped off the stack.
        ASSERT(m_overlapStack.size() >= 2);
        m_overlapStack[m_overlapStack.size() - 2].add(bounds);
        m_layers.add(layer);
    }

    bool contains(const RenderLayer* layer)
    {
        return m_layers.contains(layer);
    }

    bool overlapsLayers(const IntRect& bounds) const
    {
        return m_overlapStack.last().overlapsLayers(bounds);
    }

    bool isEmpty()
    {
        return m_layers.isEmpty();
    }

    void pushCompositingContainer()
    {
        m_overlapStack.append(OverlapMapContainer());
    }

    void popCompositingContainer()
    {
        m_overlapStack[m_overlapStack.size() - 2].unite(m_overlapStack.last());
        m_overlapStack.removeLast();
    }

    RenderGeometryMap& geometryMap() { return m_geometryMap; }

private:
    Vector<OverlapMapContainer> m_overlapStack;
    HashSet<const RenderLayer*> m_layers;
    RenderGeometryMap m_geometryMap;
};

struct CompositingState {
    CompositingState(RenderLayer* compAncestor, bool testOverlap)
        : m_compositingAncestor(compAncestor)
        , m_subtreeIsCompositing(false)
        , m_testingOverlap(testOverlap)
#ifndef NDEBUG
        , m_depth(0)
#endif
    {
    }
    
    CompositingState(const CompositingState& other)
        : m_compositingAncestor(other.m_compositingAncestor)
        , m_subtreeIsCompositing(other.m_subtreeIsCompositing)
        , m_testingOverlap(other.m_testingOverlap)
#ifndef NDEBUG
        , m_depth(other.m_depth + 1)
#endif
    {
    }
    
    RenderLayer* m_compositingAncestor;
    bool m_subtreeIsCompositing;
    bool m_testingOverlap;
#ifndef NDEBUG
    int m_depth;
#endif
};


static inline bool compositingLogEnabled()
{
#if !LOG_DISABLED
    return LogCompositing.state == WTFLogChannelOn;
#else
    return false;
#endif
}

RenderLayerCompositor::RenderLayerCompositor(RenderView* renderView)
    : m_renderView(renderView)
    , m_hasAcceleratedCompositing(true)
    , m_compositingTriggers(static_cast<ChromeClient::CompositingTriggerFlags>(ChromeClient::AllTriggers))
    , m_compositedLayerCount(0)
    , m_showDebugBorders(false)
    , m_showRepaintCounter(false)
    , m_reevaluateCompositingAfterLayout(false)
    , m_compositing(false)
    , m_compositingLayersNeedRebuild(false)
    , m_forceCompositingMode(false)
    , m_inPostLayoutUpdate(false)
    , m_needsUpdateCompositingRequirementsState(false)
    , m_isTrackingRepaints(false)
    , m_rootLayerAttachment(RootLayerUnattached)
#if !LOG_DISABLED
    , m_rootLayerUpdateCount(0)
    , m_obligateCompositedLayerCount(0)
    , m_secondaryCompositedLayerCount(0)
    , m_obligatoryBackingStoreBytes(0)
    , m_secondaryBackingStoreBytes(0)
#endif
{
}

RenderLayerCompositor::~RenderLayerCompositor()
{
    ASSERT(m_rootLayerAttachment == RootLayerUnattached);
}

void RenderLayerCompositor::enableCompositingMode(bool enable /* = true */)
{
    if (enable != m_compositing) {
        m_compositing = enable;
        
        if (m_compositing) {
            ensureRootLayer();
            notifyIFramesOfCompositingChange();
        } else
            destroyRootLayer();
    }
}

void RenderLayerCompositor::cacheAcceleratedCompositingFlags()
{
    bool hasAcceleratedCompositing = false;
    bool showDebugBorders = false;
    bool showRepaintCounter = false;
    bool forceCompositingMode = false;

    if (Settings* settings = m_renderView->document()->settings()) {
        hasAcceleratedCompositing = settings->acceleratedCompositingEnabled();

        // We allow the chrome to override the settings, in case the page is rendered
        // on a chrome that doesn't allow accelerated compositing.
        if (hasAcceleratedCompositing) {
            if (Page* page = this->page()) {
                ChromeClient* chromeClient = page->chrome().client();
                m_compositingTriggers = chromeClient->allowedCompositingTriggers();
                hasAcceleratedCompositing = m_compositingTriggers;
            }
        }

        showDebugBorders = settings->showDebugBorders();
        showRepaintCounter = settings->showRepaintCounter();
        forceCompositingMode = settings->forceCompositingMode() && hasAcceleratedCompositing;

        if (forceCompositingMode && !isMainFrame())
            forceCompositingMode = requiresCompositingForScrollableFrame();
    }

    if (hasAcceleratedCompositing != m_hasAcceleratedCompositing || showDebugBorders != m_showDebugBorders || showRepaintCounter != m_showRepaintCounter || forceCompositingMode != m_forceCompositingMode)
        setCompositingLayersNeedRebuild();

    bool debugBordersChanged = m_showDebugBorders != showDebugBorders;
    m_hasAcceleratedCompositing = hasAcceleratedCompositing;
    m_showDebugBorders = showDebugBorders;
    m_showRepaintCounter = showRepaintCounter;
    m_forceCompositingMode = forceCompositingMode;
    
    if (debugBordersChanged) {
        if (m_layerForHorizontalScrollbar)
            m_layerForHorizontalScrollbar->setShowDebugBorder(m_showDebugBorders);

        if (m_layerForVerticalScrollbar)
            m_layerForVerticalScrollbar->setShowDebugBorder(m_showDebugBorders);

        if (m_layerForScrollCorner)
            m_layerForScrollCorner->setShowDebugBorder(m_showDebugBorders);
    }
}

bool RenderLayerCompositor::canRender3DTransforms() const
{
    return hasAcceleratedCompositing() && (m_compositingTriggers & ChromeClient::ThreeDTransformTrigger);
}

void RenderLayerCompositor::setCompositingLayersNeedRebuild(bool needRebuild)
{
    if (inCompositingMode())
        m_compositingLayersNeedRebuild = needRebuild;
}

void RenderLayerCompositor::didChangeVisibleRect()
{
    GraphicsLayer* rootLayer = rootGraphicsLayer();
    if (!rootLayer)
        return;

    FrameView* frameView = m_renderView ? m_renderView->frameView() : 0;
    if (!frameView)
        return;

    IntRect visibleRect = m_clipLayer ? IntRect(IntPoint(), frameView->contentsSize()) : frameView->visibleContentRect();
    if (rootLayer->visibleRectChangeRequiresFlush(visibleRect)) {
        if (Page* page = this->page())
            page->chrome().client()->scheduleCompositingLayerFlush();
    }
}

bool RenderLayerCompositor::hasAnyAdditionalCompositedLayers(const RenderLayer* rootLayer) const
{
    return m_compositedLayerCount > (rootLayer->isComposited() ? 1 : 0);
}

void RenderLayerCompositor::updateCompositingRequirementsState()
{
    TRACE_EVENT0("blink_rendering", "RenderLayerCompositor::updateCompositingRequirementsState");

    if (!m_needsUpdateCompositingRequirementsState)
        return;

    m_needsUpdateCompositingRequirementsState = false;

    if (!rootRenderLayer() || !rootRenderLayer()->acceleratedCompositingForOverflowScrollEnabled())
        return;

    const FrameView::ScrollableAreaSet* scrollableAreas = m_renderView->frameView()->scrollableAreas();
    if (!scrollableAreas)
        return;

    for (HashSet<RenderLayer*>::iterator it = m_outOfFlowPositionedLayers.begin(); it != m_outOfFlowPositionedLayers.end(); ++it)
        (*it)->updateHasUnclippedDescendant();

    for (FrameView::ScrollableAreaSet::iterator it = scrollableAreas->begin(); it != scrollableAreas->end(); ++it)
        (*it)->updateNeedsCompositedScrolling();
}

void RenderLayerCompositor::updateCompositingLayers(CompositingUpdateType updateType, RenderLayer* updateRoot)
{
    // Avoid updating the layers with old values. Compositing layers will be updated after the layout is finished.
    if (m_renderView->needsLayout())
        return;

    if (m_forceCompositingMode && !m_compositing)
        enableCompositingMode(true);

    if (!m_reevaluateCompositingAfterLayout && !m_compositing)
        return;

    AnimationUpdateBlock animationUpdateBlock(m_renderView->frameView()->frame()->animation());

    TemporaryChange<bool> postLayoutChange(m_inPostLayoutUpdate, true);
    
    bool checkForHierarchyUpdate = m_reevaluateCompositingAfterLayout;
    bool needGeometryUpdate = false;

    switch (updateType) {
    case CompositingUpdateAfterStyleChange:
    case CompositingUpdateAfterLayout:
        checkForHierarchyUpdate = true;
        break;
    case CompositingUpdateOnScroll:
        checkForHierarchyUpdate = true; // Overlap can change with scrolling, so need to check for hierarchy updates.
        needGeometryUpdate = true;
        break;
    case CompositingUpdateOnCompositedScroll:
        needGeometryUpdate = true;
        break;
    }

    if (!checkForHierarchyUpdate && !needGeometryUpdate)
        return;

    bool needHierarchyUpdate = m_compositingLayersNeedRebuild;
    bool isFullUpdate = !updateRoot;

    // Only clear the flag if we're updating the entire hierarchy.
    m_compositingLayersNeedRebuild = false;
    updateRoot = rootRenderLayer();

    if (isFullUpdate && updateType == CompositingUpdateAfterLayout)
        m_reevaluateCompositingAfterLayout = false;

#if !LOG_DISABLED
    double startTime = 0;
    if (compositingLogEnabled()) {
        ++m_rootLayerUpdateCount;
        startTime = currentTime();
    }
#endif

    if (checkForHierarchyUpdate) {
        // Go through the layers in presentation order, so that we can compute which RenderLayers need compositing layers.
        // FIXME: we could maybe do this and the hierarchy udpate in one pass, but the parenting logic would be more complex.
        CompositingState compState(updateRoot, true);
        bool layersChanged = false;
        bool saw3DTransform = false;
        {
            TRACE_EVENT0("blink_rendering", "RenderLayerCompositor::computeCompositingRequirements");
            OverlapMap overlapTestRequestMap;
            computeCompositingRequirements(0, updateRoot, &overlapTestRequestMap, compState, layersChanged, saw3DTransform);
        }
        needHierarchyUpdate |= layersChanged;
    }

#if !LOG_DISABLED
    if (compositingLogEnabled() && isFullUpdate && (needHierarchyUpdate || needGeometryUpdate)) {
        m_obligateCompositedLayerCount = 0;
        m_secondaryCompositedLayerCount = 0;
        m_obligatoryBackingStoreBytes = 0;
        m_secondaryBackingStoreBytes = 0;

        Frame* frame = m_renderView->frameView()->frame();
        LOG(Compositing, "\nUpdate %d of %s.\n", m_rootLayerUpdateCount, isMainFrame() ? "main frame" : frame->tree()->uniqueName().string().utf8().data());
    }
#endif

    if (needHierarchyUpdate) {
        // Update the hierarchy of the compositing layers.
        Vector<GraphicsLayer*> childList;
        {
            TRACE_EVENT0("blink_rendering", "RenderLayerCompositor::rebuildCompositingLayerTree");
            rebuildCompositingLayerTree(updateRoot, childList, 0);
        }

        // Host the document layer in the RenderView's root layer.
        if (isFullUpdate) {
            // Even when childList is empty, don't drop out of compositing mode if there are
            // composited layers that we didn't hit in our traversal (e.g. because of visibility:hidden).
            if (childList.isEmpty() && !hasAnyAdditionalCompositedLayers(updateRoot))
                destroyRootLayer();
            else
                m_rootContentLayer->setChildren(childList);
        }
    } else if (needGeometryUpdate) {
        // We just need to do a geometry update. This is only used for position:fixed scrolling;
        // most of the time, geometry is updated via RenderLayer::styleChanged().
        updateLayerTreeGeometry(updateRoot, 0);
    }
    
#if !LOG_DISABLED
    if (compositingLogEnabled() && isFullUpdate && (needHierarchyUpdate || needGeometryUpdate)) {
        double endTime = currentTime();
        LOG(Compositing, "Total layers   primary   secondary   obligatory backing (KB)   secondary backing(KB)   total backing (KB)  update time (ms)\n");

        LOG(Compositing, "%8d %11d %9d %20.2f %22.2f %22.2f %18.2f\n",
            m_obligateCompositedLayerCount + m_secondaryCompositedLayerCount, m_obligateCompositedLayerCount,
            m_secondaryCompositedLayerCount, m_obligatoryBackingStoreBytes / 1024, m_secondaryBackingStoreBytes / 1024, (m_obligatoryBackingStoreBytes + m_secondaryBackingStoreBytes) / 1024, 1000.0 * (endTime - startTime));
    }
#endif
    ASSERT(updateRoot || !m_compositingLayersNeedRebuild);

    if (!hasAcceleratedCompositing())
        enableCompositingMode(false);

    // Inform the inspector that the layer tree has changed.
    InspectorInstrumentation::layerTreeDidChange(page());
}

void RenderLayerCompositor::layerBecameNonComposited(const RenderLayer* renderLayer)
{
    // Inform the inspector that the given RenderLayer was destroyed.
    InspectorInstrumentation::renderLayerDestroyed(page(), renderLayer);

    ASSERT(m_compositedLayerCount > 0);
    --m_compositedLayerCount;
}

static bool requiresCompositing(CompositingReasons reasons)
{
    return reasons != CompositingReasonNone;
}

#if !LOG_DISABLED
void RenderLayerCompositor::logLayerInfo(const RenderLayer* layer, int depth)
{
    if (!compositingLogEnabled())
        return;
        
    RenderLayerBacking* backing = layer->backing();
    if (requiresCompositing(directReasonsForCompositing(layer)) || layer->isRootLayer()) {
        ++m_obligateCompositedLayerCount;
        m_obligatoryBackingStoreBytes += backing->backingStoreMemoryEstimate();
    } else {
        ++m_secondaryCompositedLayerCount;
        m_secondaryBackingStoreBytes += backing->backingStoreMemoryEstimate();
    }

    String layerName;
#ifndef NDEBUG
    layerName = layer->debugName();
#endif

    LOG(Compositing, "%*p %dx%d %.2fKB (%s) %s\n", 12 + depth * 2, layer, backing->compositedBounds().width(), backing->compositedBounds().height(),
        backing->backingStoreMemoryEstimate() / 1024,
        logReasonsForCompositing(layer), layerName.utf8().data());
}
#endif

void RenderLayerCompositor::addOutOfFlowPositionedLayer(RenderLayer* layer)
{
    m_outOfFlowPositionedLayers.add(layer);
}

void RenderLayerCompositor::removeOutOfFlowPositionedLayer(RenderLayer* layer)
{
    m_outOfFlowPositionedLayers.remove(layer);
}

bool RenderLayerCompositor::updateBacking(RenderLayer* layer, CompositingChangeRepaint shouldRepaint)
{
    bool layerChanged = false;
    RenderLayer::ViewportConstrainedNotCompositedReason viewportConstrainedNotCompositedReason = RenderLayer::NoNotCompositedReason;
    requiresCompositingForPosition(layer->renderer(), layer, &viewportConstrainedNotCompositedReason);

    // FIXME: It would be nice to directly use the layer's compositing reason,
    // but updateBacking() also gets called without having updated compositing
    // requirements fully.
    if (needsToBeComposited(layer)) {
        enableCompositingMode();
        
        if (!layer->backing()) {
            // If we need to repaint, do so before making backing
            if (shouldRepaint == CompositingChangeRepaintNow)
                repaintOnCompositingChange(layer);

            layer->ensureBacking();

            // At this time, the ScrollingCooridnator only supports the top-level frame.
            if (layer->isRootLayer() && !isMainFrame()) {
                if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                    scrollingCoordinator->frameViewRootLayerDidChange(m_renderView->frameView());
            }

            // This layer and all of its descendants have cached repaints rects that are relative to
            // the repaint container, so change when compositing changes; we need to update them here.
            if (layer->parent())
                layer->computeRepaintRectsIncludingDescendants();

            layerChanged = true;
        }
    } else {
        if (layer->backing()) {
            // If we're removing backing on a reflection, clear the source GraphicsLayer's pointer to
            // its replica GraphicsLayer. In practice this should never happen because reflectee and reflection 
            // are both either composited, or not composited.
            if (layer->isReflection()) {
                RenderLayer* sourceLayer = toRenderLayerModelObject(layer->renderer()->parent())->layer();
                if (RenderLayerBacking* backing = sourceLayer->backing()) {
                    ASSERT(backing->graphicsLayer()->replicaLayer() == layer->backing()->graphicsLayer());
                    backing->graphicsLayer()->setReplicatedByLayer(0);
                }
            }

            removeViewportConstrainedLayer(layer);
            
            layer->clearBacking();
            layerChanged = true;

            // This layer and all of its descendants have cached repaints rects that are relative to
            // the repaint container, so change when compositing changes; we need to update them here.
            layer->computeRepaintRectsIncludingDescendants();

            // If we need to repaint, do so now that we've removed the backing
            if (shouldRepaint == CompositingChangeRepaintNow)
                repaintOnCompositingChange(layer);
        }
    }

    if (layerChanged && layer->renderer()->isRenderPart()) {
        RenderLayerCompositor* innerCompositor = frameContentsCompositor(toRenderPart(layer->renderer()));
        if (innerCompositor && innerCompositor->inCompositingMode())
            innerCompositor->updateRootLayerAttachment();
    }
    
    if (layerChanged)
        layer->clearClipRectsIncludingDescendants(PaintingClipRects);

    // If a fixed position layer gained/lost a backing or the reason not compositing it changed,
    // the scrolling coordinator needs to recalculate whether it can do fast scrolling.
    if (layer->renderer()->style()->position() == FixedPosition) {
        if (layer->viewportConstrainedNotCompositedReason() != viewportConstrainedNotCompositedReason) {
            layer->setViewportConstrainedNotCompositedReason(viewportConstrainedNotCompositedReason);
            layerChanged = true;
        }
        if (layerChanged) {
            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewFixedObjectsDidChange(m_renderView->frameView());
        }
    }
    
    if (layer->backing())
        layer->backing()->updateDebugIndicators(m_showDebugBorders, m_showRepaintCounter);

    return layerChanged;
}

bool RenderLayerCompositor::updateLayerCompositingState(RenderLayer* layer, CompositingChangeRepaint shouldRepaint)
{
    bool layerChanged = updateBacking(layer, shouldRepaint);

    // See if we need content or clipping layers. Methods called here should assume
    // that the compositing state of descendant layers has not been updated yet.
    if (layer->backing() && layer->backing()->updateGraphicsLayerConfiguration())
        layerChanged = true;

    return layerChanged;
}

void RenderLayerCompositor::repaintOnCompositingChange(RenderLayer* layer)
{
    // If the renderer is not attached yet, no need to repaint.
    if (layer->renderer() != m_renderView && !layer->renderer()->parent())
        return;

    RenderLayerModelObject* repaintContainer = layer->renderer()->containerForRepaint();
    if (!repaintContainer)
        repaintContainer = m_renderView;

    layer->repaintIncludingNonCompositingDescendants(repaintContainer);
}

// This method assumes that layout is up-to-date, unlike repaintOnCompositingChange().
void RenderLayerCompositor::repaintInCompositedAncestor(RenderLayer* layer, const LayoutRect& rect)
{
    RenderLayer* compositedAncestor = layer->enclosingCompositingLayerForRepaint(false /*exclude self*/);
    if (compositedAncestor) {
        ASSERT(compositedAncestor->backing());

        LayoutPoint offset;
        layer->convertToLayerCoords(compositedAncestor, offset);

        LayoutRect repaintRect = rect;
        repaintRect.moveBy(offset);

        compositedAncestor->setBackingNeedsRepaintInRect(repaintRect);
    }
}

// The bounds of the GraphicsLayer created for a compositing layer is the union of the bounds of all the descendant
// RenderLayers that are rendered by the composited RenderLayer.
IntRect RenderLayerCompositor::calculateCompositedBounds(const RenderLayer* layer, const RenderLayer* ancestorLayer) const
{
    if (!canBeComposited(layer))
        return IntRect();

    RenderLayer::CalculateLayerBoundsFlags flags = RenderLayer::DefaultCalculateLayerBoundsFlags | RenderLayer::ExcludeHiddenDescendants | RenderLayer::DontConstrainForMask;
#if HAVE(COMPOSITOR_FILTER_OUTSETS)
    // If the compositor computes its own filter outsets, don't include them in the composited bounds.
    if (!layer->paintsWithFilters())
        flags &= ~RenderLayer::IncludeLayerFilterOutsets;
#endif
    return layer->calculateLayerBounds(ancestorLayer, 0, flags);
}

void RenderLayerCompositor::layerWasAdded(RenderLayer* /*parent*/, RenderLayer* /*child*/)
{
    setCompositingLayersNeedRebuild();
}

void RenderLayerCompositor::layerWillBeRemoved(RenderLayer* parent, RenderLayer* child)
{
    if (!child->isComposited() || parent->renderer()->documentBeingDestroyed())
        return;

    removeViewportConstrainedLayer(child);
    repaintInCompositedAncestor(child, child->backing()->compositedBounds());

    setCompositingParent(child, 0);
    setCompositingLayersNeedRebuild();
}

RenderLayer* RenderLayerCompositor::enclosingNonStackingClippingLayer(const RenderLayer* layer) const
{
    for (RenderLayer* curr = layer->parent(); curr != 0; curr = curr->parent()) {
        if (curr->isStackingContainer())
            return 0;

        if (curr->renderer()->hasClipOrOverflowClip())
            return curr;
    }
    return 0;
}

void RenderLayerCompositor::addToOverlapMap(OverlapMap& overlapMap, RenderLayer* layer, IntRect& layerBounds, bool& boundsComputed)
{
    if (layer->isRootLayer())
        return;

    if (!boundsComputed) {
        // FIXME: If this layer's overlap bounds include its children, we don't need to add its
        // children's bounds to the overlap map.
        layerBounds = enclosingIntRect(overlapMap.geometryMap().absoluteRect(layer->overlapBounds()));
        // Empty rects never intersect, but we need them to for the purposes of overlap testing.
        if (layerBounds.isEmpty())
            layerBounds.setSize(IntSize(1, 1));
        boundsComputed = true;
    }

    IntRect clipRect = pixelSnappedIntRect(layer->backgroundClipRect(RenderLayer::ClipRectsContext(rootRenderLayer(), 0, AbsoluteClipRects)).rect()); // FIXME: Incorrect for CSS regions.
    clipRect.intersect(layerBounds);
    overlapMap.add(layer, clipRect);
}

void RenderLayerCompositor::addToOverlapMapRecursive(OverlapMap& overlapMap, RenderLayer* layer, RenderLayer* ancestorLayer)
{
    if (!canBeComposited(layer) || overlapMap.contains(layer))
        return;

    // A null ancestorLayer is an indication that 'layer' has already been pushed.
    if (ancestorLayer)
        overlapMap.geometryMap().pushMappingsToAncestor(layer, ancestorLayer);
    
    IntRect bounds;
    bool haveComputedBounds = false;
    addToOverlapMap(overlapMap, layer, bounds, haveComputedBounds);

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = negZOrderList->at(i);
                addToOverlapMapRecursive(overlapMap, curLayer, layer);
            }
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);
            addToOverlapMapRecursive(overlapMap, curLayer, layer);
        }
    }

    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = posZOrderList->at(i);
                addToOverlapMapRecursive(overlapMap, curLayer, layer);
            }
        }
    }
    
    if (ancestorLayer)
        overlapMap.geometryMap().popMappingsToAncestor(ancestorLayer);
}

//  Recurse through the layers in z-index and overflow order (which is equivalent to painting order)
//  For the z-order children of a compositing layer:
//      If a child layers has a compositing layer, then all subsequent layers must
//      be compositing in order to render above that layer.
//
//      If a child in the negative z-order list is compositing, then the layer itself
//      must be compositing so that its contents render over that child.
//      This implies that its positive z-index children must also be compositing.
//
void RenderLayerCompositor::computeCompositingRequirements(RenderLayer* ancestorLayer, RenderLayer* layer, OverlapMap* overlapMap, CompositingState& compositingState, bool& layersChanged, bool& descendantHas3DTransform)
{
    layer->updateLayerListsIfNeeded();
    
    if (overlapMap)
        overlapMap->geometryMap().pushMappingsToAncestor(layer, ancestorLayer);
    
    // Clear the flag
    layer->setHasCompositingDescendant(false);

    // Start by assuming this layer will not need to composite.
    CompositingReasons reasonsToComposite = CompositingReasonNone;

    // First accumulate the straightforward compositing reasons.
    CompositingReasons directReasons = directReasonsForCompositing(layer);
    if (canBeComposited(layer)) {
        reasonsToComposite |= directReasons;
        reasonsToComposite |= (inCompositingMode() && layer->isRootLayer()) ? CompositingReasonRoot : CompositingReasonNone;
    }

    // Next, accumulate reasons related to overlap.
    // If overlap testing is used, this reason will be overridden. If overlap testing is not
    // used, we must assume we overlap if there is anything composited behind us in paint-order.
    CompositingReasons overlapCompositingReason = compositingState.m_subtreeIsCompositing ? CompositingReasonAssumedOverlap : CompositingReasonNone;

    bool haveComputedBounds = false;
    IntRect absBounds;
    // If we know for sure the layer is going to be composited, don't bother looking it up in the overlap map.
    if (overlapMap && !overlapMap->isEmpty() && compositingState.m_testingOverlap && !requiresCompositing(directReasons)) {
        // If we're testing for overlap, we only need to composite if we overlap something that is already composited.
        absBounds = enclosingIntRect(overlapMap->geometryMap().absoluteRect(layer->overlapBounds()));

        // Empty rects never intersect, but we need them to for the purposes of overlap testing.
        if (absBounds.isEmpty())
            absBounds.setSize(IntSize(1, 1));
        haveComputedBounds = true;
        overlapCompositingReason = overlapMap->overlapsLayers(absBounds) ? CompositingReasonOverlap : CompositingReasonNone;
    }

    // Video is special. It's the only RenderLayer type that can both have
    // RenderLayer children and whose children can't use its backing to render
    // into. These children (the controls) always need to be promoted into their
    // own layers to draw on top of the accelerated video.
    if (compositingState.m_compositingAncestor && compositingState.m_compositingAncestor->renderer()->isVideo())
        overlapCompositingReason = CompositingReasonOverlap;

    reasonsToComposite |= overlapCompositingReason;

    // The children of this layer don't need to composite, unless there is
    // a compositing layer among them, so start by inheriting the compositing
    // ancestor with m_subtreeIsCompositing set to false.
    CompositingState childState(compositingState);
    childState.m_subtreeIsCompositing = false;

    bool willBeComposited = canBeComposited(layer) && requiresCompositing(reasonsToComposite);
    if (willBeComposited) {
        // Tell the parent it has compositing descendants.
        compositingState.m_subtreeIsCompositing = true;
        // This layer now acts as the ancestor for kids.
        childState.m_compositingAncestor = layer;

        if (overlapMap)
            overlapMap->pushCompositingContainer();
        // This layer is going to be composited, so children can safely ignore the fact that there's an 
        // animation running behind this layer, meaning they can rely on the overlap map testing again.
        childState.m_testingOverlap = true;
    }

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    bool anyDescendantHas3DTransform = false;

    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = negZOrderList->at(i);
                computeCompositingRequirements(layer, curLayer, overlapMap, childState, layersChanged, anyDescendantHas3DTransform);

                // If we have to make a layer for this child, make one now so we can have a contents layer
                // (since we need to ensure that the -ve z-order child renders underneath our contents).
                if (childState.m_subtreeIsCompositing) {
                    reasonsToComposite |= CompositingReasonNegativeZIndexChildren;

                    if (!willBeComposited) {
                        // make layer compositing
                        childState.m_compositingAncestor = layer;
                        if (overlapMap)
                            overlapMap->pushCompositingContainer();
                        // This layer is going to be composited, so children can safely ignore the fact that there's an
                        // animation running behind this layer, meaning they can rely on the overlap map testing again
                        childState.m_testingOverlap = true;
                        willBeComposited = true;
                    }
                }
            }
        }
    }
    
    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);
            computeCompositingRequirements(layer, curLayer, overlapMap, childState, layersChanged, anyDescendantHas3DTransform);
        }
    }

    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = posZOrderList->at(i);
                computeCompositingRequirements(layer, curLayer, overlapMap, childState, layersChanged, anyDescendantHas3DTransform);
            }
        }
    }

    // Now that the subtree has been traversed, we can check for compositing reasons that depended on the state of the subtree.

    // If we entered compositing mode during the recursion, the root will also need to be composited (as long as accelerated compositing is enabled).
    if (layer->isRootLayer()) {
        if (inCompositingMode() && m_hasAcceleratedCompositing)
            willBeComposited = true;
    }

    // All layers (even ones that aren't being composited) need to get added to
    // the overlap map. Layers that do not composite will draw into their
    // compositing ancestor's backing, and so are still considered for overlap.
    if (overlapMap && childState.m_compositingAncestor && !childState.m_compositingAncestor->isRootLayer())
        addToOverlapMap(*overlapMap, layer, absBounds, haveComputedBounds);

    // Now check for reasons to become composited that depend on the state of descendant layers.
    CompositingReasons subtreeCompositingReasons = subtreeReasonsForCompositing(layer->renderer(), childState.m_subtreeIsCompositing, anyDescendantHas3DTransform);
    reasonsToComposite |= subtreeCompositingReasons;
    if (!willBeComposited && canBeComposited(layer) && requiresCompositing(subtreeCompositingReasons)) {
        childState.m_compositingAncestor = layer;
        if (overlapMap) {
            overlapMap->pushCompositingContainer();
            addToOverlapMapRecursive(*overlapMap, layer);
        }
        willBeComposited = true;
    }

    // If the original layer is composited, the reflection needs to be, too.
    if (layer->reflectionLayer()) {
        // FIXME: Shouldn't we call computeCompositingRequirements to handle a reflection overlapping with another renderer?
        CompositingReasons reflectionCompositingReason = willBeComposited ? CompositingReasonReflectionOfCompositedParent : CompositingReasonNone;
        layer->reflectionLayer()->setCompositingReasons(layer->reflectionLayer()->compositingReasons() | reflectionCompositingReason);
    }

    // Subsequent layers in the parent's stacking context may also need to composite.
    if (childState.m_subtreeIsCompositing)
        compositingState.m_subtreeIsCompositing = true;

    // Set the flag to say that this SC has compositing children.
    layer->setHasCompositingDescendant(childState.m_subtreeIsCompositing);


    // Turn overlap testing off for later layers if it's already off, or if we have an animating transform.
    // Note that if the layer clips its descendants, there's no reason to propagate the child animation to the parent layers. That's because
    // we know for sure the animation is contained inside the clipping rectangle, which is already added to the overlap map.
    bool isCompositedClippingLayer = canBeComposited(layer) && (reasonsToComposite & CompositingReasonClipsCompositingDescendants);
    if ((!childState.m_testingOverlap && !isCompositedClippingLayer) || isRunningAcceleratedTransformAnimation(layer->renderer()))
        compositingState.m_testingOverlap = false;
    
    if (overlapMap && childState.m_compositingAncestor == layer && !layer->isRootLayer())
        overlapMap->popCompositingContainer();

    // If we're back at the root, and no other layers need to be composited, and the root layer itself doesn't need
    // to be composited, then we can drop out of compositing mode altogether. However, don't drop out of compositing mode
    // if there are composited layers that we didn't hit in our traversal (e.g. because of visibility:hidden).
    // FIXME: hasAnyAdditionalCompositedLayers() code seems fishy. We need to make root layer logic more obvious.
    if (layer->isRootLayer() && !childState.m_subtreeIsCompositing && !requiresCompositing(directReasons) && !m_forceCompositingMode && !hasAnyAdditionalCompositedLayers(layer)) {
        enableCompositingMode(false);
        willBeComposited = false;
        reasonsToComposite = CompositingReasonNone;
    }

    // At this point we have finished collecting all reasons to composite this layer.
    layer->setCompositingReasons(reasonsToComposite);

    // If the layer is going into compositing mode, repaint its old location.
    if (!layer->isComposited() && willBeComposited)
        repaintOnCompositingChange(layer);

    // Update backing now, so that we can use isComposited() reliably during tree traversal in rebuildCompositingLayerTree().
    if (updateBacking(layer, CompositingChangeRepaintNow))
        layersChanged = true;

    if (layer->reflectionLayer() && updateLayerCompositingState(layer->reflectionLayer(), CompositingChangeRepaintNow))
        layersChanged = true;

    descendantHas3DTransform |= anyDescendantHas3DTransform || layer->has3DTransform();

    if (overlapMap)
        overlapMap->geometryMap().popMappingsToAncestor(ancestorLayer);
}

void RenderLayerCompositor::setCompositingParent(RenderLayer* childLayer, RenderLayer* parentLayer)
{
    ASSERT(!parentLayer || childLayer->ancestorCompositingLayer() == parentLayer);
    ASSERT(childLayer->isComposited());

    // It's possible to be called with a parent that isn't yet composited when we're doing
    // partial updates as required by painting or hit testing. Just bail in that case;
    // we'll do a full layer update soon.
    if (!parentLayer || !parentLayer->isComposited())
        return;

    if (parentLayer) {
        GraphicsLayer* hostingLayer = parentLayer->backing()->parentForSublayers();
        GraphicsLayer* hostedLayer = childLayer->backing()->childForSuperlayers();
        
        hostingLayer->addChild(hostedLayer);
    } else
        childLayer->backing()->childForSuperlayers()->removeFromParent();
}

void RenderLayerCompositor::removeCompositedChildren(RenderLayer* layer)
{
    ASSERT(layer->isComposited());

    GraphicsLayer* hostingLayer = layer->backing()->parentForSublayers();
    hostingLayer->removeAllChildren();
}

bool RenderLayerCompositor::canAccelerateVideoRendering(RenderVideo* o) const
{
    if (!m_hasAcceleratedCompositing)
        return false;

    return o->supportsAcceleratedRendering();
}

void RenderLayerCompositor::rebuildCompositingLayerTree(RenderLayer* layer, Vector<GraphicsLayer*>& childLayersOfEnclosingLayer, int depth)
{
    // Make the layer compositing if necessary, and set up clipping and content layers.
    // Note that we can only do work here that is independent of whether the descendant layers
    // have been processed. computeCompositingRequirements() will already have done the repaint if necessary.

    // Used for gathering UMA data about the effect on memory usage of promoting all layers
    // that have a webkit-transition on opacity or transform and intersect the viewport.
    static double pixelsWithoutPromotingAllTransitions = 0.0;
    static double pixelsAddedByPromotingAllTransitions = 0.0;

    if (!depth) {
        pixelsWithoutPromotingAllTransitions = 0.0;
        pixelsAddedByPromotingAllTransitions = 0.0;
    }

    RenderLayerBacking* layerBacking = layer->backing();
    if (layerBacking) {
        // The compositing state of all our children has been updated already, so now
        // we can compute and cache the composited bounds for this layer.
        layerBacking->updateCompositedBounds();

        if (RenderLayer* reflection = layer->reflectionLayer()) {
            if (reflection->backing())
                reflection->backing()->updateCompositedBounds();
        }

        if (layerBacking->updateGraphicsLayerConfiguration())
            layerBacking->updateDebugIndicators(m_showDebugBorders, m_showRepaintCounter);
        
        layerBacking->updateGraphicsLayerGeometry();

        if (!layer->parent())
            updateRootLayerPosition();

#if !LOG_DISABLED
        logLayerInfo(layer, depth);
#else
        UNUSED_PARAM(depth);
#endif
        if (layerBacking->hasUnpositionedOverflowControlsLayers())
            layer->positionNewlyCreatedOverflowControls();

        pixelsWithoutPromotingAllTransitions += layer->size().height() * layer->size().width();
    } else {
        if ((layer->renderer()->style()->transitionForProperty(CSSPropertyOpacity) ||
             layer->renderer()->style()->transitionForProperty(CSSPropertyWebkitTransform)) &&
            m_renderView->viewRect().intersects(layer->absoluteBoundingBox()))
            pixelsAddedByPromotingAllTransitions += layer->size().height() * layer->size().width();
    }

    // If this layer has backing, then we are collecting its children, otherwise appending
    // to the compositing child list of an enclosing layer.
    Vector<GraphicsLayer*> layerChildren;
    Vector<GraphicsLayer*>& childList = layerBacking ? layerChildren : childLayersOfEnclosingLayer;

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = negZOrderList->at(i);
                rebuildCompositingLayerTree(curLayer, childList, depth + 1);
            }
        }

        // If a negative z-order child is compositing, we get a foreground layer which needs to get parented.
        if (layerBacking && layerBacking->foregroundLayer())
            childList.append(layerBacking->foregroundLayer());
    }

    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);
            rebuildCompositingLayerTree(curLayer, childList, depth + 1);
        }
    }
    
    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = posZOrderList->at(i);
                rebuildCompositingLayerTree(curLayer, childList, depth + 1);
            }
        }
    }
    
    if (layerBacking) {
        bool parented = false;
        if (layer->renderer()->isRenderPart())
            parented = parentFrameContentLayers(toRenderPart(layer->renderer()));

        if (!parented)
            layerBacking->parentForSublayers()->setChildren(layerChildren);

        // If the layer has a clipping layer the overflow controls layers will be siblings of the clipping layer.
        // Otherwise, the overflow control layers are normal children.
        if (!layerBacking->hasClippingLayer() && !layerBacking->hasScrollingLayer()) {
            if (GraphicsLayer* overflowControlLayer = layerBacking->layerForHorizontalScrollbar()) {
                overflowControlLayer->removeFromParent();
                layerBacking->parentForSublayers()->addChild(overflowControlLayer);
            }

            if (GraphicsLayer* overflowControlLayer = layerBacking->layerForVerticalScrollbar()) {
                overflowControlLayer->removeFromParent();
                layerBacking->parentForSublayers()->addChild(overflowControlLayer);
            }

            if (GraphicsLayer* overflowControlLayer = layerBacking->layerForScrollCorner()) {
                overflowControlLayer->removeFromParent();
                layerBacking->parentForSublayers()->addChild(overflowControlLayer);
            }
        }

        childLayersOfEnclosingLayer.append(layerBacking->childForSuperlayers());
    }

    if (!depth) {
        int percentageIncreaseInPixels = static_cast<int>(pixelsAddedByPromotingAllTransitions / pixelsWithoutPromotingAllTransitions * 100);
        HistogramSupport::histogramCustomCounts("Renderer.PixelIncreaseFromTransitions", percentageIncreaseInPixels, 0, 1000, 50);
    }
}

void RenderLayerCompositor::frameViewDidChangeLocation(const IntPoint& contentsOffset)
{
    if (m_overflowControlsHostLayer)
        m_overflowControlsHostLayer->setPosition(contentsOffset);
}

void RenderLayerCompositor::frameViewDidChangeSize()
{
    if (m_clipLayer) {
        FrameView* frameView = m_renderView->frameView();
        m_clipLayer->setSize(frameView->unscaledVisibleContentSize());

        frameViewDidScroll();
        updateOverflowControlsLayers();

#if ENABLE(RUBBER_BANDING)
        if (m_layerForOverhangAreas)
            m_layerForOverhangAreas->setSize(frameView->frameRect().size());
#endif
    }
}

void RenderLayerCompositor::frameViewDidScroll()
{
    FrameView* frameView = m_renderView->frameView();
    IntPoint scrollPosition = frameView->scrollPosition();

    if (!m_scrollLayer)
        return;

    // If there's a scrolling coordinator that manages scrolling for this frame view,
    // it will also manage updating the scroll layer position.
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator()) {
        if (Settings* settings = m_renderView->document()->settings()) {
            if (isMainFrame() || settings->compositedScrollingForFramesEnabled())
                scrollingCoordinator->scrollableAreaScrollLayerDidChange(frameView);
        }
    }

    m_scrollLayer->setPosition(FloatPoint(-scrollPosition.x(), -scrollPosition.y()));

    if (GraphicsLayer* fixedBackgroundLayer = fixedRootBackgroundLayer())
        fixedBackgroundLayer->setPosition(IntPoint(frameView->scrollOffsetForFixedPosition()));
}

void RenderLayerCompositor::frameViewDidLayout()
{
}

void RenderLayerCompositor::rootFixedBackgroundsChanged()
{
    // FIXME: Implement when root fixed background layer is implemented.
}

void RenderLayerCompositor::scrollingLayerDidChange(RenderLayer* layer)
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->scrollableAreaScrollLayerDidChange(layer);
}

String RenderLayerCompositor::layerTreeAsText(LayerTreeFlags flags)
{
    updateCompositingLayers(CompositingUpdateAfterLayout);

    if (!m_rootContentLayer)
        return String();

    // We skip dumping the scroll and clip layers to keep layerTreeAsText output
    // similar between platforms.
    String layerTreeText = m_rootContentLayer->layerTreeAsText(flags);

    // The true root layer is not included in the dump, so if we want to report
    // its repaint rects, they must be included here.
    if (flags & LayerTreeIncludesRepaintRects) {
        String layerTreeTextWithRootRepaintRects = m_renderView->frameView()->trackedRepaintRectsAsText();
        layerTreeTextWithRootRepaintRects.append(layerTreeText);
        return layerTreeTextWithRootRepaintRects;
    }

    return layerTreeText;
}

RenderLayerCompositor* RenderLayerCompositor::frameContentsCompositor(RenderPart* renderer)
{
    if (!renderer->node()->isFrameOwnerElement())
        return 0;
        
    HTMLFrameOwnerElement* element = toFrameOwnerElement(renderer->node());
    if (Document* contentDocument = element->contentDocument()) {
        if (RenderView* view = contentDocument->renderView())
            return view->compositor();
    }
    return 0;
}

bool RenderLayerCompositor::parentFrameContentLayers(RenderPart* renderer)
{
    RenderLayerCompositor* innerCompositor = frameContentsCompositor(renderer);
    if (!innerCompositor || !innerCompositor->inCompositingMode() || innerCompositor->rootLayerAttachment() != RootLayerAttachedViaEnclosingFrame)
        return false;
    
    RenderLayer* layer = renderer->layer();
    if (!layer->isComposited())
        return false;

    RenderLayerBacking* backing = layer->backing();
    GraphicsLayer* hostingLayer = backing->parentForSublayers();
    GraphicsLayer* rootLayer = innerCompositor->rootGraphicsLayer();
    if (hostingLayer->children().size() != 1 || hostingLayer->children()[0] != rootLayer) {
        hostingLayer->removeAllChildren();
        hostingLayer->addChild(rootLayer);
    }
    return true;
}

// This just updates layer geometry without changing the hierarchy.
void RenderLayerCompositor::updateLayerTreeGeometry(RenderLayer* layer, int depth)
{
    if (RenderLayerBacking* layerBacking = layer->backing()) {
        // The compositing state of all our children has been updated already, so now
        // we can compute and cache the composited bounds for this layer.
        layerBacking->updateCompositedBounds();

        if (RenderLayer* reflection = layer->reflectionLayer()) {
            if (reflection->backing())
                reflection->backing()->updateCompositedBounds();
        }

        layerBacking->updateGraphicsLayerConfiguration();
        layerBacking->updateGraphicsLayerGeometry();

        if (!layer->parent())
            updateRootLayerPosition();

#if !LOG_DISABLED
        logLayerInfo(layer, depth);
#else
        UNUSED_PARAM(depth);
#endif
    }

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i)
                updateLayerTreeGeometry(negZOrderList->at(i), depth + 1);
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i)
            updateLayerTreeGeometry(normalFlowList->at(i), depth + 1);
    }
    
    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i)
                updateLayerTreeGeometry(posZOrderList->at(i), depth + 1);
        }
    }
}

// Recurs down the RenderLayer tree until its finds the compositing descendants of compositingAncestor and updates their geometry.
void RenderLayerCompositor::updateCompositingDescendantGeometry(RenderLayer* compositingAncestor, RenderLayer* layer, bool compositedChildrenOnly)
{
    if (layer != compositingAncestor) {
        if (RenderLayerBacking* layerBacking = layer->backing()) {
            layerBacking->updateCompositedBounds();

            if (RenderLayer* reflection = layer->reflectionLayer()) {
                if (reflection->backing())
                    reflection->backing()->updateCompositedBounds();
            }

            layerBacking->updateGraphicsLayerGeometry();
            if (compositedChildrenOnly)
                return;
        }
    }

    if (layer->reflectionLayer())
        updateCompositingDescendantGeometry(compositingAncestor, layer->reflectionLayer(), compositedChildrenOnly);

    if (!layer->hasCompositingDescendant())
        return;

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif
    
    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i)
                updateCompositingDescendantGeometry(compositingAncestor, negZOrderList->at(i), compositedChildrenOnly);
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i)
            updateCompositingDescendantGeometry(compositingAncestor, normalFlowList->at(i), compositedChildrenOnly);
    }
    
    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i)
                updateCompositingDescendantGeometry(compositingAncestor, posZOrderList->at(i), compositedChildrenOnly);
        }
    }
}


void RenderLayerCompositor::repaintCompositedLayers(const IntRect* absRect)
{
    recursiveRepaintLayer(rootRenderLayer(), absRect);
}

void RenderLayerCompositor::recursiveRepaintLayer(RenderLayer* layer, const IntRect* rect)
{
    // FIXME: This method does not work correctly with transforms.
    if (layer->isComposited() && !layer->backing()->paintsIntoCompositedAncestor()) {
        if (rect)
            layer->setBackingNeedsRepaintInRect(*rect);
        else
            layer->setBackingNeedsRepaint();
    }

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    if (layer->hasCompositingDescendant()) {
        if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = negZOrderList->at(i);
                if (rect) {
                    IntRect childRect(*rect);
                    curLayer->convertToPixelSnappedLayerCoords(layer, childRect);
                    recursiveRepaintLayer(curLayer, &childRect);
                } else
                    recursiveRepaintLayer(curLayer);
            }
        }

        if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = posZOrderList->at(i);
                if (rect) {
                    IntRect childRect(*rect);
                    curLayer->convertToPixelSnappedLayerCoords(layer, childRect);
                    recursiveRepaintLayer(curLayer, &childRect);
                } else
                    recursiveRepaintLayer(curLayer);
            }
        }
    }
    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);
            if (rect) {
                IntRect childRect(*rect);
                curLayer->convertToPixelSnappedLayerCoords(layer, childRect);
                recursiveRepaintLayer(curLayer, &childRect);
            } else
                recursiveRepaintLayer(curLayer);
        }
    }
}

RenderLayer* RenderLayerCompositor::rootRenderLayer() const
{
    return m_renderView->layer();
}

GraphicsLayer* RenderLayerCompositor::rootGraphicsLayer() const
{
    if (m_overflowControlsHostLayer)
        return m_overflowControlsHostLayer.get();
    return m_rootContentLayer.get();
}

GraphicsLayer* RenderLayerCompositor::scrollLayer() const
{
    return m_scrollLayer.get();
}

void RenderLayerCompositor::setIsInWindow(bool isInWindow)
{
    if (!inCompositingMode())
        return;

    if (isInWindow) {
        if (m_rootLayerAttachment != RootLayerUnattached)
            return;

        RootLayerAttachment attachment = isMainFrame() ? RootLayerAttachedViaChromeClient : RootLayerAttachedViaEnclosingFrame;
        attachRootLayer(attachment);
    } else {
        if (m_rootLayerAttachment == RootLayerUnattached)
            return;

        detachRootLayer();
    }
}

void RenderLayerCompositor::clearBackingForLayerIncludingDescendants(RenderLayer* layer)
{
    if (!layer)
        return;

    if (layer->isComposited()) {
        removeViewportConstrainedLayer(layer);
        layer->clearBacking();
    }
    
    for (RenderLayer* currLayer = layer->firstChild(); currLayer; currLayer = currLayer->nextSibling())
        clearBackingForLayerIncludingDescendants(currLayer);
}

void RenderLayerCompositor::clearBackingForAllLayers()
{
    clearBackingForLayerIncludingDescendants(m_renderView->layer());
}

void RenderLayerCompositor::updateRootLayerPosition()
{
    if (m_rootContentLayer) {
        const IntRect& documentRect = m_renderView->documentRect();
        m_rootContentLayer->setSize(documentRect.size());
        m_rootContentLayer->setPosition(documentRect.location());
    }
    if (m_clipLayer) {
        FrameView* frameView = m_renderView->frameView();
        m_clipLayer->setSize(frameView->unscaledVisibleContentSize());
    }

#if ENABLE(RUBBER_BANDING)
    if (m_contentShadowLayer) {
        m_contentShadowLayer->setPosition(m_rootContentLayer->position());

        FloatSize rootContentLayerSize = m_rootContentLayer->size();
        if (m_contentShadowLayer->size() != rootContentLayerSize) {
            m_contentShadowLayer->setSize(rootContentLayerSize);
            ScrollbarTheme::theme()->setUpContentShadowLayer(m_contentShadowLayer.get());
        }
    }

    updateLayerForTopOverhangArea(m_layerForTopOverhangArea);
    updateLayerForBottomOverhangArea(m_layerForBottomOverhangArea);
#endif
}

bool RenderLayerCompositor::has3DContent() const
{
    return layerHas3DContent(rootRenderLayer());
}

bool RenderLayerCompositor::needsToBeComposited(const RenderLayer* layer) const
{
    if (!canBeComposited(layer))
        return false;

    return requiresCompositing(directReasonsForCompositing(layer)) || requiresCompositing(layer->compositingReasons()) || (inCompositingMode() && layer->isRootLayer());
}

bool RenderLayerCompositor::canBeComposited(const RenderLayer* layer) const
{
    // FIXME: We disable accelerated compositing for elements in a RenderFlowThread as it doesn't work properly.
    // See http://webkit.org/b/84900 to re-enable it.
    return m_hasAcceleratedCompositing && layer->isSelfPaintingLayer() && layer->renderer()->flowThreadState() == RenderObject::NotInsideFlowThread;
}

bool RenderLayerCompositor::requiresOwnBackingStore(const RenderLayer* layer, const RenderLayer* compositingAncestorLayer) const
{
    RenderObject* renderer = layer->renderer();
    if (compositingAncestorLayer
        && !(compositingAncestorLayer->backing()->graphicsLayer()->drawsContent()
            || compositingAncestorLayer->backing()->paintsIntoCompositedAncestor()))
        return true;

    if (layer->isRootLayer()
        || layer->transform() // note: excludes perspective and transformStyle3D.
        || requiresCompositingForVideo(renderer)
        || requiresCompositingForCanvas(renderer)
        || requiresCompositingForPlugin(renderer)
        || requiresCompositingForFrame(renderer)
        || requiresCompositingForBackfaceVisibilityHidden(renderer)
        || requiresCompositingForAnimation(renderer)
        || requiresCompositingForTransition(renderer)
        || requiresCompositingForFilters(renderer)
        || requiresCompositingForBlending(renderer)
        || requiresCompositingForPosition(renderer, layer)
        || requiresCompositingForOverflowScrolling(layer)
        || renderer->isTransparent()
        || renderer->hasMask()
        || renderer->hasReflection()
        || renderer->hasFilter())
        return true;

    CompositingReasons indirectReasonsThatNeedBacking = CompositingReasonOverlap
        | CompositingReasonAssumedOverlap
        | CompositingReasonNegativeZIndexChildren
        | CompositingReasonTransformWithCompositedDescendants
        | CompositingReasonOpacityWithCompositedDescendants
        | CompositingReasonMaskWithCompositedDescendants
        | CompositingReasonFilterWithCompositedDescendants
        | CompositingReasonBlendingWithCompositedDescendants
        | CompositingReasonPreserve3D; // preserve-3d has to create backing store to ensure that 3d-transformed elements intersect.
    return layer->compositingReasons() & indirectReasonsThatNeedBacking;
}

CompositingReasons RenderLayerCompositor::directReasonsForCompositing(const RenderLayer* layer) const
{
    RenderObject* renderer = layer->renderer();
    CompositingReasons directReasons = CompositingReasonNone;

    if (requiresCompositingForTransform(renderer))
        directReasons |= CompositingReason3DTransform;

    // Only zero or one of the following conditions will be true for a given RenderLayer.
    if (requiresCompositingForVideo(renderer))
        directReasons |= CompositingReasonVideo;
    else if (requiresCompositingForCanvas(renderer))
        directReasons |= CompositingReasonCanvas;
    else if (requiresCompositingForPlugin(renderer))
        directReasons |= CompositingReasonPlugin;
    else if (requiresCompositingForFrame(renderer))
        directReasons |= CompositingReasonIFrame;
    
    if (requiresCompositingForBackfaceVisibilityHidden(renderer))
        directReasons |= CompositingReasonBackfaceVisibilityHidden;

    if (requiresCompositingForAnimation(renderer))
        directReasons |= CompositingReasonAnimation;

    if (requiresCompositingForTransition(renderer))
        directReasons |= CompositingReasonAnimation;

    if (requiresCompositingForFilters(renderer))
        directReasons |= CompositingReasonFilters;

    if (requiresCompositingForPosition(renderer, layer))
        directReasons |= renderer->style()->position() == FixedPosition ? CompositingReasonPositionFixed : CompositingReasonPositionSticky;

    if (requiresCompositingForOverflowScrolling(layer))
        directReasons |= CompositingReasonOverflowScrollingTouch;

    if (requiresCompositingForBlending(renderer))
        directReasons |= CompositingReasonBlending;

    return directReasons;
}

CompositingReasons RenderLayerCompositor::reasonsForCompositing(const RenderLayer* layer) const
{
    CompositingReasons reasons = CompositingReasonNone;

    if (!layer || !layer->isComposited())
        return reasons;

    return layer->compositingReasons();
}

#if !LOG_DISABLED
const char* RenderLayerCompositor::logReasonsForCompositing(const RenderLayer* layer)
{
    CompositingReasons reasons = reasonsForCompositing(layer);

    if (reasons & CompositingReason3DTransform)
        return "3D transform";

    if (reasons & CompositingReasonVideo)
        return "video";

    if (reasons & CompositingReasonCanvas)
        return "canvas";

    if (reasons & CompositingReasonPlugin)
        return "plugin";

    if (reasons & CompositingReasonIFrame)
        return "iframe";
    
    if (reasons & CompositingReasonBackfaceVisibilityHidden)
        return "backface-visibility: hidden";

    if (reasons & CompositingReasonClipsCompositingDescendants)
        return "clips compositing descendants";

    if (reasons & CompositingReasonAnimation)
        return "animation";

    if (reasons & CompositingReasonFilters)
        return "filters";

    if (reasons & CompositingReasonPositionFixed)
        return "position: fixed";

    if (reasons & CompositingReasonPositionSticky)
        return "position: sticky";

    if (reasons & CompositingReasonOverflowScrollingTouch)
        return "-webkit-overflow-scrolling: touch";

    if (reasons & CompositingReasonAssumedOverlap)
        return "stacking";

    if (reasons & CompositingReasonOverlap)
        return "overlap";

    if (reasons & CompositingReasonNegativeZIndexChildren)
        return "negative z-index children";

    if (reasons & CompositingReasonTransformWithCompositedDescendants)
        return "transform with composited descendants";

    if (reasons & CompositingReasonOpacityWithCompositedDescendants)
        return "opacity with composited descendants";

    if (reasons & CompositingReasonMaskWithCompositedDescendants)
        return "mask with composited descendants";

    if (reasons & CompositingReasonReflectionWithCompositedDescendants)
        return "reflection with composited descendants";

    if (reasons & CompositingReasonFilterWithCompositedDescendants)
        return "filter with composited descendants";
            
    if (reasons & CompositingReasonBlendingWithCompositedDescendants)
        return "blending with composited descendants";

    if (reasons & CompositingReasonPerspective)
        return "perspective";

    if (reasons & CompositingReasonPreserve3D)
        return "preserve-3d";

    if (reasons & CompositingReasonRoot)
        return "root";

    return "";
}
#endif

// Return true if the given layer has some ancestor in the RenderLayer hierarchy that clips,
// up to the enclosing compositing ancestor. This is required because compositing layers are parented
// according to the z-order hierarchy, yet clipping goes down the renderer hierarchy.
// Thus, a RenderLayer can be clipped by a RenderLayer that is an ancestor in the renderer hierarchy,
// but a sibling in the z-order hierarchy.
bool RenderLayerCompositor::clippedByAncestor(RenderLayer* layer) const
{
    if (!layer->isComposited() || !layer->parent())
        return false;

    RenderLayer* compositingAncestor = layer->ancestorCompositingLayer();
    if (!compositingAncestor)
        return false;

    // If the compositingAncestor clips, that will be taken care of by clipsCompositingDescendants(),
    // so we only care about clipping between its first child that is our ancestor (the computeClipRoot),
    // and layer.
    RenderLayer* computeClipRoot = 0;
    RenderLayer* curr = layer;
    while (curr) {
        RenderLayer* next = curr->parent();
        if (next == compositingAncestor) {
            computeClipRoot = curr;
            break;
        }
        curr = next;
    }
    
    if (!computeClipRoot || computeClipRoot == layer)
        return false;

    return layer->backgroundClipRect(RenderLayer::ClipRectsContext(computeClipRoot, 0, TemporaryClipRects)).rect() != PaintInfo::infiniteRect(); // FIXME: Incorrect for CSS regions.
}

// Return true if the given layer is a stacking context and has compositing child
// layers that it needs to clip. In this case we insert a clipping GraphicsLayer
// into the hierarchy between this layer and its children in the z-order hierarchy.
bool RenderLayerCompositor::clipsCompositingDescendants(const RenderLayer* layer) const
{
    return layer->hasCompositingDescendant() && layer->renderer()->hasClipOrOverflowClip();
}

bool RenderLayerCompositor::requiresCompositingForScrollableFrame() const
{
    // Need this done first to determine overflow.
    ASSERT(!m_renderView->needsLayout());
    if (isMainFrame())
        return false;

    if (!(m_compositingTriggers & ChromeClient::ScrollableInnerFrameTrigger))
        return false;

    FrameView* frameView = m_renderView->frameView();
    return frameView->isScrollable();
}

bool RenderLayerCompositor::requiresCompositingForTransform(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::ThreeDTransformTrigger))
        return false;

    RenderStyle* style = renderer->style();
    // Note that we ask the renderer if it has a transform, because the style may have transforms,
    // but the renderer may be an inline that doesn't suppport them.
    return renderer->hasTransform() && style->transform().has3DOperation();
}

bool RenderLayerCompositor::requiresCompositingForVideo(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::VideoTrigger))
        return false;

    if (renderer->isVideo()) {
        RenderVideo* video = toRenderVideo(renderer);
        return video->shouldDisplayVideo() && canAccelerateVideoRendering(video);
    }
    return false;
}

bool RenderLayerCompositor::requiresCompositingForCanvas(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::CanvasTrigger))
        return false;

    if (renderer->isCanvas()) {
        HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(renderer->node());
#if USE(COMPOSITING_FOR_SMALL_CANVASES)
        bool isCanvasLargeEnoughToForceCompositing = true;
#else
        bool isCanvasLargeEnoughToForceCompositing = canvas->size().area() >= canvasAreaThresholdRequiringCompositing;
#endif
        return canvas->renderingContext() && canvas->renderingContext()->isAccelerated() && (canvas->renderingContext()->is3d() || isCanvasLargeEnoughToForceCompositing);
    }
    return false;
}

bool RenderLayerCompositor::requiresCompositingForPlugin(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::PluginTrigger))
        return false;

    bool composite = renderer->isEmbeddedObject() && toRenderEmbeddedObject(renderer)->allowsAcceleratedCompositing();
    if (!composite)
        return false;

    m_reevaluateCompositingAfterLayout = true;
    
    RenderWidget* pluginRenderer = toRenderWidget(renderer);
    // If we can't reliably know the size of the plugin yet, don't change compositing state.
    if (pluginRenderer->needsLayout())
        return pluginRenderer->hasLayer() && pluginRenderer->layer()->isComposited();

    // Don't go into compositing mode if height or width are zero, or size is 1x1.
    IntRect contentBox = pixelSnappedIntRect(pluginRenderer->contentBoxRect());
    return contentBox.height() * contentBox.width() > 1;
}

bool RenderLayerCompositor::requiresCompositingForFrame(RenderObject* renderer) const
{
    if (!renderer->isRenderPart())
        return false;
    
    RenderPart* frameRenderer = toRenderPart(renderer);

    if (!frameRenderer->requiresAcceleratedCompositing())
        return false;

    m_reevaluateCompositingAfterLayout = true;

    RenderLayerCompositor* innerCompositor = frameContentsCompositor(frameRenderer);
    if (!innerCompositor)
        return false;

    // If we can't reliably know the size of the iframe yet, don't change compositing state.
    if (renderer->needsLayout())
        return frameRenderer->hasLayer() && frameRenderer->layer()->isComposited();
    
    // Don't go into compositing mode if height or width are zero.
    IntRect contentBox = pixelSnappedIntRect(frameRenderer->contentBoxRect());
    return contentBox.height() * contentBox.width() > 0;
}

bool RenderLayerCompositor::requiresCompositingForBackfaceVisibilityHidden(RenderObject* renderer) const
{
    return canRender3DTransforms() && renderer->style()->backfaceVisibility() == BackfaceVisibilityHidden;
}

bool RenderLayerCompositor::requiresCompositingForAnimation(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::AnimationTrigger))
        return false;

    if (AnimationController* animController = renderer->animation()) {
        return (animController->isRunningAnimationOnRenderer(renderer, CSSPropertyOpacity) && inCompositingMode())
            // <rdar://problem/10907251> - WebKit2 doesn't support CA animations of CI filters on Lion and below
            || animController->isRunningAnimationOnRenderer(renderer, CSSPropertyWebkitFilter)
            || animController->isRunningAnimationOnRenderer(renderer, CSSPropertyWebkitTransform);
    }
    return false;
}

bool RenderLayerCompositor::requiresCompositingForTransition(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::AnimationTrigger))
        return false;

    if (Settings* settings = m_renderView->document()->settings()) {
        if (!settings->acceleratedCompositingForTransitionEnabled())
            return false;
    }

    return renderer->style()->transitionForProperty(CSSPropertyOpacity)
        || renderer->style()->transitionForProperty(CSSPropertyWebkitFilter)
        || renderer->style()->transitionForProperty(CSSPropertyWebkitTransform);
}

CompositingReasons RenderLayerCompositor::subtreeReasonsForCompositing(RenderObject* renderer, bool hasCompositedDescendants, bool has3DTransformedDescendants) const
{
    CompositingReasons subtreeReasons = CompositingReasonNone;

    // FIXME: this seems to be a potentially different layer than the layer for which this was called. May not be an error, but is very confusing.
    RenderLayer* layer = toRenderBoxModelObject(renderer)->layer();

    // When a layer has composited descendants, some effects, like 2d transforms, filters, masks etc must be implemented
    // via compositing so that they also apply to those composited descdendants.
    if (hasCompositedDescendants) {
        if (layer->transform())
            subtreeReasons |= CompositingReasonTransformWithCompositedDescendants;

        // If the implementation of createsGroup changes, we need to be aware of that in this part of code.
        ASSERT((renderer->isTransparent() || renderer->hasMask() || renderer->hasFilter() || renderer->hasBlendMode()) == renderer->createsGroup());
        if (renderer->isTransparent())
            subtreeReasons |= CompositingReasonOpacityWithCompositedDescendants;
        if (renderer->hasMask())
            subtreeReasons |= CompositingReasonMaskWithCompositedDescendants;
        if (renderer->hasFilter())
            subtreeReasons |= CompositingReasonFilterWithCompositedDescendants;
        if (renderer->hasBlendMode())
            subtreeReasons |= CompositingReasonBlendingWithCompositedDescendants;

        if (renderer->hasReflection())
            subtreeReasons |= CompositingReasonReflectionWithCompositedDescendants;

        if (renderer->hasClipOrOverflowClip())
            subtreeReasons |= CompositingReasonClipsCompositingDescendants;
    }


    // A layer with preserve-3d or perspective only needs to be composited if there are descendant layers that
    // will be affected by the preserve-3d or perspective.
    if (has3DTransformedDescendants) {
        if (renderer->style()->transformStyle3D() == TransformStyle3DPreserve3D)
            subtreeReasons |= CompositingReasonPreserve3D;
    
        if (renderer->style()->hasPerspective())
            subtreeReasons |= CompositingReasonPerspective;
    }

    return subtreeReasons;
}

bool RenderLayerCompositor::requiresCompositingForFilters(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::FilterTrigger))
        return false;

    return renderer->hasFilter();
}

bool RenderLayerCompositor::requiresCompositingForBlending(RenderObject* renderer) const
{
    return renderer->hasBlendMode();
}

bool RenderLayerCompositor::requiresCompositingForPosition(RenderObject* renderer, const RenderLayer* layer, RenderLayer::ViewportConstrainedNotCompositedReason* viewportConstrainedNotCompositedReason) const
{
    // position:fixed elements that create their own stacking context (e.g. have an explicit z-index,
    // opacity, transform) can get their own composited layer. A stacking context is required otherwise
    // z-index and clipping will be broken.
    if (!renderer->isPositioned())
        return false;
    
    EPosition position = renderer->style()->position();
    bool isFixed = renderer->isOutOfFlowPositioned() && position == FixedPosition;
    if (isFixed && !layer->isStackingContainer())
        return false;
    
    bool isSticky = renderer->isInFlowPositioned() && position == StickyPosition;
    if (!isFixed && !isSticky)
        return false;

    // FIXME: acceleratedCompositingForFixedPositionEnabled should probably be renamed acceleratedCompositingForViewportConstrainedPositionEnabled().
    if (Settings* settings = m_renderView->document()->settings()) {
        if (!settings->acceleratedCompositingForFixedPositionEnabled())
            return false;
    }

    if (isSticky)
        return true;

    RenderObject* container = renderer->container();
    // If the renderer is not hooked up yet then we have to wait until it is.
    if (!container) {
        m_reevaluateCompositingAfterLayout = true;
        return false;
    }

    // Don't promote fixed position elements that are descendants of a non-view container, e.g. transformed elements.
    // They will stay fixed wrt the container rather than the enclosing frame.
    if (container != m_renderView) {
        if (viewportConstrainedNotCompositedReason)
            *viewportConstrainedNotCompositedReason = RenderLayer::NotCompositedForNonViewContainer;
        return false;
    }
    
    // If the fixed-position element does not have any scrollable ancestor between it and
    // its container, then we do not need to spend compositor resources for it. Start by
    // assuming we can opt-out (i.e. no scrollable ancestor), and refine the answer below.
    bool hasScrollableAncestor = false;

    // The FrameView has the scrollbars associated with the top level viewport, so we have to
    // check the FrameView in addition to the hierarchy of ancestors.
    FrameView* frameView = m_renderView->frameView();
    if (frameView && frameView->isScrollable())
        hasScrollableAncestor = true;

    RenderLayer* ancestor = layer->parent();
    while (ancestor && !hasScrollableAncestor) {
        if (frameView->containsScrollableArea(ancestor))
            hasScrollableAncestor = true;
        if (ancestor->renderer() == m_renderView)
            break;
        ancestor = ancestor->parent();
    }

    if (!hasScrollableAncestor) {
        if (viewportConstrainedNotCompositedReason)
            *viewportConstrainedNotCompositedReason = RenderLayer::NotCompositedForUnscrollableAncestors;
        return false;
    }

    // Subsequent tests depend on layout. If we can't tell now, just keep things the way they are until layout is done.
    if (!m_inPostLayoutUpdate) {
        m_reevaluateCompositingAfterLayout = true;
        return layer->isComposited();
    }

    bool paintsContent = layer->isVisuallyNonEmpty() || layer->hasVisibleDescendant();
    if (!paintsContent) {
        if (viewportConstrainedNotCompositedReason)
            *viewportConstrainedNotCompositedReason = RenderLayer::NotCompositedForNoVisibleContent;
        return false;
    }

    // Fixed position elements that are invisible in the current view don't get their own layer.
    if (FrameView* frameView = m_renderView->frameView()) {
        LayoutRect viewBounds = frameView->viewportConstrainedVisibleContentRect();
        LayoutRect layerBounds = layer->calculateLayerBounds(rootRenderLayer(), 0, RenderLayer::DefaultCalculateLayerBoundsFlags
            | RenderLayer::ExcludeHiddenDescendants | RenderLayer::DontConstrainForMask | RenderLayer::IncludeCompositedDescendants);
        if (!viewBounds.intersects(enclosingIntRect(layerBounds))) {
            if (viewportConstrainedNotCompositedReason)
                *viewportConstrainedNotCompositedReason = RenderLayer::NotCompositedForBoundsOutOfView;
            return false;
        }
    }
    
    return true;
}

bool RenderLayerCompositor::requiresCompositingForOverflowScrolling(const RenderLayer* layer) const
{
    return layer->needsCompositedScrolling();
}

bool RenderLayerCompositor::isRunningAcceleratedTransformAnimation(RenderObject* renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::AnimationTrigger))
        return false;

    if (AnimationController* animController = renderer->animation())
        return animController->isRunningAnimationOnRenderer(renderer, CSSPropertyWebkitTransform);

    return false;
}

// If an element has negative z-index children, those children render in front of the 
// layer background, so we need an extra 'contents' layer for the foreground of the layer
// object.
bool RenderLayerCompositor::needsContentsCompositingLayer(const RenderLayer* layer) const
{
    return layer->hasNegativeZOrderList();
}

static void paintScrollbar(Scrollbar* scrollbar, GraphicsContext& context, const IntRect& clip)
{
    if (!scrollbar)
        return;

    context.save();
    const IntRect& scrollbarRect = scrollbar->frameRect();
    context.translate(-scrollbarRect.x(), -scrollbarRect.y());
    IntRect transformedClip = clip;
    transformedClip.moveBy(scrollbarRect.location());
    scrollbar->paint(&context, transformedClip);
    context.restore();
}

void RenderLayerCompositor::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& context, GraphicsLayerPaintingPhase, const IntRect& clip)
{
    if (graphicsLayer == layerForHorizontalScrollbar())
        paintScrollbar(m_renderView->frameView()->horizontalScrollbar(), context, clip);
    else if (graphicsLayer == layerForVerticalScrollbar())
        paintScrollbar(m_renderView->frameView()->verticalScrollbar(), context, clip);
    else if (graphicsLayer == layerForScrollCorner()) {
        const IntRect& scrollCorner = m_renderView->frameView()->scrollCornerRect();
        context.save();
        context.translate(-scrollCorner.x(), -scrollCorner.y());
        IntRect transformedClip = clip;
        transformedClip.moveBy(scrollCorner.location());
        m_renderView->frameView()->paintScrollCorner(&context, transformedClip);
        context.restore();
#if ENABLE(RUBBER_BANDING)
    } else if (graphicsLayer == layerForOverhangAreas()) {
        ScrollView* view = m_renderView->frameView();
        view->calculateAndPaintOverhangAreas(&context, clip);
#endif
    }
}

bool RenderLayerCompositor::supportsFixedRootBackgroundCompositing() const
{
    return false;  // FIXME: Return true if this is supported when implemented.
}

bool RenderLayerCompositor::needsFixedRootBackgroundLayer(const RenderLayer* layer) const
{
    if (layer != m_renderView->layer())
        return false;

    return supportsFixedRootBackgroundCompositing() && m_renderView->rootBackgroundIsEntirelyFixed();
}

GraphicsLayer* RenderLayerCompositor::fixedRootBackgroundLayer() const
{
    // Get the fixed root background from the RenderView layer's backing.
    RenderLayer* viewLayer = m_renderView->layer();
    if (!viewLayer)
        return 0;
    
    if (viewLayer->isComposited() && viewLayer->backing()->backgroundLayerPaintsFixedRootBackground())
        return viewLayer->backing()->backgroundLayer();

    return 0;
}

static void resetTrackedRepaintRectsRecursive(GraphicsLayer* graphicsLayer)
{
    if (!graphicsLayer)
        return;

    graphicsLayer->resetTrackedRepaints();

    for (size_t i = 0; i < graphicsLayer->children().size(); ++i)
        resetTrackedRepaintRectsRecursive(graphicsLayer->children()[i]);

    if (GraphicsLayer* replicaLayer = graphicsLayer->replicaLayer())
        resetTrackedRepaintRectsRecursive(replicaLayer);

    if (GraphicsLayer* maskLayer = graphicsLayer->maskLayer())
        resetTrackedRepaintRectsRecursive(maskLayer);
}

void RenderLayerCompositor::resetTrackedRepaintRects()
{
    if (GraphicsLayer* rootLayer = rootGraphicsLayer())
        resetTrackedRepaintRectsRecursive(rootLayer);
}

void RenderLayerCompositor::setTracksRepaints(bool tracksRepaints)
{
    m_isTrackingRepaints = tracksRepaints;
}

bool RenderLayerCompositor::isTrackingRepaints() const
{
    return m_isTrackingRepaints;
}

void RenderLayerCompositor::didCommitChangesForLayer(const GraphicsLayer*) const
{
    // Nothing to do here yet.
}

static bool shouldCompositeOverflowControls(FrameView* view)
{
    if (Page* page = view->frame()->page()) {
        if (ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator())
            if (scrollingCoordinator->coordinatesScrollingForFrameView(view))
                return true;
    }

    return true;
}

bool RenderLayerCompositor::requiresHorizontalScrollbarLayer() const
{
    FrameView* view = m_renderView->frameView();
    return shouldCompositeOverflowControls(view) && view->horizontalScrollbar();
}

bool RenderLayerCompositor::requiresVerticalScrollbarLayer() const
{
    FrameView* view = m_renderView->frameView();
    return shouldCompositeOverflowControls(view) && view->verticalScrollbar();
}

bool RenderLayerCompositor::requiresScrollCornerLayer() const
{
    FrameView* view = m_renderView->frameView();
    return shouldCompositeOverflowControls(view) && view->isScrollCornerVisible();
}

#if ENABLE(RUBBER_BANDING)
bool RenderLayerCompositor::requiresOverhangAreasLayer() const
{
    // We don't want a layer if this is a subframe.
    if (!isMainFrame())
        return false;

    // We do want a layer if we have a scrolling coordinator and can scroll.
    if (scrollingCoordinator() && m_renderView->frameView()->hasOpaqueBackground() && !m_renderView->frameView()->prohibitsScrolling())
        return true;

    // Chromium always wants a layer.
    return true;
}

bool RenderLayerCompositor::requiresContentShadowLayer() const
{
    // We don't want a layer if this is a subframe.
    if (!isMainFrame())
        return false;

    return false;
}

GraphicsLayer* RenderLayerCompositor::updateLayerForTopOverhangArea(bool wantsLayer)
{
    if (!isMainFrame())
        return 0;

    if (!wantsLayer) {
        if (m_layerForTopOverhangArea) {
            m_layerForTopOverhangArea->removeFromParent();
            m_layerForTopOverhangArea = nullptr;
        }
        return 0;
    }

    if (!m_layerForTopOverhangArea) {
        m_layerForTopOverhangArea = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
        m_layerForTopOverhangArea->setName("top overhang area");
#endif
        m_scrollLayer->addChildBelow(m_layerForTopOverhangArea.get(), m_rootContentLayer.get());
    }

    return m_layerForTopOverhangArea.get();
}

GraphicsLayer* RenderLayerCompositor::updateLayerForBottomOverhangArea(bool wantsLayer)
{
    if (!isMainFrame())
        return 0;

    if (!wantsLayer) {
        if (m_layerForBottomOverhangArea) {
            m_layerForBottomOverhangArea->removeFromParent();
            m_layerForBottomOverhangArea = nullptr;
        }
        return 0;
    }

    if (!m_layerForBottomOverhangArea) {
        m_layerForBottomOverhangArea = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
        m_layerForBottomOverhangArea->setName("bottom overhang area");
#endif
        m_scrollLayer->addChildBelow(m_layerForBottomOverhangArea.get(), m_rootContentLayer.get());
    }

    m_layerForBottomOverhangArea->setPosition(FloatPoint(0, m_rootContentLayer->size().height()));
    return m_layerForBottomOverhangArea.get();
}

#endif

bool RenderLayerCompositor::viewHasTransparentBackground(Color* backgroundColor) const
{
    FrameView* frameView = m_renderView->frameView();
    if (frameView->isTransparent()) {
        if (backgroundColor)
            *backgroundColor = Color(); // Return an invalid color.
        return true;
    }

    Color documentBackgroundColor = frameView->documentBackgroundColor();
    if (!documentBackgroundColor.isValid())
        documentBackgroundColor = Color::white;

    if (backgroundColor)
        *backgroundColor = documentBackgroundColor;
        
    return documentBackgroundColor.hasAlpha();
}

void RenderLayerCompositor::updateOverflowControlsLayers()
{
#if ENABLE(RUBBER_BANDING)
    if (requiresOverhangAreasLayer()) {
        if (!m_layerForOverhangAreas) {
            m_layerForOverhangAreas = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
            m_layerForOverhangAreas->setName("overhang areas");
#endif
            m_layerForOverhangAreas->setDrawsContent(false);
            m_layerForOverhangAreas->setSize(m_renderView->frameView()->frameRect().size());

            // We want the overhang areas layer to be positioned below the frame contents,
            // so insert it below the clip layer.
            m_overflowControlsHostLayer->addChildBelow(m_layerForOverhangAreas.get(), m_clipLayer.get());
        }
    } else if (m_layerForOverhangAreas) {
        m_layerForOverhangAreas->removeFromParent();
        m_layerForOverhangAreas = nullptr;
    }

    if (requiresContentShadowLayer()) {
        if (!m_contentShadowLayer) {
            m_contentShadowLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
            m_contentShadowLayer->setName("content shadow");
#endif
            m_contentShadowLayer->setSize(m_rootContentLayer->size());
            m_contentShadowLayer->setPosition(m_rootContentLayer->position());
            ScrollbarTheme::theme()->setUpContentShadowLayer(m_contentShadowLayer.get());

            m_scrollLayer->addChildBelow(m_contentShadowLayer.get(), m_rootContentLayer.get());
        }
    } else if (m_contentShadowLayer) {
        m_contentShadowLayer->removeFromParent();
        m_contentShadowLayer = nullptr;
    }
#endif

    if (requiresHorizontalScrollbarLayer()) {
        if (!m_layerForHorizontalScrollbar) {
            m_layerForHorizontalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), this);
            m_layerForHorizontalScrollbar->setShowDebugBorder(m_showDebugBorders);
#ifndef NDEBUG
            m_layerForHorizontalScrollbar->setName("horizontal scrollbar");
#endif
            m_overflowControlsHostLayer->addChild(m_layerForHorizontalScrollbar.get());

            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView->frameView(), HorizontalScrollbar);
        }
    } else if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar->removeFromParent();
        m_layerForHorizontalScrollbar = nullptr;

        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView->frameView(), HorizontalScrollbar);
    }

    if (requiresVerticalScrollbarLayer()) {
        if (!m_layerForVerticalScrollbar) {
            m_layerForVerticalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), this);
            m_layerForVerticalScrollbar->setShowDebugBorder(m_showDebugBorders);
#ifndef NDEBUG
            m_layerForVerticalScrollbar->setName("vertical scrollbar");
#endif
            m_overflowControlsHostLayer->addChild(m_layerForVerticalScrollbar.get());

            if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView->frameView(), VerticalScrollbar);
        }
    } else if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar->removeFromParent();
        m_layerForVerticalScrollbar = nullptr;

        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView->frameView(), VerticalScrollbar);
    }

    if (requiresScrollCornerLayer()) {
        if (!m_layerForScrollCorner) {
            m_layerForScrollCorner = GraphicsLayer::create(graphicsLayerFactory(), this);
            m_layerForScrollCorner->setShowDebugBorder(m_showDebugBorders);
#ifndef NDEBUG
            m_layerForScrollCorner->setName("scroll corner");
#endif
            m_overflowControlsHostLayer->addChild(m_layerForScrollCorner.get());
        }
    } else if (m_layerForScrollCorner) {
        m_layerForScrollCorner->removeFromParent();
        m_layerForScrollCorner = nullptr;
    }

    m_renderView->frameView()->positionScrollbarLayers();
}

void RenderLayerCompositor::ensureRootLayer()
{
    RootLayerAttachment expectedAttachment = isMainFrame() ? RootLayerAttachedViaChromeClient : RootLayerAttachedViaEnclosingFrame;
    if (expectedAttachment == m_rootLayerAttachment)
         return;

    if (!m_rootContentLayer) {
        m_rootContentLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
        m_rootContentLayer->setName("content root");
#endif
        IntRect overflowRect = m_renderView->pixelSnappedLayoutOverflowRect();
        m_rootContentLayer->setSize(FloatSize(overflowRect.maxX(), overflowRect.maxY()));
        m_rootContentLayer->setPosition(FloatPoint());

        // Need to clip to prevent transformed content showing outside this frame
        m_rootContentLayer->setMasksToBounds(true);
    }

    if (!m_overflowControlsHostLayer) {
        ASSERT(!m_scrollLayer);
        ASSERT(!m_clipLayer);

        // Create a layer to host the clipping layer and the overflow controls layers.
        m_overflowControlsHostLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
        m_overflowControlsHostLayer->setName("overflow controls host");
#endif

        // Create a clipping layer if this is an iframe
        m_clipLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
        m_clipLayer->setName("frame clipping");
#endif
        m_clipLayer->setMasksToBounds(true);

        m_scrollLayer = GraphicsLayer::create(graphicsLayerFactory(), this);
#ifndef NDEBUG
        m_scrollLayer->setName("frame scrolling");
#endif
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->setLayerIsContainerForFixedPositionLayers(m_scrollLayer.get(), true);

        // Hook them up
        m_overflowControlsHostLayer->addChild(m_clipLayer.get());
        m_clipLayer->addChild(m_scrollLayer.get());
        m_scrollLayer->addChild(m_rootContentLayer.get());

        frameViewDidChangeSize();
        frameViewDidScroll();
    }

    // Check to see if we have to change the attachment
    if (m_rootLayerAttachment != RootLayerUnattached)
        detachRootLayer();

    attachRootLayer(expectedAttachment);
}

void RenderLayerCompositor::destroyRootLayer()
{
    if (!m_rootContentLayer)
        return;

    detachRootLayer();

#if ENABLE(RUBBER_BANDING)
    if (m_layerForOverhangAreas) {
        m_layerForOverhangAreas->removeFromParent();
        m_layerForOverhangAreas = nullptr;
    }
#endif

    if (m_layerForHorizontalScrollbar) {
        m_layerForHorizontalScrollbar->removeFromParent();
        m_layerForHorizontalScrollbar = nullptr;
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView->frameView(), HorizontalScrollbar);
        if (Scrollbar* horizontalScrollbar = m_renderView->frameView()->verticalScrollbar())
            m_renderView->frameView()->invalidateScrollbar(horizontalScrollbar, IntRect(IntPoint(0, 0), horizontalScrollbar->frameRect().size()));
    }

    if (m_layerForVerticalScrollbar) {
        m_layerForVerticalScrollbar->removeFromParent();
        m_layerForVerticalScrollbar = nullptr;
        if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView->frameView(), VerticalScrollbar);
        if (Scrollbar* verticalScrollbar = m_renderView->frameView()->verticalScrollbar())
            m_renderView->frameView()->invalidateScrollbar(verticalScrollbar, IntRect(IntPoint(0, 0), verticalScrollbar->frameRect().size()));
    }

    if (m_layerForScrollCorner) {
        m_layerForScrollCorner = nullptr;
        m_renderView->frameView()->invalidateScrollCorner(m_renderView->frameView()->scrollCornerRect());
    }

    if (m_overflowControlsHostLayer) {
        m_overflowControlsHostLayer = nullptr;
        m_clipLayer = nullptr;
        m_scrollLayer = nullptr;
    }
    ASSERT(!m_scrollLayer);
    m_rootContentLayer = nullptr;
}

void RenderLayerCompositor::attachRootLayer(RootLayerAttachment attachment)
{
    if (!m_rootContentLayer)
        return;

    switch (attachment) {
        case RootLayerUnattached:
            ASSERT_NOT_REACHED();
            break;
        case RootLayerAttachedViaChromeClient: {
            Frame* frame = m_renderView->frameView()->frame();
            Page* page = frame ? frame->page() : 0;
            if (!page)
                return;

            page->chrome().client()->attachRootGraphicsLayer(frame, rootGraphicsLayer());
            break;
        }
        case RootLayerAttachedViaEnclosingFrame: {
            // The layer will get hooked up via RenderLayerBacking::updateGraphicsLayerConfiguration()
            // for the frame's renderer in the parent document.
            m_renderView->document()->ownerElement()->scheduleSyntheticStyleChange();
            break;
        }
    }

    m_rootLayerAttachment = attachment;
}

void RenderLayerCompositor::detachRootLayer()
{
    if (!m_rootContentLayer || m_rootLayerAttachment == RootLayerUnattached)
        return;

    switch (m_rootLayerAttachment) {
    case RootLayerAttachedViaEnclosingFrame: {
        // The layer will get unhooked up via RenderLayerBacking::updateGraphicsLayerConfiguration()
        // for the frame's renderer in the parent document.
        if (m_overflowControlsHostLayer)
            m_overflowControlsHostLayer->removeFromParent();
        else
            m_rootContentLayer->removeFromParent();

        if (HTMLFrameOwnerElement* ownerElement = m_renderView->document()->ownerElement())
            ownerElement->scheduleSyntheticStyleChange();
        break;
    }
    case RootLayerAttachedViaChromeClient: {
        Frame* frame = m_renderView->frameView()->frame();
        Page* page = frame ? frame->page() : 0;
        if (!page)
            return;

        page->chrome().client()->attachRootGraphicsLayer(frame, 0);
    }
    break;
    case RootLayerUnattached:
        break;
    }

    m_rootLayerAttachment = RootLayerUnattached;
}

void RenderLayerCompositor::updateRootLayerAttachment()
{
    ensureRootLayer();
}

bool RenderLayerCompositor::isMainFrame() const
{
    return !m_renderView->document()->ownerElement();
}

// IFrames are special, because we hook compositing layers together across iframe boundaries
// when both parent and iframe content are composited. So when this frame becomes composited, we have
// to use a synthetic style change to get the iframes into RenderLayers in order to allow them to composite.
void RenderLayerCompositor::notifyIFramesOfCompositingChange()
{
    Frame* frame = m_renderView->frameView() ? m_renderView->frameView()->frame() : 0;
    if (!frame)
        return;

    for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->traverseNext(frame)) {
        if (child->document() && child->document()->ownerElement())
            child->document()->ownerElement()->scheduleSyntheticStyleChange();
    }
    
    // Compositing also affects the answer to RenderIFrame::requiresAcceleratedCompositing(), so 
    // we need to schedule a style recalc in our parent document.
    if (HTMLFrameOwnerElement* ownerElement = m_renderView->document()->ownerElement())
        ownerElement->scheduleSyntheticStyleChange();
}

bool RenderLayerCompositor::layerHas3DContent(const RenderLayer* layer) const
{
    const RenderStyle* style = layer->renderer()->style();

    if (style && 
        (style->transformStyle3D() == TransformStyle3DPreserve3D ||
         style->hasPerspective() ||
         style->transform().has3DOperation()))
        return true;

    const_cast<RenderLayer*>(layer)->updateLayerListsIfNeeded();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(const_cast<RenderLayer*>(layer));
#endif

    if (layer->isStackingContainer()) {
        if (Vector<RenderLayer*>* negZOrderList = layer->negZOrderList()) {
            size_t listSize = negZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = negZOrderList->at(i);
                if (layerHas3DContent(curLayer))
                    return true;
            }
        }

        if (Vector<RenderLayer*>* posZOrderList = layer->posZOrderList()) {
            size_t listSize = posZOrderList->size();
            for (size_t i = 0; i < listSize; ++i) {
                RenderLayer* curLayer = posZOrderList->at(i);
                if (layerHas3DContent(curLayer))
                    return true;
            }
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = layer->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);
            if (layerHas3DContent(curLayer))
                return true;
        }
    }
    return false;
}

static bool isRootmostFixedOrStickyLayer(RenderLayer* layer)
{
    if (layer->renderer()->isStickyPositioned())
        return true;

    if (layer->renderer()->style()->position() != FixedPosition)
        return false;

    for (RenderLayer* stackingContainer = layer->ancestorStackingContainer(); stackingContainer; stackingContainer = stackingContainer->ancestorStackingContainer()) {
        if (stackingContainer->isComposited() && stackingContainer->renderer()->style()->position() == FixedPosition)
            return false;
    }

    return true;
}

void RenderLayerCompositor::updateViewportConstraintStatus(RenderLayer* layer)
{
    if (isRootmostFixedOrStickyLayer(layer))
        addViewportConstrainedLayer(layer);
    else
        removeViewportConstrainedLayer(layer);
}

void RenderLayerCompositor::addViewportConstrainedLayer(RenderLayer* layer)
{
    m_viewportConstrainedLayers.add(layer);
}

void RenderLayerCompositor::removeViewportConstrainedLayer(RenderLayer* layer)
{
    if (!m_viewportConstrainedLayers.contains(layer))
        return;

    m_viewportConstrainedLayers.remove(layer);
}

FixedPositionViewportConstraints RenderLayerCompositor::computeFixedViewportConstraints(RenderLayer* layer) const
{
    ASSERT(layer->isComposited());

    FrameView* frameView = m_renderView->frameView();
    LayoutRect viewportRect = frameView->viewportConstrainedVisibleContentRect();

    FixedPositionViewportConstraints constraints;

    GraphicsLayer* graphicsLayer = layer->backing()->graphicsLayer();

    constraints.setLayerPositionAtLastLayout(graphicsLayer->position());
    constraints.setViewportRectAtLastLayout(viewportRect);

    RenderStyle* style = layer->renderer()->style();
    if (!style->left().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeLeft);

    if (!style->right().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeRight);

    if (!style->top().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeTop);

    if (!style->bottom().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeBottom);

    // If left and right are auto, use left.
    if (style->left().isAuto() && style->right().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeLeft);

    // If top and bottom are auto, use top.
    if (style->top().isAuto() && style->bottom().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeTop);
        
    return constraints;
}

StickyPositionViewportConstraints RenderLayerCompositor::computeStickyViewportConstraints(RenderLayer* layer) const
{
    ASSERT(layer->isComposited());

    FrameView* frameView = m_renderView->frameView();
    LayoutRect viewportRect = frameView->viewportConstrainedVisibleContentRect();

    StickyPositionViewportConstraints constraints;

    RenderBoxModelObject* renderer = toRenderBoxModelObject(layer->renderer());

    renderer->computeStickyPositionConstraints(constraints, viewportRect);

    GraphicsLayer* graphicsLayer = layer->backing()->graphicsLayer();

    constraints.setLayerPositionAtLastLayout(graphicsLayer->position());
    constraints.setStickyOffsetAtLastLayout(renderer->stickyPositionOffset());

    return constraints;
}

ScrollingCoordinator* RenderLayerCompositor::scrollingCoordinator() const
{
    if (Page* page = this->page())
        return page->scrollingCoordinator();

    return 0;
}

GraphicsLayerFactory* RenderLayerCompositor::graphicsLayerFactory() const
{
    if (Page* page = this->page())
        return page->chrome().client()->graphicsLayerFactory();

    return 0;
}

Page* RenderLayerCompositor::page() const
{
    if (Frame* frame = m_renderView->frameView()->frame())
        return frame->page();
    
    return 0;
}

void RenderLayerCompositor::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, PlatformMemoryTypes::Rendering);
    info.addWeakPointer(m_renderView);
    info.addMember(m_rootContentLayer, "rootContentLayer");
    info.addMember(m_clipLayer, "clipLayer");
    info.addMember(m_scrollLayer, "scrollLayer");
    info.addMember(m_viewportConstrainedLayers, "viewportConstrainedLayers");
    info.addMember(m_overflowControlsHostLayer, "overflowControlsHostLayer");
    info.addMember(m_layerForHorizontalScrollbar, "layerForHorizontalScrollbar");
    info.addMember(m_layerForVerticalScrollbar, "layerForVerticalScrollbar");
    info.addMember(m_layerForScrollCorner, "layerForScrollCorner");
#if ENABLE(RUBBER_BANDING)
    info.addMember(m_layerForOverhangAreas, "layerForOverhangAreas");
    info.addMember(m_contentShadowLayer, "contentShadowLayer");
    info.addMember(m_layerForTopOverhangArea, "layerForTopOverhangArea");
    info.addMember(m_layerForBottomOverhangArea, "layerForBottomOverhangArea");
#endif
}

} // namespace WebCore
