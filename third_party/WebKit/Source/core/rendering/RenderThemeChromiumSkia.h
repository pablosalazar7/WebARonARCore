/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008, 2009 Google, Inc.
 * All rights reserved.
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
 *
 */

#ifndef RenderThemeChromiumSkia_h
#define RenderThemeChromiumSkia_h

#include "core/rendering/RenderTheme.h"

namespace WebCore {

class RenderProgress;

class RenderThemeChromiumSkia : public RenderTheme {
public:
    RenderThemeChromiumSkia();
    virtual ~RenderThemeChromiumSkia();

    virtual String extraDefaultStyleSheet();
    virtual String extraQuirksStyleSheet();
    virtual String extraMediaControlsStyleSheet();

    virtual Color platformTapHighlightColor() const OVERRIDE
    {
        return Color(defaultTapHighlightColor);
    }

    // A method asking if the theme's controls actually care about redrawing when hovered.
    virtual bool supportsHover(const RenderStyle*) const;

    // A method asking if the theme is able to draw the focus ring.
    virtual bool supportsFocusRing(const RenderStyle*) const;

    virtual bool supportsClosedCaptioning() const OVERRIDE;
    // The platform selection color.
    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveSelectionForegroundColor() const;
    virtual Color platformInactiveSelectionForegroundColor() const;
    virtual Color platformFocusRingColor() const;

    // To change the blink interval, override caretBlinkIntervalInternal instead of this one so that we may share layout test code an intercepts.
    virtual double caretBlinkInterval() const;

    // System fonts.
    virtual void systemFont(CSSValueID, FontDescription&) const;

    virtual int minimumMenuListSize(RenderStyle*) const;

    virtual void setCheckboxSize(RenderStyle*) const;

    virtual void setRadioSize(RenderStyle*) const;

    virtual void adjustButtonStyle(RenderStyle*, Element*) const;

    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldStyle(RenderStyle*, Element*) const;
    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldCancelButtonStyle(RenderStyle*, Element*) const;
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldDecorationStyle(RenderStyle*, Element*) const;

    virtual void adjustSearchFieldResultsDecorationStyle(RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderThumbSize(RenderStyle*, Element*) const;
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaToggleClosedCaptionsButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual String formatMediaControlsTime(float time) const;
    virtual String formatMediaControlsCurrentTime(float currentTime, float duration) const;
    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);

    // MenuList refers to an unstyled menulist (meaning a menulist without
    // background-color or border set) and MenuListButton refers to a styled
    // menulist (a menulist with background-color or border set). They have
    // this distinction to support showing aqua style themes whenever they
    // possibly can, which is something we don't want to replicate.
    //
    // In short, we either go down the MenuList code path or the MenuListButton
    // codepath. We never go down both. And in both cases, they render the
    // entire menulist.
    virtual void adjustMenuListStyle(RenderStyle*, Element*) const;
    virtual void adjustMenuListButtonStyle(RenderStyle*, Element*) const;
    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const;
    virtual double animationDurationForProgressBar(RenderProgress*) const;

    // These methods define the padding for the MenuList's inner block.
    virtual int popupInternalPaddingLeft(RenderStyle*) const;
    virtual int popupInternalPaddingRight(RenderStyle*) const;
    virtual int popupInternalPaddingTop(RenderStyle*) const;
    virtual int popupInternalPaddingBottom(RenderStyle*) const;

    // Media controls
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const { return true; }
    virtual bool usesVerticalVolumeSlider() const { return false; }

    // Provide a way to pass the default font size from the Settings object
    // to the render theme. FIXME: http://b/1129186 A cleaner way would be
    // to remove the default font size from this object and have callers
    // that need the value to get it directly from the appropriate Settings
    // object.
    static void setDefaultFontSize(int);

protected:
    virtual double caretBlinkIntervalInternal() const;

    virtual int menuListArrowPadding() const;

    IntRect determinateProgressValueRectFor(RenderProgress*, const IntRect&) const;
    IntRect indeterminateProgressValueRectFor(RenderProgress*, const IntRect&) const;
    IntRect progressValueRectFor(RenderProgress*, const IntRect&) const;

    class DirectionFlippingScope {
    public:
        DirectionFlippingScope(RenderObject*, const PaintInfo&, const IntRect&);
        ~DirectionFlippingScope();

    private:
        bool m_needsFlipping;
        const PaintInfo& m_paintInfo;
    };

private:
    virtual bool shouldShowPlaceholderWhenFocused() const OVERRIDE;

    int menuListInternalPadding(RenderStyle*, int paddingType) const;
    bool paintMediaButtonInternal(GraphicsContext*, const IntRect&, Image*);
    IntRect convertToPaintingRect(RenderObject* inputRenderer, const RenderObject* partRenderer, LayoutRect partRect, const IntRect& localOffset) const;

    static const RGBA32 defaultTapHighlightColor = 0x2e000000; // 18% black.
};

} // namespace WebCore

#endif // RenderThemeChromiumSkia_h
