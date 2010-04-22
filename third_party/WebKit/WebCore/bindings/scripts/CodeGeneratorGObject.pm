# Copyright (C) 2008 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
# Copyright (C) 2008 Martin Soto <soto@freedesktop.org>
# Copyright (C) 2008 Alp Toker <alp@atoker.com>
# Copyright (C) 2009 Adam Dingle <adam@yorba.org>
# Copyright (C) 2009 Jim Nelson <jim@yorba.org>
# Copyright (C) 2009, 2010 Igalia S.L.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

package CodeGeneratorGObject;

# Global Variables
my %implIncludes = ();
my %hdrIncludes = ();

my $className = "";

# Default constructor
sub new {
    my $object = shift;
    my $reference = { };

    $codeGenerator = shift;
    $outputDir = shift;
    mkdir $outputDir;

    bless($reference, $object);
}

sub finish {
}

my $licenceTemplate = << "EOF";
/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
EOF

sub GenerateModule {
}

sub GetParentClassName {
    my $dataNode = shift;

    return "WebKitDOMObject" if @{$dataNode->parents} eq 0;
    return "WebKitDOM" . $codeGenerator->StripModule($dataNode->parents(0));
}

# From String::CamelCase 0.01
sub camelize
{
        my $s = shift;
        join('', map{ ucfirst $_ } split(/(?<=[A-Za-z])_(?=[A-Za-z])|\b/, $s));
}

sub decamelize
{
        my $s = shift;
        $s =~ s{([^a-zA-Z]?)([A-Z]*)([A-Z])([a-z]?)}{
                my $fc = pos($s)==0;
                my ($p0,$p1,$p2,$p3) = ($1,lc$2,lc$3,$4);
                my $t = $p0 || $fc ? $p0 : '_';
                $t .= $p3 ? $p1 ? "${p1}_$p2$p3" : "$p2$p3" : "$p1$p2";
                $t;
        }ge;
        $s;
}

sub FixUpDecamelizedName {
    my $classname = shift;

    # FIXME: try to merge this somehow with the fixes in ClassNameToGobjectType
    $classname =~ s/x_path/xpath/;
    $classname =~ s/web_kit/webkit/;

    return $classname;
}

sub ClassNameToGObjectType {
    my $className = shift;
    my $CLASS_NAME = uc(decamelize($className));
    # Fixup: with our prefix being 'WebKitDOM' decamelize can't get
    # WebKitDOMCSS right, so we have to fix it manually (and there
    # might be more like this in the future)
    $CLASS_NAME =~ s/DOMCSS/DOM_CSS/;
    $CLASS_NAME =~ s/DOMHTML/DOM_HTML/;
    $CLASS_NAME =~ s/DOMDOM/DOM_DOM/;
    $CLASS_NAME =~ s/DOMCDATA/DOM_CDATA/;
    $CLASS_NAME =~ s/DOMX_PATH/DOM_XPATH/;
    $CLASS_NAME =~ s/DOM_WEB_KIT/DOM_WEBKIT/;
    return $CLASS_NAME;
}

sub GetParentGObjType {
    my $dataNode = shift;

    return "WEBKIT_TYPE_DOM_OBJECT" if @{$dataNode->parents} eq 0;
    return "WEBKIT_TYPE_DOM_" . ClassNameToGObjectType($codeGenerator->StripModule($dataNode->parents(0)));
}

sub GetClassName {
    my $name = $codeGenerator->StripModule(shift);

    return "WebKitDOM$name";
}

sub GetCoreObject {
    my ($interfaceName, $name, $parameter) = @_;

    return "WebCore::${interfaceName}* $name = WebKit::core($parameter);";
}

sub SkipAttribute {
    my $attribute = shift;
    
    if ($attribute->signature->extendedAttributes->{"CustomGetter"} ||
        $attribute->signature->extendedAttributes->{"CustomSetter"} ||
        $attribute->signature->extendedAttributes->{"Replaceable"}) {
        return 1;
    }
    
    my $propType = $attribute->signature->type;
    if ($propType eq "EventListener") {
        return 1;
    }

    if ($propType =~ /Constructor$/) {
        return 1;
    }

    # This is for DOMWindow.idl location attribute
    if ($attribute->signature->name eq "location") {
        return 1;
    }

    # This is for HTMLInput.idl valueAsDate
    if ($attribute->signature->name eq "valueAsDate") {
        return 1;
    }

    # This is for DOMWindow.idl Crypto attribute
    if ($attribute->signature->type eq "Crypto") {
        return 1;
    }

    return 0;
}

sub SkipFunction {
    my $function = shift;

    # FIXME: skip all custom methods; is this ok?
    if ($function->signature->extendedAttributes->{"Custom"}) {
        return 1;
    }

    if ($function->signature->type eq "Event") {
        return 1;
    }

    if ($function->signature->name eq "getSVGDocument") {
        return 1;
    }

    if ($function->signature->name eq "getCSSCanvasContext") {
        return 1;
    }

    return 0;
}

