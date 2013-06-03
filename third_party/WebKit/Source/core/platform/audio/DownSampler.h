/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DownSampler_h
#define DownSampler_h

#include "core/platform/audio/AudioArray.h"
#include "core/platform/audio/DirectConvolver.h"

namespace WebCore {

// DownSampler down-samples the source stream by a factor of 2x.

class DownSampler {
public:
    DownSampler(size_t inputBlockSize);

    // The destination buffer |destP| is of size sourceFramesToProcess / 2.
    void process(const float* sourceP, float* destP, size_t sourceFramesToProcess);

    void reset();

    // Latency based on the destination sample-rate.
    size_t latencyFrames() const;

private:
    enum { DefaultKernelSize = 256 };

    size_t m_inputBlockSize;

    // Computes ideal band-limited half-band filter coefficients.
    // In other words, filter out all frequencies higher than 0.25 * Nyquist.
    void initializeKernel();
    AudioFloatArray m_reducedKernel;

    // Half-band filter.
    DirectConvolver m_convolver;

    AudioFloatArray m_tempBuffer;

    // Used as delay-line (FIR filter history) for the input samples to account for the 0.5 term right in the middle of the kernel.
    AudioFloatArray m_inputBuffer;
};

} // namespace WebCore

#endif // DownSampler_h
