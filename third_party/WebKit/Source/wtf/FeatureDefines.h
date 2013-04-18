/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef WTF_FeatureDefines_h
#define WTF_FeatureDefines_h

/* Use this file to list _all_ ENABLE() macros. Define the macros to be one of the following values:
 *  - "0" disables the feature by default. The feature can still be enabled for a specific port or environment.
 *  - "1" enables the feature by default. The feature can still be disabled for a specific port or environment.
 *
 * The feature defaults in this file are only taken into account if the (port specific) build system
 * has not enabled or disabled a particular feature. 
 *
 * Use this file to define ENABLE() macros only. Do not use this file to define USE() or macros !
 *
 * Only define a macro if it was not defined before - always check for !defined first.
 * 
 * Keep the file sorted by the name of the defines. As an exception you can change the order
 * to allow interdependencies between the default values.
 * 
 * Below are a few potential commands to take advantage of this file running from the Source/WTF directory
 *
 * Get the list of feature defines: grep -o "ENABLE_\(\w\+\)" wtf/FeatureDefines.h | sort | uniq
 * Get the list of features enabled by default for a PLATFORM(XXX): gcc -E -dM -I. -DWTF_PLATFORM_XXX "wtf/Platform.h" | grep "ENABLE_\w\+ 1" | cut -d' ' -f2 | sort 
 */

/* FIXME: Move out the PLATFORM specific rules into platform specific files. */

/* ENABLE macro defaults for WebCore */
/* Do not use PLATFORM() tests in this section ! */

#if !defined(ENABLE_3D_PLUGIN)
#define ENABLE_3D_PLUGIN 0
#endif

#if !defined(ENABLE_8BIT_TEXTRUN)
#define ENABLE_8BIT_TEXTRUN 0
#endif

#if !defined(ENABLE_ACCELERATED_OVERFLOW_SCROLLING)
#define ENABLE_ACCELERATED_OVERFLOW_SCROLLING 0
#endif

#if !defined(ENABLE_BATTERY_STATUS)
#define ENABLE_BATTERY_STATUS 0
#endif

#if !defined(ENABLE_CALENDAR_PICKER)
#define ENABLE_CALENDAR_PICKER 0
#endif

#if !defined(ENABLE_CANVAS_PROXY)
#define ENABLE_CANVAS_PROXY 0
#endif

#if !defined(ENABLE_CHANNEL_MESSAGING)
#define ENABLE_CHANNEL_MESSAGING 1
#endif

#if !defined(ENABLE_CONTEXT_MENUS)
#define ENABLE_CONTEXT_MENUS 1
#endif

#if !defined(ENABLE_CSS3_CONDITIONAL_RULES)
#define ENABLE_CSS3_CONDITIONAL_RULES 0
#endif

#if !defined(ENABLE_CSS3_TEXT)
#define ENABLE_CSS3_TEXT 0
#endif

#if !defined(ENABLE_CSS_DEVICE_ADAPTATION)
#define ENABLE_CSS_DEVICE_ADAPTATION 0
#endif

#if !defined(ENABLE_CSS_COMPOSITING)
#define ENABLE_CSS_COMPOSITING 0
#endif

#if !defined(ENABLE_CSS_FILTERS)
#define ENABLE_CSS_FILTERS 0
#endif

#if !defined(ENABLE_CSS_IMAGE_ORIENTATION)
#define ENABLE_CSS_IMAGE_ORIENTATION 0
#endif

#if !defined(ENABLE_CSS_IMAGE_SET)
#define ENABLE_CSS_IMAGE_SET 0
#endif

#if !defined(ENABLE_CSS_TRANSFORMS_ANIMATIONS_TRANSITIONS_UNPREFIXED)
#define ENABLE_CSS_TRANSFORMS_ANIMATIONS_TRANSITIONS_UNPREFIXED 0
#endif

#if !defined(ENABLE_CUSTOM_SCHEME_HANDLER)
#define ENABLE_CUSTOM_SCHEME_HANDLER 0
#endif

#if !defined(ENABLE_DATALIST_ELEMENT)
#define ENABLE_DATALIST_ELEMENT 0
#endif

#if !defined(ENABLE_DIALOG_ELEMENT)
#define ENABLE_DIALOG_ELEMENT 0
#endif

#if !defined(ENABLE_ENCRYPTED_MEDIA)
#define ENABLE_ENCRYPTED_MEDIA 0
#endif

#if !defined(ENABLE_ENCRYPTED_MEDIA_V2)
#define ENABLE_ENCRYPTED_MEDIA_V2 0
#endif

#if !defined(ENABLE_FAST_MOBILE_SCROLLING)
#define ENABLE_FAST_MOBILE_SCROLLING 0
#endif

#if !defined(ENABLE_FONT_LOAD_EVENTS)
#define ENABLE_FONT_LOAD_EVENTS 0
#endif

#if !defined(ENABLE_GAMEPAD)
#define ENABLE_GAMEPAD 0
#endif

#if !defined(ENABLE_HIDDEN_PAGE_DOM_TIMER_THROTTLING)
#define ENABLE_HIDDEN_PAGE_DOM_TIMER_THROTTLING 0
#endif

#if !defined(ENABLE_HIGH_DPI_CANVAS)
#define ENABLE_HIGH_DPI_CANVAS 0
#endif

