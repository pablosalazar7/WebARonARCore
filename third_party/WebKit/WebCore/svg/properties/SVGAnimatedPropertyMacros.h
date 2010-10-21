/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGAnimatedPropertyMacros_h
#define SVGAnimatedPropertyMacros_h

#if ENABLE(SVG)
#include "SVGAnimatedListPropertyTearOff.h"
#include "SVGAnimatedPropertySynchronizer.h"
#include "SVGAnimatedPropertyTearOff.h"
#include "SVGPropertyTraits.h"

namespace WebCore {

template<typename PropertyType>
struct SVGSynchronizableAnimatedProperty {
    SVGSynchronizableAnimatedProperty()
        : value(SVGPropertyTraits<PropertyType>::initialValue())
        , shouldSynchronize(false)
    {
    }

    template<typename ConstructorParameter1>
    SVGSynchronizableAnimatedProperty(const ConstructorParameter1& value1)
        : value(value1)
        , shouldSynchronize(false)
    {
    }

    template<typename ConstructorParameter1, typename ConstructorParameter2>
    SVGSynchronizableAnimatedProperty(const ConstructorParameter1& value1, const ConstructorParameter2& value2)
        : value(value1, value2)
        , shouldSynchronize(false)
    {
    }

    PropertyType value;
    bool shouldSynchronize : 1;
};

// FIXME: These macros should be removed, after the transition to the new SVGAnimatedProperty concept is finished.
#define DECLARE_ANIMATED_PROPERTY_NEW_SHARED(OwnerType, DOMAttribute, TearOffType, PropertyType, UpperProperty, LowerProperty) \
public: \
PropertyType& LowerProperty() const \
{ \
    return m_##LowerProperty.value; \
} \
\
PropertyType& LowerProperty##BaseValue() const \
{ \
    return m_##LowerProperty.value; \
} \
\
void set##UpperProperty##BaseValue(const PropertyType& type) \
{ \
    m_##LowerProperty.value = type; \
    invalidateSVGAttributes(); \
} \
\
void synchronize##UpperProperty() \
{ \
    if (!m_##LowerProperty.shouldSynchronize) \
         return; \
    AtomicString value(SVGPropertyTraits<PropertyType>::toString(LowerProperty##BaseValue())); \
    SVGAnimatedPropertySynchronizer<IsDerivedFromSVGElement<OwnerType>::value>::synchronize(this, DOMAttribute, value); \
} \
\
PassRefPtr<TearOffType> LowerProperty##Animated() \
{ \
    m_##LowerProperty.shouldSynchronize = true; \
    return SVGAnimatedProperty::lookupOrCreateWrapper<TearOffType, PropertyType>(this, DOMAttribute, m_##LowerProperty.value); \
} \
private: \
    mutable SVGSynchronizableAnimatedProperty<PropertyType> m_##LowerProperty;

#define DECLARE_ANIMATED_PROPERTY_NEW(OwnerType, DOMAttribute, PropertyType, UpperProperty, LowerProperty) \
DECLARE_ANIMATED_PROPERTY_NEW_SHARED(OwnerType, DOMAttribute, SVGAnimatedPropertyTearOff<PropertyType>, PropertyType, UpperProperty, LowerProperty)

#define DECLARE_ANIMATED_LIST_PROPERTY_NEW(OwnerType, DOMAttribute, PropertyType, UpperProperty, LowerProperty) \
DECLARE_ANIMATED_PROPERTY_NEW_SHARED(OwnerType, DOMAttribute, SVGAnimatedListPropertyTearOff<PropertyType>, PropertyType, UpperProperty, LowerProperty) \
\
void detachAnimated##UpperProperty##ListWrappers(unsigned newListSize) \
{ \
    SVGAnimatedProperty* wrapper = SVGAnimatedProperty::lookupWrapper<SVGAnimatedListPropertyTearOff<PropertyType> >(this, DOMAttribute); \
    if (!wrapper) \
        return; \
    static_cast<SVGAnimatedListPropertyTearOff<PropertyType>*>(wrapper)->detachListWrappers(newListSize); \
}

}

#endif // ENABLE(SVG)
#endif // SVGAnimatedPropertyMacros_h
