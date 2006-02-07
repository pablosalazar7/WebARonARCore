/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef TIMER_H
#define TIMER_H

namespace WebCore {

    // Time intervals are all in seconds.

    class TimerBase {
    public:
        TimerBase();
        virtual ~TimerBase();

        void start(double nextFireInterval, double repeatInterval);

        void startRepeating(double repeatInterval);
        void startOneShot(double interval);

        void stop();
        bool isActive() const;

        double nextFireInterval() const;
        double repeatInterval() const;

        void fire();

    private:
        virtual void fired() = 0;

        TimerBase(const TimerBase&);
        TimerBase& operator=(const TimerBase&);

#if __APPLE__
        CFRunLoopTimerRef m_runLoopTimer;
#else #if WIN32
        int m_timerID;
        bool m_active;
#endif
    };

    template <typename TimerFiredClass> class Timer : public TimerBase {
    public:
        typedef void (TimerFiredClass::*TimerFiredFunction)(Timer*);

        Timer(TimerFiredClass* o, TimerFiredFunction f)
            : m_object(o), m_function(f) { }

    private:
        virtual void fired() { (m_object->*m_function)(this); }

        TimerFiredClass* m_object;
        TimerFiredFunction m_function;
    };

    // Set to true to prevent any timers from firing.
    // When set back to false, timers that were deferred will fire.
    bool isDeferringTimers();
    void setDeferringTimers(bool);

}

#endif