# Name type used in the g_value_{set,get}_* functions
sub GetGValueTypeName {
    my $type = shift;

    my %types = ("DOMString", "string",
                 "float", "float",
                 "double", "double",
                 "boolean", "boolean",
                 "char", "char",
                 "long", "long",
                 "short", "int",
                 "uchar", "uchar",
                 "unsigned", "uint",
                 "int", "int",
                 "unsigned int", "uint",
                 "unsigned long long", "uint64", 
                 "unsigned long", "ulong",
                 "unsigned short", "ushort");

    return $types{$type} ? $types{$type} : "object";
}

# Name type used in C declarations
sub GetGlibTypeName {
    my $type = shift;
    my $name = GetClassName($type);

    my %types = ("DOMString", "gchar* ",
                 "CompareHow", "gushort",
                 "float", "gfloat",
                 "double", "gdouble",
                 "boolean", "gboolean",
                 "char", "gchar",
                 "long", "glong",
                 "short", "gshort",
                 "uchar", "guchar",
                 "unsigned", "guint",
                 "int", "gint",
                 "unsigned int", "guint",
                 "unsigned long", "gulong",
                 "unsigned long long", "guint64",
                 "unsigned short", "gushort",
                 "void", "void");

    return $types{$type} ? $types{$type} : "$name* ";
}

sub IsGDOMClassType {
    my $type = shift;

    return 0 if $type eq "DOMString";
    return 0 if $type eq "CompareHow";
    return 0 if $type eq "float";
    return 0 if $type eq "double";
    return 0 if $type eq "boolean";
    return 0 if $type eq "char";
    return 0 if $type eq "long";
    return 0 if $type eq "short";
    return 0 if $type eq "uchar";
    return 0 if $type eq "unsigned";
    return 0 if $type eq "int";
    return 0 if $type eq "unsigned int";
    return 0 if $type eq "unsigned long";
    return 0 if $type eq "unsigned long long";
    return 0 if $type eq "unsigned short";
    return 0 if $type eq "void";

    return 1;
}