#if !defined(ENABLE_ICONDATABASE)
#define ENABLE_ICONDATABASE 1
#endif

#if !defined(ENABLE_IMAGE_DECODER_DOWN_SAMPLING)
#define ENABLE_IMAGE_DECODER_DOWN_SAMPLING 0
#endif

#if !defined(ENABLE_INPUT_MULTIPLE_FIELDS_UI)
#define ENABLE_INPUT_MULTIPLE_FIELDS_UI 0
#endif

#if !defined(ENABLE_INPUT_SPEECH)
#define ENABLE_INPUT_SPEECH 0
#endif

#if !defined(ENABLE_INPUT_TYPE_COLOR)
#define ENABLE_INPUT_TYPE_COLOR 0
#endif

#if !defined(ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE)
#define ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE 0
#endif

#if !defined(ENABLE_JAVASCRIPT_I18N_API)
#define ENABLE_JAVASCRIPT_I18N_API 0
#endif

#if !defined(ENABLE_LEGACY_NOTIFICATIONS)
#define ENABLE_LEGACY_NOTIFICATIONS 0
#endif

#if !defined(ENABLE_MATHML)
#define ENABLE_MATHML 1
#endif

#if !defined(ENABLE_MEDIA_CAPTURE)
#define ENABLE_MEDIA_CAPTURE 0
#endif

#if !defined(ENABLE_MEDIA_STREAM)
#define ENABLE_MEDIA_STREAM 0
#endif

#if !defined(ENABLE_MOUSE_CURSOR_SCALE)
#define ENABLE_MOUSE_CURSOR_SCALE 0
#endif

#if !defined(ENABLE_NAVIGATOR_CONTENT_UTILS)
#define ENABLE_NAVIGATOR_CONTENT_UTILS 0
#endif

#if !defined(ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE)
#define ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE 0
#endif

#if !defined(ENABLE_NOTIFICATIONS)
#define ENABLE_NOTIFICATIONS 0
#endif

#if !defined(ENABLE_OPENTYPE_VERTICAL)
#define ENABLE_OPENTYPE_VERTICAL 0
#endif

#if !defined(ENABLE_ORIENTATION_EVENTS)
#define ENABLE_ORIENTATION_EVENTS 0
#endif

#if !defined(ENABLE_PAGE_POPUP)
#define ENABLE_PAGE_POPUP 0
#endif

#if OS(WINDOWS)
#if !defined(ENABLE_PAN_SCROLLING)
#define ENABLE_PAN_SCROLLING 1
#endif
#endif

#if !defined(ENABLE_PARSED_STYLE_SHEET_CACHING)
#define ENABLE_PARSED_STYLE_SHEET_CACHING 1
#endif

#if !defined(ENABLE_REPAINT_THROTTLING)
#define ENABLE_REPAINT_THROTTLING 0
#endif

#if !defined(ENABLE_RUBBER_BANDING)
#define ENABLE_RUBBER_BANDING 0
#endif

#if !defined(ENABLE_SATURATED_LAYOUT_ARITHMETIC)
#define ENABLE_SATURATED_LAYOUT_ARITHMETIC 1
#endif

#if !defined(ENABLE_SHARED_WORKERS)
#define ENABLE_SHARED_WORKERS 0
#endif

#if !defined(ENABLE_SPEECH_SYNTHESIS)
#define ENABLE_SPEECH_SYNTHESIS 0
#endif

#if !defined(ENABLE_SVG)
#define ENABLE_SVG 1
#endif

#if ENABLE(SVG)
#if !defined(ENABLE_SVG_FONTS)
#define ENABLE_SVG_FONTS 1
#endif
#endif

#if !defined(ENABLE_TEXT_NOTIFICATIONS_ONLY)
#define ENABLE_TEXT_NOTIFICATIONS_ONLY 0
#endif

#if !defined(ENABLE_THREADED_HTML_PARSER)
#define ENABLE_THREADED_HTML_PARSER 0
#endif

#if !defined(ENABLE_THREADED_SCROLLING)
#define ENABLE_THREADED_SCROLLING 0
#endif

#if !defined(ENABLE_TOUCH_EVENTS)
#define ENABLE_TOUCH_EVENTS 0
#endif

#if !defined(ENABLE_TOUCH_ICON_LOADING)
#define ENABLE_TOUCH_ICON_LOADING 0
#endif

#if !defined(ENABLE_VIBRATION)
#define ENABLE_VIBRATION 0
#endif

#if !defined(ENABLE_VIDEO)
#define ENABLE_VIDEO 0
#endif

#if !defined(ENABLE_VIEWPORT)
#define ENABLE_VIEWPORT 0
#endif

#if !defined(ENABLE_VIEWSOURCE_ATTRIBUTE)
#define ENABLE_VIEWSOURCE_ATTRIBUTE 1
#endif

#if !defined(ENABLE_WEBGL)
#define ENABLE_WEBGL 0
#endif

#if !defined(ENABLE_WEB_AUDIO)
#define ENABLE_WEB_AUDIO 0
#endif

#if !defined(ENABLE_XHR_TIMEOUT)
#define ENABLE_XHR_TIMEOUT 0
#endif

/* Asserts, invariants for macro definitions */

#if ENABLE(SVG_FONTS) && !ENABLE(SVG)
#error "ENABLE(SVG_FONTS) requires ENABLE(SVG)"
#endif

#endif /* WTF_FeatureDefines_h */
