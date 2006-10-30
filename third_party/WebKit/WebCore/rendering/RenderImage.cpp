/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "RenderImage.h"

#include "Document.h"
#include "GraphicsContext.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "RenderView.h"
#include "TextStyle.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderImage::RenderImage(Node* node)
    : RenderReplaced(node)
    , m_cachedImage(0)
    , m_isAnonymousImage(0)
{
    m_selectionState = SelectionNone;
    setIntrinsicWidth(0);
    setIntrinsicHeight(0);
    updateAltText();
}

RenderImage::~RenderImage()
{
    if (m_cachedImage)
        m_cachedImage->deref(this);
}

void RenderImage::setStyle(RenderStyle* newStyle)
{
    RenderReplaced::setStyle(newStyle);
    setShouldPaintBackgroundOrBorder(true);
}

void RenderImage::setContentObject(CachedResource* cachedResource)
{
    if (cachedResource && m_cachedImage != cachedResource) {
        if (m_cachedImage)
            m_cachedImage->deref(this);
        m_cachedImage = static_cast<CachedImage*>(cachedResource);
        if (m_cachedImage)
            m_cachedImage->ref(this);
    }
}

void RenderImage::setCachedImage(CachedImage* newImage)
{
    if (isAnonymousImage())
        return;

    if (m_cachedImage != newImage) {
        if (m_cachedImage)
            m_cachedImage->deref(this);
        m_cachedImage = newImage;
        if (m_cachedImage)
            m_cachedImage->ref(this);
        if (m_cachedImage->isErrorImage())
            imageChanged(m_cachedImage);
    }
}

void RenderImage::imageChanged(CachedImage* newImage)
{
    if (documentBeingDestroyed())
        return;

    if (newImage != m_cachedImage) {
        RenderReplaced::imageChanged(newImage);
        return;
    }

    bool imageSizeChanged = false;

    if (newImage->isErrorImage()) {
        int imageWidth = newImage->image()->width() + 4;
        int imageHeight = newImage->image()->height() + 4;

        // we have an alt and the user meant it (its not a text we invented)
        if (!m_altText.isEmpty()) {
            const Font& font = style()->font();
            imageWidth = max(imageWidth, min(font.width(TextRun(m_altText.characters(), m_altText.length())), 1024));
            imageHeight = max(imageHeight, min(font.height(), 256));
        }

        if (imageWidth != intrinsicWidth()) {
            setIntrinsicWidth(imageWidth);
            imageSizeChanged = true;
        }
        if (imageHeight != intrinsicHeight()) {
            setIntrinsicHeight(imageHeight);
            imageSizeChanged = true;
        }
    }

    bool ensureLayout = false;

    // Image dimensions have been changed, see what needs to be done
    if (newImage->imageSize().width() != intrinsicWidth() || newImage->imageSize().height() != intrinsicHeight() || imageSizeChanged) {
        if (!newImage->isErrorImage()) {
            setIntrinsicWidth(newImage->imageSize().width());
            setIntrinsicHeight(newImage->imageSize().height());
        }

        // In the case of generated image content using :before/:after, we might not be in the
        // render tree yet.  In that case, we don't need to worry about check for layout, since we'll get a
        // layout when we get added in to the render tree hierarchy later.
        if (containingBlock()) {
            // lets see if we need to relayout at all..
            int oldwidth = m_width;
            int oldheight = m_height;
            calcWidth();
            calcHeight();
    
            if (imageSizeChanged || m_width != oldwidth || m_height != oldheight)
                ensureLayout = true;

            m_width = oldwidth;
            m_height = oldheight;
        }
    }

    if (ensureLayout) {
        if (!selfNeedsLayout())
            setNeedsLayout(true);
        if (minMaxKnown())
            setMinMaxKnown(false);
    } else
        // FIXME: We always just do a complete repaint, since we always pass in the full image
        // rect at the moment anyway.
        repaintRectangle(IntRect(borderLeft() + paddingLeft(), borderTop() + paddingTop(), contentWidth(), contentHeight()));
}

void RenderImage::resetAnimation()
{
    if (m_cachedImage) {
        image()->resetAnimation();
        if (!needsLayout())
            repaint();
    }
}

