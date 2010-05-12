/**
 * Copyright (C) 2003, 2006 Apple Computer, Inc.
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "EllipsisBox.h"

#include "Document.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "RootInlineBox.h"

namespace WebCore {

void EllipsisBox::paint(RenderObject::PaintInfo& paintInfo, int tx, int ty)
{
    GraphicsContext* context = paintInfo.context;
    RenderStyle* style = m_renderer->style(m_firstLine);
    Color textColor = style->color();
    if (textColor != context->fillColor())
        context->setFillColor(textColor, style->colorSpace());
    bool setShadow = false;
    if (style->textShadow()) {
        context->setShadow(IntSize(style->textShadow()->x(), style->textShadow()->y()),
                           style->textShadow()->blur(), style->textShadow()->color(), style->colorSpace());
        setShadow = true;
    }

    if (selectionState() != RenderObject::SelectionNone) {
        paintSelection(context, tx, ty, style, style->font());

        // Select the correct color for painting the text.
        Color foreground = paintInfo.forceBlackText ? Color::black : renderer()->selectionForegroundColor();
        if (foreground.isValid() && foreground != textColor)
            context->setFillColor(foreground, style->colorSpace());
    }

    const String& str = m_str;
    context->drawText(style->font(), TextRun(str.characters(), str.length(), false, 0, 0, false, style->visuallyOrdered()), IntPoint(x() + tx, y() + ty + style->font().ascent()));

    // Restore the regular fill color.
    if (textColor != context->fillColor())
        context->setFillColor(textColor, style->colorSpace());

    if (setShadow)
        context->clearShadow();

    if (m_markupBox) {
        // Paint the markup box
        tx += frameRect().right() - m_markupBox->x();
        ty += y() + style->font().ascent() - (m_markupBox->y() + m_markupBox->renderer()->style(m_firstLine)->font().ascent());
        m_markupBox->paint(paintInfo, tx, ty);
    }
}

IntRect EllipsisBox::selectionRect(int tx, int ty)
{
    RenderStyle* style = m_renderer->style(m_firstLine);
    const Font& f = style->font();
    return enclosingIntRect(f.selectionRectForText(TextRun(m_str.characters(), m_str.length(), false, 0, 0, false, style->visuallyOrdered()),
            IntPoint(x() + tx, y() + ty + root()->selectionTop()), root()->selectionHeight()));
}

void EllipsisBox::paintSelection(GraphicsContext* context, int tx, int ty, RenderStyle* style, const Font& font)
{
    Color textColor = style->color();
    Color c = m_renderer->selectionBackgroundColor();
    if (!c.isValid() || !c.alpha())
        return;

    // If the text color ends up being the same as the selection background, invert the selection
    // background.
    if (textColor == c)
        c = Color(0xff - c.red(), 0xff - c.green(), 0xff - c.blue());

    context->save();
    int t = root()->selectionTop();
    int h = root()->selectionHeight();
    context->clip(IntRect(x() + tx, t + ty, width(), h));
    context->drawHighlightForText(font, TextRun(m_str.characters(), m_str.length(), false, 0, 0, false, style->visuallyOrdered()),
        IntPoint(x() + tx, y() + ty + t), h, c, style->colorSpace());
    context->restore();
}

bool EllipsisBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int pointX, int pointY, int tx, int ty)
{
    tx += x();
    ty += y();

    // Hit test the markup box.
    if (m_markupBox) {
        RenderStyle* style = m_renderer->style(m_firstLine);
        int mtx = tx + width() - m_markupBox->x();
        int mty = ty + style->font().ascent() - (m_markupBox->y() + m_markupBox->renderer()->style(m_firstLine)->font().ascent());
        if (m_markupBox->nodeAtPoint(request, result, pointX, pointY, mtx, mty)) {
            renderer()->updateHitTestResult(result, IntPoint(pointX - mtx, pointY - mty));
            return true;
        }
    }

    if (visibleToHitTesting() && IntRect(tx, ty, width(), height()).contains(pointX, pointY)) {
        renderer()->updateHitTestResult(result, IntPoint(pointX - tx, pointY - ty));
        return true;
    }

    return false;
}

} // namespace WebCore
