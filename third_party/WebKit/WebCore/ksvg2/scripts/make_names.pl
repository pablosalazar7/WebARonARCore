#!/usr/bin/perl -w

# Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer. 
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

use strict;
use Getopt::Long;
use File::Path;

my $printFactory = 0;
my $cppNamespace = "";
my $namespace = "";
my $namespaceURI = "";
my $tagsFile = "";
my $attrsFile = "";
my $outputDir = ".";
my @tags = ();
my @attrs = ();
my $tagsNullNamespace = 0;
my $attrsNullNamespace = 0;

GetOptions('tags=s' => \$tagsFile, 
    'attrs=s' => \$attrsFile,
    'outputDir=s' => \$outputDir,
    'namespace=s' => \$namespace,
    'namespaceURI=s' => \$namespaceURI,
    'cppNamespace=s' => \$cppNamespace,
    'factory' => \$printFactory,
    'tagsNullNamespace' => \$tagsNullNamespace,
    'attrsNullNamespace' => \$attrsNullNamespace,);

die "You must specify a namespace (e.g. SVG) for <namespace>Names.h" unless $namespace;
die "You must specify a namespaceURI (e.g. http://www.w3.org/2000/svg)" unless $namespaceURI;
die "You must specify a cppNamespace (e.g. DOM) used for <cppNamespace>::<namespace>Names::fooTag" unless $cppNamespace;
die "You must specify at least one of --tags <file> or --attrs <file>" unless (length($tagsFile) || length($attrsFile));

@tags = readNames($tagsFile) if length($tagsFile);
@attrs = readNames($attrsFile) if length($attrsFile);

mkpath($outputDir);
my $namesBasePath = "$outputDir/${namespace}Names";
my $factoryBasePath = "$outputDir/${namespace}ElementFactory";

printNamesHeaderFile("$namesBasePath.h");
printNamesCppFile("$namesBasePath.cpp");
if ($printFactory) {
	printFactoryCppFile("$factoryBasePath.cpp");
	printFactoryHeaderFile("$factoryBasePath.h");
}


## Support routines

