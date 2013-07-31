/**
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "core/dom/KeyboardEvent.h"

#include "core/dom/EventNames.h"
#include "core/platform/PlatformKeyboardEvent.h"
#include "core/platform/WindowsKeyboardCodes.h"

namespace WebCore {

static inline const AtomicString& eventTypeForKeyboardEventType(PlatformEvent::Type type)
{
    switch (type) {
        case PlatformEvent::KeyUp:
            return eventNames().keyupEvent;
        case PlatformEvent::RawKeyDown:
            return eventNames().keydownEvent;
        case PlatformEvent::Char:
            return eventNames().keypressEvent;
        case PlatformEvent::KeyDown:
            // The caller should disambiguate the combined event into RawKeyDown or Char events.
            break;
        default:
            break;
    }
    ASSERT_NOT_REACHED();
    return eventNames().keydownEvent;
}

static inline int windowsVirtualKeyCodeWithoutLocation(int keycode)
{
    switch (keycode) {
    case VK_LCONTROL:
    case VK_RCONTROL:
        return VK_CONTROL;
    case VK_LSHIFT:
    case VK_RSHIFT:
        return VK_SHIFT;
    case VK_LMENU:
    case VK_RMENU:
        return VK_MENU;
    default:
        return keycode;
    }
}

static inline KeyboardEvent::KeyLocationCode keyLocationCode(const PlatformKeyboardEvent& key)
{
    if (key.isKeypad())
        return KeyboardEvent::DOM_KEY_LOCATION_NUMPAD;

    // FIXME: Support DOM_KEY_LOCATION_MOBILE & DOM_KEY_LOCATION_JOYSTICK (crbug.com/265446).
    switch (key.windowsVirtualKeyCode()) {
    case VK_LCONTROL:
    case VK_LSHIFT:
    case VK_LMENU:
    case VK_LWIN:
        return KeyboardEvent::DOM_KEY_LOCATION_LEFT;
    case VK_RCONTROL:
    case VK_RSHIFT:
    case VK_RMENU:
    case VK_RWIN:
        return KeyboardEvent::DOM_KEY_LOCATION_RIGHT;
    default:
        return KeyboardEvent::DOM_KEY_LOCATION_STANDARD;
    }
}

KeyboardEventInit::KeyboardEventInit()
    : location(0)
    , ctrlKey(false)
    , altKey(false)
    , shiftKey(false)
    , metaKey(false)
{
}

KeyboardEvent::KeyboardEvent()
    : m_location(DOM_KEY_LOCATION_STANDARD)
    , m_altGraphKey(false)
{
    ScriptWrappable::init(this);
}

KeyboardEvent::KeyboardEvent(const PlatformKeyboardEvent& key, AbstractView* view)
    : UIEventWithKeyState(eventTypeForKeyboardEventType(key.type()),
                          true, true, view, 0, key.ctrlKey(), key.altKey(), key.shiftKey(), key.metaKey())
    , m_keyEvent(adoptPtr(new PlatformKeyboardEvent(key)))
    , m_keyIdentifier(key.keyIdentifier())
    , m_location(keyLocationCode(key))
    , m_altGraphKey(false)
{
    ScriptWrappable::init(this);
}

KeyboardEvent::KeyboardEvent(const AtomicString& eventType, const KeyboardEventInit& initializer)
    : UIEventWithKeyState(eventType, initializer.bubbles, initializer.cancelable, initializer.view, initializer.detail, initializer.ctrlKey, initializer.altKey, initializer.shiftKey, initializer.metaKey)
    , m_keyIdentifier(initializer.keyIdentifier)
    , m_location(initializer.location)
    , m_altGraphKey(false)
{
    ScriptWrappable::init(this);
}

KeyboardEvent::KeyboardEvent(const AtomicString& eventType, bool canBubble, bool cancelable, AbstractView *view,
                             const String &keyIdentifier,  unsigned location,
                             bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool altGraphKey)
    : UIEventWithKeyState(eventType, canBubble, cancelable, view, 0, ctrlKey, altKey, shiftKey, metaKey)
    , m_keyIdentifier(keyIdentifier)
    , m_location(location)
    , m_altGraphKey(altGraphKey)
{
    ScriptWrappable::init(this);
}

KeyboardEvent::~KeyboardEvent()
{
}

void KeyboardEvent::initKeyboardEvent(const AtomicString& type, bool canBubble, bool cancelable, AbstractView* view,
                                      const String &keyIdentifier, unsigned location,
                                      bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool altGraphKey)
{
    if (dispatched())
        return;

    initUIEvent(type, canBubble, cancelable, view, 0);

    m_keyIdentifier = keyIdentifier;
    m_location = location;
    m_ctrlKey = ctrlKey;
    m_shiftKey = shiftKey;
    m_altKey = altKey;
    m_metaKey = metaKey;
    m_altGraphKey = altGraphKey;
}

bool KeyboardEvent::getModifierState(const String& keyIdentifier) const
{
    // FIXME: The following keyIdentifiers are not supported yet (crbug.com/265458):
    // "AltGraph", "CapsLock", "Fn", "NumLock", "ScrollLock", "SymbolLock", "OS".
    if (keyIdentifier == "Control")
        return ctrlKey();
    if (keyIdentifier == "Shift")
        return shiftKey();
    if (keyIdentifier == "Alt")
        return altKey();
    if (keyIdentifier == "Meta")
        return metaKey();
    return false;
}

int KeyboardEvent::keyCode() const
{
    // IE: virtual key code for keyup/keydown, character code for keypress
    // Firefox: virtual key code for keyup/keydown, zero for keypress
    // We match IE.
    if (!m_keyEvent)
        return 0;
    if (type() == eventNames().keydownEvent || type() == eventNames().keyupEvent)
        return windowsVirtualKeyCodeWithoutLocation(m_keyEvent->windowsVirtualKeyCode());

    return charCode();
}

int KeyboardEvent::charCode() const
{
    // IE: not supported
    // Firefox: 0 for keydown/keyup events, character code for keypress
    // We match Firefox

    if (!m_keyEvent || (type() != eventNames().keypressEvent))
        return 0;
    String text = m_keyEvent->text();
    return static_cast<int>(text.characterStartingAt(0));
}

const AtomicString& KeyboardEvent::interfaceName() const
{
    return eventNames().interfaceForKeyboardEvent;
}

bool KeyboardEvent::isKeyboardEvent() const
{
    return true;
}

int KeyboardEvent::which() const
{
    // Netscape's "which" returns a virtual key code for keydown and keyup, and a character code for keypress.
    // That's exactly what IE's "keyCode" returns. So they are the same for keyboard events.
    return keyCode();
}

KeyboardEvent* findKeyboardEvent(Event* event)
{
    for (Event* e = event; e; e = e->underlyingEvent()) {
        if (e->isKeyboardEvent())
            return toKeyboardEvent(e);
    }
    return 0;
}

PassRefPtr<KeyboardEventDispatchMediator> KeyboardEventDispatchMediator::create(PassRefPtr<KeyboardEvent> event)
{
    return adoptRef(new KeyboardEventDispatchMediator(event));
}

KeyboardEventDispatchMediator::KeyboardEventDispatchMediator(PassRefPtr<KeyboardEvent> event)
    : EventDispatchMediator(event)
{
}

bool KeyboardEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    // Make sure not to return true if we already took default action while handling the event.
    return EventDispatchMediator::dispatchEvent(dispatcher) && !event()->defaultHandled();
}

} // namespace WebCore