void RenderImage::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (!shouldPaint(paintInfo, tx, ty))
        return;

    tx += m_x;
    ty += m_y;
        
    if (shouldPaintBackgroundOrBorder() && paintInfo.phase != PaintPhaseOutline && paintInfo.phase != PaintPhaseSelfOutline) 
        paintBoxDecorations(paintInfo, tx, ty);

    GraphicsContext* p = paintInfo.p;

    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && style()->outlineWidth() && style()->visibility() == VISIBLE)
        paintOutline(p, tx, ty, width(), height(), style());

    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection)
        return;

    if (!shouldPaintWithinRoot(paintInfo))
        return;
        
    bool isPrinting = document()->printing();
    bool drawSelectionTint = isSelected() && !isPrinting;
    if (paintInfo.phase == PaintPhaseSelection) {
        if (selectionState() == SelectionNone)
            return;
        drawSelectionTint = false;
    }
        
    int cWidth = contentWidth();
    int cHeight = contentHeight();
    int leftBorder = borderLeft();
    int topBorder = borderTop();
    int leftPad = paddingLeft();
    int topPad = paddingTop();

    if (isPrinting && !view()->printImages())
        return;

    if (!m_cachedImage || image()->isNull() || errorOccurred()) {
        if (paintInfo.phase == PaintPhaseSelection)
            return;

        if (cWidth > 2 && cHeight > 2) {
            if (!errorOccurred()) {
                p->setPen(Color::lightGray);
                p->setFillColor(Color::transparent);
                p->drawRect(IntRect(tx + leftBorder + leftPad, ty + topBorder + topPad, cWidth, cHeight));
            }

            bool errorPictureDrawn = false;
            int imageX = 0;
            int imageY = 0;
            int usableWidth = cWidth;
            int usableHeight = cHeight;

            if (errorOccurred() && !image()->isNull() && (usableWidth >= image()->width()) && (usableHeight >= image()->height())) {
                // Center the error image, accounting for border and padding.
                int centerX = (usableWidth - image()->width()) / 2;
                if (centerX < 0)
                    centerX = 0;
                int centerY = (usableHeight - image()->height()) / 2;
                if (centerY < 0)
                    centerY = 0;
                imageX = leftBorder + leftPad + centerX;
                imageY = topBorder + topPad + centerY;
                p->drawImage(image(), IntPoint(tx + imageX, ty + imageY));
                errorPictureDrawn = true;
            }

            if (!m_altText.isEmpty()) {
                DeprecatedString text = m_altText.deprecatedString();
                text.replace('\\', backslashAsCurrencySymbol());
                p->setFont(style()->font());
                p->setPen(style()->color());
                int ax = tx + leftBorder + leftPad;
                int ay = ty + topBorder + topPad;
                const Font& font = style()->font();
                int ascent = font.ascent();
                
                // Only draw the alt text if it'll fit within the content box,
                // and only if it fits above the error image.
                TextRun textRun(reinterpret_cast<const UChar*>(text.unicode()), text.length());
                int textWidth = font.width(textRun);
                if (errorPictureDrawn) {
                    if (usableWidth >= textWidth && font.height() <= imageY)
                        p->drawText(textRun, IntPoint(ax, ay + ascent));
                } else if (usableWidth >= textWidth && cHeight >= font.height())
                    p->drawText(textRun, IntPoint(ax, ay + ascent));
            }
        }
    } else if (m_cachedImage) {
#if PLATFORM(MAC)
        if (style()->highlight() != nullAtom && !paintInfo.p->paintingDisabled())
            paintCustomHighlight(tx - m_x, ty - m_y, style()->highlight(), true);
#endif

        IntRect rect(IntPoint(tx + leftBorder + leftPad, ty + topBorder + topPad), IntSize(cWidth, cHeight));

        HTMLImageElement* imageElt = (element() && element()->hasTagName(imgTag)) ? static_cast<HTMLImageElement*>(element()) : 0;
        CompositeOperator compositeOperator = imageElt ? imageElt->compositeOperator() : CompositeSourceOver;
        p->drawImage(image(), rect, compositeOperator);

        if (drawSelectionTint)
            p->fillRect(selectionRect(), selectionBackgroundColor());
    }
}