sub readNames
{
	my $namesFile = shift;
	
	die "Failed to open file: $namesFile" unless open(NAMES, "<", $namesFile);
	my @names = ();
	while (<NAMES>) {
		next if (m/#/);
		s/-/_/g;
		chomp $_;
		push @names, $_;
	}	
	close(NAMES);
	
	die "Failed to read names from file: $namesFile" unless (scalar(@names));
	
	return @names
}

sub printMacros
{
	my @names = @_;
	for my $name (@names) {
		print "    macro($name) \\\n";
	}
}

sub printConstructors
{
	my @names = @_;
	for my $name (@names) {
		my $upperCase = upperCaseName($name);
	
		print "${namespace}ElementImpl *${name}Constructor(DocumentImpl *doc, bool createdByParser)\n";
		print "{\n";
		print "    return new ${namespace}${upperCase}ElementImpl(${name}Tag, doc);\n";
		print "}\n\n";
	}
}

sub printFunctionInits
{
	my @names = @_;
	for my $name (@names) {
		print "    gFunctionMap->set(${name}Tag.localName().impl(), (void*)&${name}Constructor);\n";
	}
}

sub svgCapitalizationHacks
{
	my $name = shift;
	
	if ($name =~ /^fe(.+)$/) {
		$name = "FE" . ucfirst $1;
	}
	$name =~ s/kern/Kern/;
	$name =~ s/mpath/MPath/;
	$name =~ s/svg/SVG/;
	$name =~ s/tref/TRef/;
	$name =~ s/tspan/TSpan/;
	$name =~ s/uri/URI/;
	
	return $name;
}

sub upperCaseName
{
	my $name = shift;
	
	$name = svgCapitalizationHacks($name) if ($namespace eq "SVG");
	
	while ($name =~ /^(.*?)_(.*)/) {
		$name = $1 . ucfirst $2;
	}
	
	return ucfirst $name;
}

sub printLicenseHeader
{
	print "/*
 * THIS FILE IS AUTOMATICALLY GENERATED, DO NOT EDIT.
 *
 *
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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


";
}

sub printNamesHeaderFile
{
	my $headerPath = shift;
	redirectSTDOUT($headerPath);
	
	printLicenseHeader();
	print "#ifndef DOM_${namespace}NAMES_H\n";
	print "#define DOM_${namespace}NAMES_H\n\n";
	print "#include \"dom_qname.h\"\n\n";
	
	print "namespace $cppNamespace { namespace ${namespace}Names {\n\n";
	
	if (scalar(@tags)) {
		print"#define DOM_${namespace}NAMES_FOR_EACH_TAG(macro) \\\n";
		printMacros(@tags);
		print"// end of macro\n\n";
	}
	if (scalar(@attrs)) {
		print "#define DOM_${namespace}NAMES_FOR_EACH_ATTR(macro) \\\n";
		printMacros(@attrs);
		print "// end of macro\n\n";
	}
	
	my $lowerNamespace = lc($namespace);
	print "#if !DOM_${namespace}NAMES_HIDE_GLOBALS\n";
    print "// Namespace\n";
    print "extern const DOM::AtomicString ${lowerNamespace}NamespaceURI;\n\n";

	if (scalar(@tags)) {
    	print "// Tags\n";
		print "#define DOM_NAMES_DEFINE_TAG_GLOBAL(name) extern const DOM::QualifiedName name##Tag;\n";
		print "DOM_${namespace}NAMES_FOR_EACH_TAG(DOM_NAMES_DEFINE_TAG_GLOBAL)\n";
		print "#undef DOM_NAMES_DEFINE_TAG_GLOBAL\n\n";
    }
	
	if (scalar(@attrs)) {
		print "// Attributes\n";
		print "#define DOM_NAMES_DEFINE_ATTR_GLOBAL(name) extern const DOM::QualifiedName name##Attr;\n";
		print "DOM_${namespace}NAMES_FOR_EACH_ATTR(DOM_NAMES_DEFINE_ATTR_GLOBAL)\n";
		print "#undef DOM_NAMES_DEFINE_ATTR_GLOBAL\n\n";
    }
	print "#endif\n\n";
	print "void init();\n\n";
	print "} }\n\n";
	print "#endif\n\n";
	
	restoreSTDOUT();
}

sub printNamesCppFile
{
	my $cppPath = shift;
	redirectSTDOUT($cppPath);
	
	printLicenseHeader();
	
	my $lowerNamespace = lc($namespace);

print "#define DOM_${namespace}NAMES_HIDE_GLOBALS 1\n\n";

print "#include \"config.h\"\n";
print "#include \"${namespace}Names.h\"\n\n";

print "namespace $cppNamespace { namespace ${namespace}Names {

using namespace KDOM;

// Define a properly-sized array of pointers to avoid static initialization.
// Use an array of pointers instead of an array of char in case there is some alignment issue.

#define DEFINE_UNINITIALIZED_GLOBAL(type, name) void *name[(sizeof(type) + sizeof(void *) - 1) / sizeof(void *)];

DEFINE_UNINITIALIZED_GLOBAL(AtomicString, ${lowerNamespace}NamespaceURI)
";

	if (scalar(@tags)) {
		print "#define DEFINE_TAG_GLOBAL(name) DEFINE_UNINITIALIZED_GLOBAL(QualifiedName, name##Tag)\n";
		print "DOM_${namespace}NAMES_FOR_EACH_TAG(DEFINE_TAG_GLOBAL)\n\n";
	}

	if (scalar(@attrs)) {
		print "#define DEFINE_ATTR_GLOBAL(name) DEFINE_UNINITIALIZED_GLOBAL(QualifiedName, name##Attr)\n";
		print "DOM_${namespace}NAMES_FOR_EACH_ATTR(DEFINE_ATTR_GLOBAL)\n\n";
	}

print "void init()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    
    // Use placement new to initialize the globals.
";
	
    print("    AtomicString ${lowerNamespace}NS(\"$namespaceURI\");\n\n");

    print("    // Namespace\n");
    print("    new (&${lowerNamespace}NamespaceURI) AtomicString(${lowerNamespace}NS);\n\n");
    if (scalar(@tags)) {
    	my $tagsNamespace = $tagsNullNamespace ? "nullAtom" : "${lowerNamespace}NS";
		printDefinitions(\@tags, "tags", $tagsNamespace);
	}
	if (scalar(@attrs)) {
    	my $attrsNamespace = $attrsNullNamespace ? "nullAtom" : "${lowerNamespace}NS";
		printDefinitions(\@attrs, "attributes", $attrsNamespace);
	}

	print "}\n\n} }";
	restoreSTDOUT();
}

sub printElementIncludes
{
	my @names = @_;
	for my $name (@names) {
		my $upperCase = upperCaseName($name);
		print "#include \"${namespace}${upperCase}ElementImpl.h\"\n";
	}
}

sub printDefinitions
{
	my ($namesRef, $type, $namespaceURI) = @_;
	my $singularType = substr($type, 0, -1);
	my $shortType = substr($singularType, 0, 4);
	my $shortCamelType = ucfirst($shortType);
	my $shortUpperType = uc($shortType);
	
	print "    // " . ucfirst($type) . "\n";
	print "    #define DEFINE_${shortUpperType}_STRING(name) const char *name##${shortCamelType}String = #name;\n";
	print "    DOM_${namespace}NAMES_FOR_EACH_${shortUpperType}(DEFINE_${shortUpperType}_STRING)\n\n";
	for my $name (@$namesRef) {
		if ($name =~ /_/) {
			my $realName = $name;
			$realName =~ s/_/-/g;
			print "    ${name}${shortCamelType}String = \"$realName\";\n";
		}
	}
	print "\n    #define INITIALIZE_${shortUpperType}_GLOBAL(name) new (&name##${shortCamelType}) QualifiedName(nullAtom, name##${shortCamelType}String, $namespaceURI);\n";
	print "    DOM_${namespace}NAMES_FOR_EACH_${shortUpperType}(INITIALIZE_${shortUpperType}_GLOBAL)\n\n";
}

my $savedSTDOUT;

sub redirectSTDOUT
{
	my $filepath = shift;
	print "Writing $filepath...\n";
	open $savedSTDOUT, ">&STDOUT" or die "Can't save STDOUT";
	open(STDOUT, ">", $filepath) or die "Failed to open file: $filepath";
}

sub restoreSTDOUT
{
	open STDOUT, ">&", $savedSTDOUT or die "Can't restor STDOUT: \$oldout: $!";
}

sub printFactoryCppFile
{
	my $cppPath = shift;
	redirectSTDOUT($cppPath);

printLicenseHeader();

print "#include \"config.h\"\n\n";
print "#include \"${namespace}ElementFactory.h\"\n";
print "#include \"${namespace}Names.h\"\n\n";

printElementIncludes(@tags);

print "\n\n#include <kxmlcore/HashMap.h>\n\n";

print "using namespace KDOM;\n";
print "using namespace ${cppNamespace}::${namespace}Names;\n\n";

print "typedef KXMLCore::HashMap<DOMStringImpl *, void *, KXMLCore::PointerHash<DOMStringImpl *> > FunctionMap;\n";
print "static FunctionMap *gFunctionMap = 0;\n\n";

print "namespace ${cppNamespace}\n{\n\n";

print "typedef ${namespace}ElementImpl *(*ConstructorFunc)(DocumentImpl *doc, bool createdByParser);\n\n";

printConstructors(@tags);

print "
static inline void createFunctionMapIfNecessary()
{
    if (gFunctionMap)
        return;
    // Create the table.
    gFunctionMap = new FunctionMap;
    
    // Populate it with constructor functions.
";

printFunctionInits(@tags);

print "}\n";

print "
${namespace}ElementImpl *${namespace}ElementFactory::create${namespace}Element(const QualifiedName& qName, DocumentImpl* doc, bool createdByParser)
{
    if (!doc)
        return 0; // Don't allow elements to ever be made without having a doc.

    createFunctionMapIfNecessary();
    void* result = gFunctionMap->get(qName.localName().impl());
    if (result) {
        ConstructorFunc func = (ConstructorFunc)result;
        return (func)(doc, createdByParser);
    }
    
    return new ${namespace}ElementImpl(qName, doc);
}

}; // namespace

";
	restoreSTDOUT();
}

sub printFactoryHeaderFile
{
	my $headerPath = shift;
	redirectSTDOUT($headerPath);

	printLicenseHeader();

print "#ifndef ${namespace}ELEMENTFACTORY_H\n";
print "#define ${namespace}ELEMENTFACTORY_H\n\n";

print "
namespace KDOM {
    class ElementImpl;
    class DocumentImpl;
    class QualifiedName;
    class AtomicString;
}

namespace ${cppNamespace}
{
    class ${namespace}ElementImpl;

    // The idea behind this class is that there will eventually be a mapping from namespace URIs to ElementFactories that can dispense
    // elements.  In a compound document world, the generic createElement function (will end up being virtual) will be called.
    class ${namespace}ElementFactory
    {
    public:
        KDOM::ElementImpl *createElement(const KDOM::QualifiedName& qName, KDOM::DocumentImpl *doc, bool createdByParser = true);
        static ${namespace}ElementImpl *create${namespace}Element(const KDOM::QualifiedName& qName, KDOM::DocumentImpl *doc, bool createdByParser = true);
    };
}

#endif

";

	restoreSTDOUT();
}


