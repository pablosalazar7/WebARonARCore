/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (c) 2010, Google Inc. All rights reserved.
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

#ifndef WebGLTypedArrayBase_h
#define WebGLTypedArrayBase_h

#include "WebGLArray.h"
#include "WebGLArrayBuffer.h"

namespace WebCore {

template <typename T>
class WebGLTypedArrayBase : public WebGLArray {
  public:
    T* data() const { return static_cast<T*>(baseAddress()); }

    void set(WebGLTypedArrayBase<T>* array, unsigned offset, ExceptionCode& ec)
    {
        setImpl(array, offset * sizeof(T), ec);
    }

    // Overridden from WebGLArray. This must be public because of
    // rules about inheritance of members in template classes, and
    // because it is accessed via pointers to subclasses.
    virtual unsigned length() const
    {
        return m_length;
    }

  protected:
    WebGLTypedArrayBase(PassRefPtr<WebGLArrayBuffer> buffer, unsigned byteOffset, unsigned length)
        : WebGLArray(buffer, byteOffset)
        , m_length(length)
    {
    }

    template <class Subclass>
    static PassRefPtr<Subclass> create(unsigned length)
    {
        RefPtr<WebGLArrayBuffer> buffer = WebGLArrayBuffer::create(length, sizeof(T));
        return create<Subclass>(buffer, 0, length);
    }

    template <class Subclass>
    static PassRefPtr<Subclass> create(T* array, unsigned length)
    {
        RefPtr<Subclass> a = create<Subclass>(length);
        for (unsigned i = 0; i < length; ++i)
            a->set(i, array[i]);
        return a;
    }

    template <class Subclass>
    static PassRefPtr<Subclass> create(PassRefPtr<WebGLArrayBuffer> buffer,
                                       unsigned byteOffset,
                                       unsigned length)
    {
        RefPtr<WebGLArrayBuffer> buf(buffer);
        if (!verifySubRange<T>(buf, byteOffset, length))
            return 0;

        return adoptRef(new Subclass(buf, byteOffset, length));
    }

    template <class Subclass>
    PassRefPtr<Subclass> sliceImpl(int start, int end) const
    {
        unsigned offset, length;
        calculateOffsetAndLength(start, end, m_length, &offset, &length);
        clampOffsetAndNumElements<T>(buffer(), m_byteOffset, &offset, &length);
        return create<Subclass>(buffer(), offset, length);
    }

    // We do not want to have to access this via a virtual function in subclasses,
    // which is why it is protected rather than private.
    unsigned m_length;

  private:
    // Overridden from WebGLArray.
    virtual unsigned byteLength() const
    {
        return m_length * sizeof(T);
    }
};

} // namespace WebCore

#endif // WebGLTypedArrayBase_h