sub GenerateProperties {
    my ($object, $interfaceName, $dataNode) = @_;

    my $clsCaps = substr(ClassNameToGObjectType($className), 12);
    my $lowerCaseIfaceName = "webkit_dom_" . (FixUpDecamelizedName(decamelize($interfaceName)));

    # Properties
    my $implContent = "";

    # Properties
    $implContent = << "EOF";
enum {
    PROP_0,
EOF
    push(@cBodyPriv, $implContent);

    my @txtInstallProps = ();
    my @txtSetProps = ();
    my @txtGetProps = ();

    my $privFunction = GetCoreObject($interfaceName, "coreSelf", "self");

    my $txtGetProp = << "EOF";
static void ${lowerCaseIfaceName}_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    ${className}* self = WEBKIT_DOM_${clsCaps}(object);
    $privFunction

    switch (prop_id) {
EOF
    push(@txtGetProps, $txtGetProp);

    my $txtSetProps = << "EOF";
static void ${lowerCaseIfaceName}_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    ${className} *self = WEBKIT_DOM_${clsCaps}(object);
    $privFunction

    switch (prop_id) {
EOF
    push(@txtSetProps, $txtSetProps);

    # Iterate over the interface attributes and generate a property for
    # each one of them.
  SKIPENUM:
    foreach my $attribute (@{$dataNode->attributes}) {
        if (SkipAttribute($attribute)) {
            next SKIPENUM;
        }

        my $camelPropName = $attribute->signature->name;
        my $setPropNameFunction = $codeGenerator->WK_ucfirst($camelPropName);
        my $getPropNameFunction = $codeGenerator->WK_lcfirst($camelPropName);

        my $propName = decamelize($camelPropName);
        my $propNameCaps = uc($propName);
        $propName =~ s/_/-/g;
        my ${propEnum} = "PROP_${propNameCaps}";
        push(@cBodyPriv, "    ${propEnum},\n");

        my $propType = $attribute->signature->type;
        my ${propGType} = decamelize($propType);
        if ($propGType eq "event_target") {
            $propGType = "event_target_node";
        }
        my ${ucPropGType} = uc($propGType);

        my $gtype = GetGValueTypeName($propType);
        my $gparamflag = "WEBKIT_PARAM_READABLE";
        my $writeable = $attribute->type !~ /^readonly/;
        my $const = "read-only ";
        if ($writeable && $custom) {
            $const = "read-only (due to custom functions needed in webkitdom)";
            next SKIPENUM;
        }
        if ($writeable && !$custom) {
            $gparamflag = "WEBKIT_PARAM_READWRITE";
            $const = "read-write ";
        }

        my $type = GetGlibTypeName($propType);
        $nick = decamelize("${interfaceName}_${propName}");
        $long = "${const} ${type} ${interfaceName}.${propName}";

        my $convertFunction = "";
        if ($gtype eq "string") {
            $convertFunction = "WebCore::String::fromUTF8";
        } elsif ($attribute->signature->extendedAttributes->{"ConvertFromString"}) {
            $convertFunction = "WebCore::String::number";
        }

        my $setterContentHead;
        my $getterContentHead;
        my $reflect = $attribute->signature->extendedAttributes->{"Reflect"};
        my $reflectURL = $attribute->signature->extendedAttributes->{"ReflectURL"};
        if ($reflect || $reflectURL) {
            my $contentAttributeName = (($reflect || $reflectURL) eq "1") ? $camelPropName : ($reflect || $reflectURL);
            my $namespace = $codeGenerator->NamespaceForAttributeName($interfaceName, $contentAttributeName);
            $implIncludes{"${namespace}.h"} = 1;
            my $getAttributeFunctionName = $reflectURL ? "getURLAttribute" : "getAttribute";
            $setterContentHead = "coreSelf->setAttribute(WebCore::${namespace}::${contentAttributeName}Attr, ${convertFunction}(g_value_get_$gtype(value))";
            $getterContentHead = "coreSelf->${getAttributeFunctionName}(WebCore::${namespace}::${contentAttributeName}Attr";
        } else {
            $setterContentHead = "coreSelf->set${setPropNameFunction}(${convertFunction}(g_value_get_$gtype(value))";
            $getterContentHead = "coreSelf->${getPropNameFunction}(";
        }

        if ($writeable && ($gtype eq "boolean" || $gtype eq "float" || $gtype eq "double" ||
                           $gtype eq "uint64" || $gtype eq "ulong" || $gtype eq "long" || 
                           $gtype eq "uint" || $gtype eq "ushort" || $gtype eq "uchar" ||
                           $gtype eq "char" || $gtype eq "string")) {

            push(@txtSetProps, "   case ${propEnum}:\n    {\n");
            push(@txtSetProps, "        WebCore::ExceptionCode ec = 0;\n") if @{$attribute->setterExceptions};
            push(@txtSetProps, "        ${setterContentHead}");
            push(@txtSetProps, ", ec") if @{$attribute->setterExceptions};
            push(@txtSetProps, ");\n");
            push(@txtSetProps, "        break;\n    }\n");
        }

        push(@txtGetProps, "   case ${propEnum}:\n    {\n");

        my $exception = "";
        if (@{$attribute->getterExceptions}) {
            $exception = "ec";
            push(@txtGetProps, "        WebCore::ExceptionCode ec = 0;\n");
        }

        my $postConvertFunction = "";
        my $done = 0;
        if ($gtype eq "string") {
            push(@txtGetProps, "        g_value_take_string(value, convertToUTF8String(${getterContentHead}${exception})));\n");
            $done = 1;
        } elsif ($gtype eq "object") {
            $txtGetProp = << "EOF";
        RefPtr<WebCore::${propType}> ptr = coreSelf->${getPropNameFunction}(${exception});
        g_value_set_object(value, WebKit::kit(ptr.get()));
EOF
            push(@txtGetProps, $txtGetProp);
            $done = 1;
        }

        if($attribute->signature->extendedAttributes->{"ConvertFromString"}) {
            # TODO: Add other conversion functions for different types.  Current
            # IDLs only list longs.
            if($gtype eq "long") {
                $convertFunction = "";
                $postConvertFunction = ".toInt()";
            } else {
                die "Can't convert to type ${gtype}.";
            }
        }

        # FIXME: get rid of this glitch?
        my $_gtype = $gtype;
        if ($gtype eq "ushort") {
            $_gtype = "uint";
        }

        if (!$done) {
            push(@txtGetProps, "        g_value_set_$_gtype(value, ${convertFunction}coreSelf->${getPropNameFunction}(${exception})${postConvertFunction});\n");
        }

        push(@txtGetProps, "        break;\n    }\n");

my %param_spec_options = ("int", "G_MININT, /* min */\nG_MAXINT, /* max */\n0, /* default */",
                          "boolean", "FALSE, /* default */",
                          "float", "G_MINFLOAT, /* min */\nG_MAXFLOAT, /* max */\n0.0, /* default */",
                          "double", "G_MINDOUBLE, /* min */\nG_MAXDOUBLE, /* max */\n0.0, /* default */",
                          "uint64", "0, /* min */\nG_MAXUINT64, /* min */\n0, /* default */",
                          "long", "G_MINLONG, /* min */\nG_MAXLONG, /* max */\n0, /* default */",
                          "ulong", "0, /* min */\nG_MAXULONG, /* max */\n0, /* default */",
                          "uint", "0, /* min */\nG_MAXUINT, /* max */\n0, /* default */",
                          "ushort", "0, /* min */\nG_MAXUINT16, /* max */\n0, /* default */",
                          "uchar", "G_MININT8, /* min */\nG_MAXINT8, /* max */\n0, /* default */",
                          "char", "0, /* min */\nG_MAXUINT8, /* max */\n0, /* default */",
                          "string", "\"\", /* default */",
                          "object", "WEBKIT_TYPE_DOM_${ucPropGType}, /* gobject type */");

        my $txtInstallProp = << "EOF";
    g_object_class_install_property(gobjectClass,
                                    ${propEnum},
                                    g_param_spec_${_gtype}("${propName}", /* name */
                                                           "$nick", /* short description */
                                                           "$long", /* longer - could do with some extra doc stuff here */
                                                           $param_spec_options{$gtype}
                                                           ${gparamflag}));
EOF
        push(@txtInstallProps, $txtInstallProp);
        $txtInstallProp = "/* TODO! $gtype */\n";
    }

    push(@cBodyPriv, "};\n\n");

    $txtGetProp = << "EOF";
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}
EOF
    push(@txtGetProps, $txtGetProp);

    $txtSetProps = << "EOF";
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}
EOF
    push(@txtSetProps, $txtSetProps);

    # TODO: work out if it's appropriate to split this into many different
    # signals e.g. "click" etc.
    my $txtInstallSignals = "";

    $implContent = << "EOF";

static void ${lowerCaseIfaceName}_finalize(GObject* object)
{
    WebKitDOMObject* dom_object = WEBKIT_DOM_OBJECT(object);
    
    if (dom_object->coreObject != NULL) {
        WebCore::${interfaceName}* coreObject = static_cast<WebCore::${interfaceName} *>(dom_object->coreObject);

        WebKit::DOMObjectCache::forget(coreObject);
        coreObject->deref();

        dom_object->coreObject = NULL;
    }

    G_OBJECT_CLASS(${lowerCaseIfaceName}_parent_class)->finalize(object);
}

@txtSetProps

@txtGetProps

static void ${lowerCaseIfaceName}_class_init(${className}Class* requestClass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(requestClass);
    gobjectClass->finalize = ${lowerCaseIfaceName}_finalize;
    gobjectClass->set_property = ${lowerCaseIfaceName}_set_property;
    gobjectClass->get_property = ${lowerCaseIfaceName}_get_property;

@txtInstallProps

$txtInstallSignals
}

static void ${lowerCaseIfaceName}_init(${className}* request)
{
}

EOF
    push(@cBodyPriv, $implContent);
}

sub GenerateHeader {
    my ($object, $interfaceName, $parentClassName) = @_;

    my $implContent = "";

    # Add the default header template
    @hPrefix = split("\r", $licenceTemplate);
    push(@hPrefix, "\n");

    #Header guard
    my $guard = $className . "_h";

    @hPrefixGuard = << "EOF";
#ifndef $guard
#define $guard

EOF

    $implContent = << "EOF";
G_BEGIN_DECLS
EOF

    push(@hBodyPre, $implContent);

    my $decamelize = FixUpDecamelizedName(decamelize($interfaceName));
    my $clsCaps = uc($decamelize);
    my $lowerCaseIfaceName = "webkit_dom_" . ($decamelize);

    $implContent = << "EOF";
#define WEBKIT_TYPE_DOM_${clsCaps}            (${lowerCaseIfaceName}_get_type())
#define WEBKIT_DOM_${clsCaps}(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_DOM_${clsCaps}, ${className}))
#define WEBKIT_DOM_${clsCaps}_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_DOM_${clsCaps}, ${className}Class)
#define WEBKIT_DOM_IS_${clsCaps}(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_DOM_${clsCaps}))
#define WEBKIT_DOM_IS_${clsCaps}_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_DOM_${clsCaps}))
#define WEBKIT_DOM_${clsCaps}_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_DOM_${clsCaps}, ${className}Class))

struct _${className} {
    ${parentClassName} parent_instance;
};

struct _${className}Class {
    ${parentClassName}Class parent_class;
};

WEBKIT_API GType
${lowerCaseIfaceName}_get_type (void);

EOF

    push(@hBody, $implContent);
}

sub getIncludeHeader {
    my $type = shift;
    my $name = GetClassName($type);

    return "" if $type eq "int";
    return "" if $type eq "long";
    return "" if $type eq "short";
    return "" if $type eq "char";
    return "" if $type eq "float";
    return "" if $type eq "double";
    return "" if $type eq "unsigned";
    return "" if $type eq "unsigned int";
    return "" if $type eq "unsigned long";
    return "" if $type eq "unsigned long long";
    return "" if $type eq "unsigned short";
    return "" if $type eq "DOMTimeStamp";
    return "" if $type eq "EventListener";
    return "" if $type eq "unsigned char";
    return "" if $type eq "DOMString";
    return "" if $type eq "float";
    return "" if $type eq "boolean";
    return "" if $type eq "void";
    return "" if $type eq "CompareHow";

    return "$name.h";
}

sub addIncludeInBody {
    my $type = shift;

    if ($type eq "DOMObject") {
        return;
    }

    my $header = getIncludeHeader($type);
    if ($header eq "") {
        return;
    }
    
    if (IsGDOMClassType($type)) {
        $implIncludes{"webkit/$header"} = 1;
    } else {
        $implIncludes{$header} = 1
    }
}

sub GenerateFunction {
    my ($object, $interfaceName, $function, $prefix) = @_;

    if (SkipFunction($function)) {
        return;
    }

    my $functionSigName = $function->signature->name;
    my $functionSigType = $function->signature->type;
    my $decamelize = FixUpDecamelizedName(decamelize($interfaceName));
    my $functionName = "webkit_dom_" . $decamelize . "_" . $prefix . decamelize($functionSigName);
    my $returnType = GetGlibTypeName($functionSigType);
    my $returnValueIsGDOMType = IsGDOMClassType($functionSigType);

    my $functionSig = "$className *self";

    my $callImplParams = "";

    # skip some custom functions for now
    my $isCustomFunction = $function->signature->extendedAttributes->{"Custom"} ||
                       $function->signature->extendedAttributes->{"CustomArgumentHandling"};

    foreach my $param (@{$function->parameters}) {
        my $paramIDLType = $param->type;
        if ($paramIDLType eq "Event" || $paramIDLType eq "EventListener") {
            push(@hBody, "\n/* TODO: event function ${functionName} */\n\n");
            push(@cBody, "\n/* TODO: event function ${functionName} */\n\n");
            return;
        }
        addIncludeInBody($paramIDLType);
        my $paramType = GetGlibTypeName($paramIDLType);
        my $paramName = decamelize($param->name);

        $functionSig .= ", $paramType $paramName";

        my $paramIsGDOMType = IsGDOMClassType($paramIDLType);
        if ($paramIsGDOMType) {
            if ($paramIDLType ne "DOMObject") {
                $implIncludes{"webkit/WebKitDOM${paramIDLType}Private.h"} = 1;
            }
        }
        if ($paramIsGDOMType || ($paramIDLType eq "DOMString") || ($paramIDLType eq "CompareHow")) {
            $paramName = "_g_" . $paramName;
        }
        if ($callImplParams) {
            $callImplParams .= ", $paramName";
        } else {
            $callImplParams = "$paramName";
        }
    }

    if ($returnType ne "void" && $returnValueIsGDOMType && $functionSigType ne "DOMObject") {
        if ($functionSigType ne "EventTarget") {
            $implIncludes{"webkit/WebKitDOM${functionSigType}Private.h"} = 1;
            $implIncludes{"webkit/WebKitDOM${functionSigType}.h"} = 1;
        }

        $implIncludes{"${functionSigType}.h"} = 1;
    }

    # skip custom functions for now
    # but skip from here to allow some headers to be created
    # for a successful compile.
    if ($isCustomFunction &&
        $functionName ne "webkit_dom_node_remove_child" && 
        $functionName ne "webkit_dom_node_insert_before" &&
        $functionName ne "webkit_dom_node_replace_child" &&
        $functionName ne "webkit_dom_node_append_child") {
        push(@hBody, "\n/* TODO: custom function ${functionName} */\n\n");
        push(@cBody, "\n/* TODO: custom function ${functionName} */\n\n");
        return;
    }

    if(@{$function->raisesExceptions}) {
        $functionSig .= ", GError **error";
    }

    push(@hBody, "WEBKIT_API $returnType\n$functionName ($functionSig);\n\n");
    push(@cBody, "$returnType\n$functionName ($functionSig)\n{\n");

    if ($returnType ne "void") {
        # TODO: return proper default result
        push(@cBody, "    g_return_val_if_fail (self, 0);\n");
    } else {
        push(@cBody, "    g_return_if_fail (self);\n");
    }

    # The WebKit::core implementations check for NULL already; no need to
    # duplicate effort.
    push(@cBody, "    WebCore::${interfaceName} * item = WebKit::core(self);\n");

    foreach my $param (@{$function->parameters}) {
        my $paramName = decamelize($param->name);
        my $paramIDLType = $param->type;
        my $paramTypeIsPrimitive = $codeGenerator->IsPrimitiveType($paramIDLType);
        my $paramIsGDOMType = IsGDOMClassType($paramIDLType);
        if (!$paramTypeIsPrimitive) {
            if ($returnType ne "void") {
                # TODO: return proper default result
                push(@cBody, "    g_return_val_if_fail ($paramName, 0);\n");
            } else {
                push(@cBody, "    g_return_if_fail ($paramName);\n");
            }
        }
    }

    $returnParamName = "";
    foreach my $param (@{$function->parameters}) {
        my $paramIDLType = $param->type;
        my $paramName = decamelize($param->name);

        my $paramIsGDOMType = IsGDOMClassType($paramIDLType);
        if ($paramIDLType eq "DOMString") {
            push(@cBody, "    WebCore::String _g_${paramName} = WebCore::String::fromUTF8($paramName);\n");
        } elsif ($paramIDLType eq "CompareHow") {
            push(@cBody, "    WebCore::Range::CompareHow _g_${paramName} = static_cast<WebCore::Range::CompareHow>($paramName);\n");
        } elsif ($paramIsGDOMType) {
            push(@cBody, "    WebCore::${paramIDLType} * _g_${paramName} = WebKit::core($paramName);\n");
            if ($returnType ne "void") {
                # TODO: return proper default result
                push(@cBody, "    g_return_val_if_fail (_g_${paramName}, 0);\n");
            } else {
                push(@cBody, "    g_return_if_fail (_g_${paramName});\n");
            }
        }
        $returnParamName = "_g_".$paramName if $param->extendedAttributes->{"Return"};
    }

    my $assign = "";
    my $assignPre = "";
    my $assignPost = "";

    if ($returnType ne "void" && !$isCustomFunction) {
        if ($returnValueIsGDOMType) {
            $assign = "PassRefPtr<WebCore::${functionSigType}> g_res = ";
            $assignPre = "WTF::getPtr(";
            $assignPost = ")";
        } else {
            $assign = "${returnType} res = ";
        }
    }
    my $exceptions = "";
    if (@{$function->raisesExceptions}) {
        push(@cBody, "    WebCore::ExceptionCode ec = 0;\n");
        if (${callImplParams} ne "") {
            $exceptions = ", ec";
        } else {
            $exceptions = "ec";
        }
    }

    # We need to special-case these Node methods because their C++ signature is different
    # from what we'd expect given their IDL description; see Node.h.
    if ($functionName eq "webkit_dom_node_append_child" ||
        $functionName eq "webkit_dom_node_insert_before" ||
        $functionName eq "webkit_dom_node_replace_child" ||
        $functionName eq "webkit_dom_node_remove_child") {
        my $customNodeAppendChild = << "EOF";
    bool ok = item->${functionSigName}(${callImplParams}${exceptions});
    if (ok)
    {
        ${returnType} res = static_cast<${returnType}>(WebKit::kit($returnParamName));
        return res;
    }
EOF
        push(@cBody, $customNodeAppendChild);
    
        if(@{$function->raisesExceptions}) {
            my $exceptionHandling = << "EOF";

    WebCore::ExceptionCodeDescription ecdesc;
    WebCore::getExceptionCodeDescription(ec, ecdesc);
    g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
EOF
            push(@cBody, $exceptionHandling);
        }
        push(@cBody, "return NULL;");
        push(@cBody, "}\n\n");
        return;
    } elsif ($functionSigType eq "DOMString") {
        my $getterContentHead;
        my $reflect = $function->signature->extendedAttributes->{"Reflect"};
        my $reflectURL = $function->signature->extendedAttributes->{"ReflectURL"};
        if ($reflect || $reflectURL) {
            my $contentAttributeName = (($reflect || $reflectURL) eq "1") ? $functionSigName : ($reflect || $reflectURL);
            my $namespace = $codeGenerator->NamespaceForAttributeName($interfaceName, $contentAttributeName);
            $implIncludes{"${namespace}.h"} = 1;
            my $getAttributeFunctionName = $reflectURL ? "getURLAttribute" : "getAttribute";
            $getterContentHead = "${assign}convertToUTF8String(item->${getAttributeFunctionName}(WebCore::${namespace}::${contentAttributeName}Attr));\n";
        } else {
            $getterContentHead = "${assign}convertToUTF8String(item->${functionSigName}(${callImplParams}${exceptions}));\n";
        }

        push(@cBody, "    ${getterContentHead}");
    } else {
        my $setterContentHead;
        my $reflect = $function->signature->extendedAttributes->{"Reflect"};
        my $reflectURL = $function->signature->extendedAttributes->{"ReflectURL"};
        if ($reflect || $reflectURL) {
            my $contentAttributeName = (($reflect || $reflectURL) eq "1") ? $functionSigName : ($reflect || $reflectURL);
            $contentAttributeName =~ s/set//;
            $contentAttributeName = $codeGenerator->WK_lcfirst($contentAttributeName);
            my $namespace = $codeGenerator->NamespaceForAttributeName($interfaceName, $contentAttributeName);
            $implIncludes{"${namespace}.h"} = 1;
            $setterContentHead = "${assign}${assignPre}item->setAttribute(WebCore::${namespace}::${contentAttributeName}Attr, ${callImplParams}${exceptions}${assignPost});\n";
        } else {
            $setterContentHead = "${assign}${assignPre}item->${functionSigName}(${callImplParams}${exceptions}${assignPost});\n";
        }

        push(@cBody, "    ${setterContentHead}");
        
        if(@{$function->raisesExceptions}) {
            my $exceptionHandling = << "EOF";
    if (ec) {
        WebCore::ExceptionCodeDescription ecdesc;
        WebCore::getExceptionCodeDescription(ec, ecdesc);
        g_set_error_literal(error, g_quark_from_string("WEBKIT_DOM"), ecdesc.code, ecdesc.name);
    }
EOF
            push(@cBody, $exceptionHandling);
        }
    }

    if ($returnType ne "void" && !$isCustomFunction) {
        if ($functionSigType ne "DOMObject") {
            if ($returnValueIsGDOMType) {
                push(@cBody, "    ${returnType} res = static_cast<${returnType}>(WebKit::kit(g_res.get()));\n");
            }
        }
        if ($functionSigType eq "DOMObject") {
            push(@cBody, "    return NULL; /* TODO: return canvas object */\n");
        } else {
            push(@cBody, "    return res;\n");
        }
    }
    push(@cBody, "\n}\n\n");
}

sub ClassHasFunction {
    my ($class, $name) = @_;

    foreach my $function (@{$class->functions}) {
        if ($function->signature->name eq $name) {
            return 1;
        }
    }

    return 0;
}

sub GenerateFunctions {
    my ($object, $interfaceName, $dataNode) = @_;

    foreach my $function (@{$dataNode->functions}) {
        $object->GenerateFunction($interfaceName, $function, "");
    }

    TOP:
    foreach my $attribute (@{$dataNode->attributes}) {
        if (SkipAttribute($attribute)) {
            next TOP;
        }
        
        if ($attribute->signature->name eq "type"
            # This will conflict with the get_type() function we define to return a GType
            # according to GObject conventions.  Skip this for now.
            || $attribute->signature->name eq "URL"     # TODO: handle this
            || $attribute->signature->extendedAttributes->{"ConvertFromString"}    # TODO: handle this
            ) {
            next TOP;
        }
            
        my $attrNameUpper = $codeGenerator->WK_ucfirst($attribute->signature->name);
        my $getname = "get${attrNameUpper}";
        my $setname = "set${attrNameUpper}";
        if (ClassHasFunction($dataNode, $getname) || ClassHasFunction($dataNode, $setname)) {
            # Very occasionally an IDL file defines getter/setter functions for one of its
            # attributes; in this case we don't need to autogenerate the getter/setter.
            next TOP;
        }
        
        # Generate an attribute getter.  For an attribute "foo", this is a function named
        # "get_foo" which calls a DOM class method named foo().
        my $function = new domFunction();
        $function->signature($attribute->signature);
        $function->raisesExceptions($attribute->getterExceptions);
        $object->GenerateFunction($interfaceName, $function, "get_");
        
        if ($attribute->type =~ /^readonly/) {
            next TOP;
        }
        
        # Generate an attribute setter.  For an attribute, "foo", this is a function named
        # "set_foo" which calls a DOM class method named setFoo().
        $function = new domFunction();
        
        $function->signature(new domSignature());
        $function->signature->name($setname);
        $function->signature->type("void");
        $function->signature->extendedAttributes($attribute->signature->extendedAttributes);
        
        my $param = new domSignature();
        $param->name("value");
        $param->type($attribute->signature->type);
        my %attributes = ();
        $param->extendedAttributes(attributes);
        my $arrayRef = $function->parameters;
        push(@$arrayRef, $param);
        
        $function->raisesExceptions($attribute->setterExceptions);
        
        $object->GenerateFunction($interfaceName, $function, "");
    }
}

sub GenerateCFile {
    my ($object, $interfaceName, $parentClassName, $parentGObjType, $dataNode) = @_;
    my $implContent = "";

    my $clsCaps = uc(FixUpDecamelizedName(decamelize($interfaceName)));
    my $lowerCaseIfaceName = "webkit_dom_" . FixUpDecamelizedName(decamelize($interfaceName));

    $implContent = << "EOF";
G_DEFINE_TYPE(${className}, ${lowerCaseIfaceName}, ${parentGObjType})

namespace WebKit {

${className}* wrap${interfaceName}(WebCore::${interfaceName}* coreObject)
{
    g_return_val_if_fail(coreObject != 0, 0);
    
    ${className}* wrapper = WEBKIT_DOM_${clsCaps}(g_object_new(WEBKIT_TYPE_DOM_${clsCaps}, NULL));
    g_return_val_if_fail(wrapper != 0, 0);

    /* We call ref() rather than using a C++ smart pointer because we can't store a C++ object
     * in a C-allocated GObject structure.  See the finalize() code for the
     * matching deref().
     */

    coreObject->ref();
    WEBKIT_DOM_OBJECT(wrapper)->coreObject = coreObject;

    return wrapper;
}

WebCore::${interfaceName}* core(${className}* request)
{
    g_return_val_if_fail(request != 0, 0);
    
    WebCore::${interfaceName}* coreObject = static_cast<WebCore::${interfaceName}*>(WEBKIT_DOM_OBJECT(request)->coreObject);
    g_return_val_if_fail(coreObject != 0, 0);
    
    return coreObject;
}

} // namespace WebKit
EOF

    push(@cBodyPriv, $implContent);
    $object->GenerateProperties($interfaceName, $dataNode);
    $object->GenerateFunctions($interfaceName, $dataNode);
}

sub GenerateEndHeader {
    my ($object) = @_;

    #Header guard
    my $guard = $className . "_h";

    push(@hBody, "G_END_DECLS\n\n");
    push(@hBody, "#endif /* $guard */\n");
}

sub GeneratePrivateHeader {
    my $object = shift;
    my $dataNode = shift;

    my $interfaceName = $dataNode->name;
    my $filename = "$outputDir/" . $className . "Private.h";
    my $guard = uc(decamelize($className)) . "_PRIVATE_H";
    my $parentClassName = GetParentClassName($dataNode);
    my $hasLegacyParent = $dataNode->extendedAttributes->{"LegacyParent"};
    my $hasRealParent = @{$dataNode->parents} > 0;
    my $hasParent = $hasLegacyParent || $hasRealParent;
    
    open(PRIVHEADER, ">$filename") or die "Couldn't open file $filename for writing";
    
    print PRIVHEADER split("\r", $licenceTemplate);
    print PRIVHEADER "\n";
    
    my $text = << "EOF";
#ifndef $guard
#define $guard

#include <glib-object.h>
#include <webkit/${parentClassName}.h>
#include "${interfaceName}.h"
EOF

    print PRIVHEADER $text;
    
    print PRIVHEADER map { "#include \"$_\"\n" } sort keys(%hdrPropIncludes);
    print PRIVHEADER "\n" if keys(%hdrPropIncludes);
    
    $text = << "EOF";
namespace WebKit {
    ${className} *
    wrap${interfaceName}(WebCore::${interfaceName} *coreObject);

    WebCore::${interfaceName} *
    core(${className} *request);

EOF

    print PRIVHEADER $text;

    if ($className ne "WebKitDOMNode") {
        $text = << "EOF";
    gpointer
    kit(WebCore::${interfaceName}* node);

EOF
        print PRIVHEADER $text;
    }

    $text = << "EOF";
} // namespace WebKit

#endif /* ${guard} */
EOF
    print PRIVHEADER $text;
    
    close(PRIVHEADER);
}

sub UsesManualKitImplementation {
    my $type = shift;

    return 1 if $type eq "Node" or $type eq "Element";
    return 0;
}

sub Generate {
    my ($object, $dataNode) = @_;

    my $hasLegacyParent = $dataNode->extendedAttributes->{"LegacyParent"};
    my $hasRealParent = @{$dataNode->parents} > 0;
    my $hasParent = $hasLegacyParent || $hasRealParent;
    my $parentClassName = GetParentClassName($dataNode);
    my $parentGObjType = GetParentGObjType($dataNode);
    my $interfaceName = $dataNode->name;

    # Add the default impl header template
    @cPrefix = split("\r", $licenceTemplate);
    push(@cPrefix, "\n");

    $implIncludes{"webkitmarshal.h"} = 1;
    $implIncludes{"webkitprivate.h"} = 1;
    $implIncludes{"WebKitDOMBinding.h"} = 1;
    $implIncludes{"gobject/ConvertToUTF8String.h"} = 1;
    $implIncludes{"webkit/$className.h"} = 1;
    $implIncludes{"webkit/${className}Private.h"} = 1;
    $implIncludes{"${interfaceName}.h"} = 1;
    $implIncludes{"ExceptionCode.h"} = 1;

    $hdrIncludes{"webkit/${parentClassName}.h"} = 1;

    if (!UsesManualKitImplementation($interfaceName)) {
        my $converter = << "EOF";
namespace WebKit {
    
gpointer kit(WebCore::$interfaceName* obj)
{
    g_return_val_if_fail(obj != 0, 0);

    if (gpointer ret = DOMObjectCache::get(obj))
        return ret;

    return DOMObjectCache::put(obj, WebKit::wrap${interfaceName}(obj));
}
    
} // namespace WebKit //

EOF
    push(@cBody, $converter);
    }

    $object->GenerateHeader($interfaceName, $parentClassName);
    $object->GenerateCFile($interfaceName, $parentClassName, $parentGObjType, $dataNode);
    $object->GenerateEndHeader();
    $object->GeneratePrivateHeader($dataNode);
}

# Internal helper
sub WriteData {
    my ($object, $name) = @_;

    # Write public header.
    my $hdrFName = "$outputDir/" . $name . ".h";
    open(HEADER, ">$hdrFName") or die "Couldn't open file $hdrFName";

    print HEADER @hPrefix;
    print HEADER @hPrefixGuard;
    print HEADER "#include \"webkit/webkitdomdefines.h\"\n";
    print HEADER "#include <glib-object.h>\n";
    print HEADER "#include <webkit/webkitdefines.h>\n";
    print HEADER map { "#include \"$_\"\n" } sort keys(%hdrIncludes);
    print HEADER "\n" if keys(%hdrIncludes);
    print HEADER "\n";
    print HEADER @hBodyPre;
    print HEADER @hBody;

    close(HEADER);

    # Write the implementation sources
    my $implFileName = "$outputDir/" . $name . ".cpp";
    open(IMPL, ">$implFileName") or die "Couldn't open file $implFileName";

    print IMPL @cPrefix;
    print IMPL "#include <glib-object.h>\n";
    print IMPL "#include \"config.h\"\n\n";
    print IMPL "#include <wtf/GetPtr.h>\n";
    print IMPL "#include <wtf/RefPtr.h>\n";
    print IMPL map { "#include \"$_\"\n" } sort keys(%implIncludes);
    print IMPL "\n" if keys(%implIncludes);
    print IMPL @cBody;

    print IMPL "\n";
    print IMPL @cBodyPriv;

    close(IMPL);

    %implIncludes = ();
    %hdrIncludes = ();
    @hPrefix = ();
    @hBody = ();

    @cPrefix = ();
    @cBody = ();
    @cBodyPriv = ();
}

sub GenerateInterface {
    my ($object, $dataNode, $defines) = @_;
    my $name = $dataNode->name;

    # Set up some global variables
    $className = GetClassName($dataNode->name);
    $object->Generate($dataNode);

    # Write changes
    my $fname = "WebKitDOM_" . $name;
    $fname =~ s/_//g;
    $object->WriteData($fname);
}
