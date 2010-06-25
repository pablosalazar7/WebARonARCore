/*
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
 * Copyright (C) 2005, 2006 Apple Computer, Inc.
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2010 Google, Inc.
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

#ifndef DocumentParser_h
#define DocumentParser_h

#include <wtf/Noncopyable.h>

namespace WebCore {

class Document;
class DocumentWriter;
class LegacyHTMLTreeBuilder;
class LegacyHTMLDocumentParser;
class SegmentedString;
class XSSAuditor;

class DocumentParser : public Noncopyable {
public:
    virtual ~DocumentParser() { }

    // insert is used by document.write
    virtual void insert(const SegmentedString&) = 0;
    // appendBytes is used by DocumentWriter (the loader)
    virtual void appendBytes(DocumentWriter*, const char* bytes, int length, bool flush);

    // FIXME: append() should be private, but DocumentWriter::replaceDocument
    // uses it for now.
    virtual void append(const SegmentedString&) = 0;

    virtual void finish() = 0;
    virtual bool finishWasCalled() = 0;

    virtual bool isWaitingForScripts() const = 0;
    virtual bool isExecutingScript() const { return false; }

    virtual void stopParsing() { m_parserStopped = true; }
    // FIXME: processingData() is only used by DocumentLoader::isLoadingInAPISense
    // and is very unclear as to what it actually means.  Only LegacyHTMLDocumentParser
    // actually implements it.
    virtual bool processingData() const { return false; }

    virtual bool wellFormed() const { return true; }

    virtual int lineNumber() const { return -1; }
    virtual int columnNumber() const { return -1; }

    virtual void executeScriptsWaitingForStylesheets() {}

    virtual LegacyHTMLTreeBuilder* htmlTreeBuilder() const { return 0; }
    virtual LegacyHTMLDocumentParser* asHTMLDocumentParser() { return 0; }

    // FIXME: view source mode is only used by the HTML and Text
    // DocumentParsers and may not belong on the base-class.
    bool inViewSourceMode() const { return m_inViewSourceMode; }
    void setInViewSourceMode(bool mode) { m_inViewSourceMode = mode; }

    Document* document() const { return m_document; }

    XSSAuditor* xssAuditor() const { return m_XSSAuditor; }
    void setXSSAuditor(XSSAuditor* auditor) { m_XSSAuditor = auditor; }

protected:
    DocumentParser(Document*, bool viewSourceMode = false);

    // The parser has buffers, so parsing may continue even after
    // it stops receiving data. We use m_parserStopped to stop the parser
    // even when it has buffered data.
    bool m_parserStopped;
    bool m_inViewSourceMode;

    // Every DocumentParser needs a pointer back to the document.
    Document* m_document;
    // The XSSAuditor associated with this document parser.
    XSSAuditor* m_XSSAuditor;
};

} // namespace WebCore

#endif // DocumentParser_h