void RenderImage::layout()
{
    ASSERT(needsLayout());
    ASSERT(minMaxKnown());

    IntRect oldBounds;
    bool checkForRepaint = checkForRepaintDuringLayout();
    if (checkForRepaint)
        oldBounds = getAbsoluteRepaintRect();

    // minimum height
    m_height = m_cachedImage && m_cachedImage->isErrorImage() ? intrinsicHeight() : 0;

    calcWidth();
    calcHeight();

    if (checkForRepaint)
        repaintAfterLayoutIfNeeded(oldBounds, oldBounds);
    
    setNeedsLayout(false);
}

HTMLMapElement* RenderImage::imageMap()
{
    HTMLImageElement* i = element() && element()->hasTagName(imgTag) ? static_cast<HTMLImageElement*>(element()) : 0;
    return i ? i->document()->getImageMap(i->imageMap()) : 0;
}

bool RenderImage::nodeAtPoint(HitTestResult& result, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction)
{
    bool inside = RenderReplaced::nodeAtPoint(result, _x, _y, _tx, _ty, hitTestAction);

    if (inside && element()) {
        int tx = _tx + m_x;
        int ty = _ty + m_y;
        
        HTMLMapElement* map = imageMap();
        if (map) {
            // we're a client side image map
            inside = map->mapMouseEvent(_x - tx, _y - ty, IntSize(contentWidth(), contentHeight()), result);
            result.setInnerNonSharedNode(element());
        }
    }

    return inside;
}

void RenderImage::updateAltText()
{
    if (!element())
        return;

    if (element()->hasTagName(inputTag))
        m_altText = static_cast<HTMLInputElement*>(element())->altText();
    else if (element()->hasTagName(imgTag))
        m_altText = static_cast<HTMLImageElement*>(element())->altText();
}

bool RenderImage::isWidthSpecified() const
{
    switch (style()->width().type()) {
        case Fixed:
        case Percent:
            return true;
        default:
            return false;
    }
    assert(false);
    return false;
}

bool RenderImage::isHeightSpecified() const
{
    switch (style()->height().type()) {
        case Fixed:
        case Percent:
            return true;
        default:
            return false;
    }
    assert(false);
    return false;
}

int RenderImage::calcReplacedWidth() const
{
    int width;
    if (isWidthSpecified())
        width = calcReplacedWidthUsing(style()->width());
    else
        width = calcAspectRatioWidth();

    int minW = calcReplacedWidthUsing(style()->minWidth());
    int maxW = style()->maxWidth().value() == undefinedLength ? width : calcReplacedWidthUsing(style()->maxWidth());

    return max(minW, min(width, maxW));
}

int RenderImage::calcReplacedHeight() const
{
    int height;
    if (isHeightSpecified())
        height = calcReplacedHeightUsing(style()->height());
    else
        height = calcAspectRatioHeight();

    int minH = calcReplacedHeightUsing(style()->minHeight());
    int maxH = style()->maxHeight().value() == undefinedLength ? height : calcReplacedHeightUsing(style()->maxHeight());

    return max(minH, min(height, maxH));
}

int RenderImage::calcAspectRatioWidth() const
{
    if (!intrinsicHeight())
        return 0;
    if (!m_cachedImage || m_cachedImage->isErrorImage())
        return intrinsicWidth(); // Don't bother scaling.
    return RenderReplaced::calcReplacedHeight() * intrinsicWidth() / intrinsicHeight();
}

int RenderImage::calcAspectRatioHeight() const
{
    if (!intrinsicWidth())
        return 0;
    if (!m_cachedImage || m_cachedImage->isErrorImage())
        return intrinsicHeight(); // Don't bother scaling.
    return RenderReplaced::calcReplacedWidth() * intrinsicHeight() / intrinsicWidth();
}

void RenderImage::calcMinMaxWidth()
{
    ASSERT(!minMaxKnown());

    m_maxWidth = calcReplacedWidth() + paddingLeft() + paddingRight() + borderLeft() + borderRight();

    if (style()->width().isPercent() || style()->height().isPercent() || 
        style()->maxWidth().isPercent() || style()->maxHeight().isPercent() ||
        style()->minWidth().isPercent() || style()->minHeight().isPercent())
        m_minWidth = 0;
    else
        m_minWidth = m_maxWidth;

    setMinMaxKnown();
}

Image* RenderImage::nullImage()
{
    static Image sharedNullImage;
    return &sharedNullImage;
}

} // namespace WebCore
