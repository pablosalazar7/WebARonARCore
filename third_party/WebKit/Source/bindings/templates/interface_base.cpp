{# FIXME: update copyright and license header #}
/*
    This file is part of the Blink open source project.
    This file has been auto-generated by CodeGeneratorV8.pm. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
{% if conditional_string %}
#if {{conditional_string}}
{% endif %}
#include "{{v8_class_name}}.h"

{% for filename in cpp_includes %}
#include "{{filename}}"
{% endfor %}

namespace WebCore {

static void initializeScriptWrappableForInterface({{cpp_class_name}}* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &{{v8_class_name}}::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::{{cpp_class_name}}* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo {{v8_class_name}}::info = { {{v8_class_name}}::GetTemplate, {{v8_class_name}}::derefObject, 0, 0, 0, {{v8_class_name}}::installPerContextEnabledPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace {{cpp_class_name}}V8Internal {

template <typename T> void V8_USE(T) { }

{% from 'attributes.cpp' import attribute_getter, attribute_getter_callback,
       attribute_setter, attribute_setter_callback
   with context %}
{% for attribute in attributes %}
{% for world_suffix in attribute.world_suffixes %}
{% if not attribute.has_custom_getter %}
{{attribute_getter(attribute, world_suffix)}}
{% endif %}
{{attribute_getter_callback(attribute, world_suffix)}}
{% if not attribute.is_read_only %}
{% if not attribute.has_custom_setter %}
{{attribute_setter(attribute, world_suffix)}}
{% endif %}
{{attribute_setter_callback(attribute, world_suffix)}}
{% endif %}
{% endfor %}
{% endfor %}
{% block replaceable_attribute_setter_and_callback %}{% endblock %}
} // namespace {{cpp_class_name}}V8Internal

{% block class_attributes %}{% endblock %}
{% block configure_class_template %}{% endblock %}
{% block get_template %}{% endblock %}
{% block has_instance_and_has_instance_in_any_world %}{% endblock %}
{% block install_per_context_attributes %}{% endblock %}
{% block create_wrapper_and_deref_object %}{% endblock %}
} // namespace WebCore
{% if conditional_string %}

#endif // {{conditional_string}}
{% endif %}
