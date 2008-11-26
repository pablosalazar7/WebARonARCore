/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AnimationBase_h
#define AnimationBase_h

#include "AtomicString.h"
#include "Timer.h"
#include <wtf/HashMap.h>

namespace WebCore {

class Animation;
class AnimationBase;
class AnimationController;
class CompositeAnimation;
class Element;
class Node;
class RenderObject;
class RenderStyle;
class TimingFunction;

class AnimationTimerBase {
public:
    AnimationTimerBase(AnimationBase* anim)
        : m_timer(this, &AnimationTimerBase::timerFired)
        , m_anim(anim)
    {
        m_timer.startOneShot(0);
    }

    virtual ~AnimationTimerBase() { }

    void startTimer(double timeout = 0)
    {
        m_timer.startOneShot(timeout);
    }

    void cancelTimer()
    {
        m_timer.stop();
    }

    virtual void timerFired(Timer<AnimationTimerBase>*) = 0;

private:
    Timer<AnimationTimerBase> m_timer;

protected:
    AnimationBase* m_anim;
};

class AnimationTimerCallback : public AnimationTimerBase {
public:
    AnimationTimerCallback(AnimationBase* anim) 
        : AnimationTimerBase(anim)
        , m_elapsedTime(0)
    {
    }

    virtual ~AnimationTimerCallback() { }

    virtual void timerFired(Timer<AnimationTimerBase>*);

    void startTimer(double timeout, const AtomicString& eventType, double elapsedTime)
    {
        m_eventType = eventType;
        m_elapsedTime = elapsedTime;
        AnimationTimerBase::startTimer(timeout);
    }

private:
    AtomicString m_eventType;
    double m_elapsedTime;
};

class AnimationBase : public RefCounted<AnimationBase> {
    friend class CompositeAnimationPrivate;

public:
    AnimationBase(const Animation* transition, RenderObject* renderer, CompositeAnimation* compAnim);
    virtual ~AnimationBase();

    RenderObject* renderer() const { return m_object; }
    void clearRenderer() { m_object = 0; }
    
    double startTime() const { return m_startTime; }
    double duration() const;

    void cancelTimers()
    {
        m_animationTimerCallback.cancelTimer();
    }

    // Animations and Transitions go through the states below. When entering the STARTED state
    // the animation is started. This may or may not require deferred response from the animator.
    // If so, we stay in this state until that response is received (and it returns the start time).
    // Otherwise, we use the current time as the start time and go immediately to AnimationStateLooping
    // or AnimationStateEnding.
    enum AnimState { 
        AnimationStateNew,                  // animation just created, animation not running yet
        AnimationStateStartWaitTimer,       // start timer running, waiting for fire
        AnimationStateStartWaitStyleAvailable,   // waiting for style setup so we can start animations
        AnimationStateStartWaitResponse,    // animation started, waiting for response
        AnimationStateLooping,              // response received, animation running, loop timer running, waiting for fire
        AnimationStateEnding,               // received, animation running, end timer running, waiting for fire
        AnimationStatePausedWaitTimer,      // in pause mode when animation started
        AnimationStatePausedWaitResponse,   // animation paused when in STARTING state
        AnimationStatePausedRun,            // animation paused when in LOOPING or ENDING state
        AnimationStateDone                  // end timer fired, animation finished and removed
    };

    enum AnimStateInput {
        AnimationStateInputMakeNew,           // reset back to new from any state
        AnimationStateInputStartAnimation,    // animation requests a start
        AnimationStateInputRestartAnimation,  // force a restart from any state
        AnimationStateInputStartTimerFired,   // start timer fired
        AnimationStateInputStyleAvailable,    // style is setup, ready to start animating
        AnimationStateInputStartTimeSet,      // m_startTime was set
        AnimationStateInputLoopTimerFired,    // loop timer fired
        AnimationStateInputEndTimerFired,     // end timer fired
        AnimationStateInputPauseOverride,     // pause an animation due to override
        AnimationStateInputResumeOverride,    // resume an overridden animation
        AnimationStateInputPlayStateRunnning, // play state paused -> running
        AnimationStateInputPlayStatePaused,   // play state running -> paused
        AnimationStateInputEndAnimation       // force an end from any state
    };

    // Called when animation is in AnimationStateNew to start animation
    void updateStateMachine(AnimStateInput, double param);

    // Animation has actually started, at passed time
    void onAnimationStartResponse(double startTime);

    // Called to change to or from paused state
    void updatePlayState(bool running);
    bool playStatePlaying() const;

    bool waitingToStart() const { return m_animState == AnimationStateNew || m_animState == AnimationStateStartWaitTimer; }
    bool preActive() const
    {
        return m_animState == AnimationStateNew || m_animState == AnimationStateStartWaitTimer || m_animState == AnimationStateStartWaitStyleAvailable || m_animState == AnimationStateStartWaitResponse;
    }

    bool postActive() const { return m_animState == AnimationStateDone; }
    bool active() const { return !postActive() && !preActive(); }
    bool running() const { return !isNew() && !postActive(); }
    bool paused() const { return m_pauseTime >= 0; }
    bool isNew() const { return m_animState == AnimationStateNew; }
    bool waitingForStartTime() const { return m_animState == AnimationStateStartWaitResponse; }
    bool waitingForStyleAvailable() const { return m_animState == AnimationStateStartWaitStyleAvailable; }

    // "animating" means that something is running that requires a timer to keep firing
    // (e.g. a software animation)
    void setAnimating(bool inAnimating = true) { m_isAnimating = inAnimating; }
    bool isAnimating() const { return m_isAnimating; }

    double progress(double scale, double offset, const TimingFunction*) const;

    virtual void animate(CompositeAnimation*, RenderObject*, const RenderStyle* currentStyle, 
                         const RenderStyle* targetStyle, RefPtr<RenderStyle>& animatedStyle) { }

    virtual bool shouldFireEvents() const { return false; }

    void animationTimerCallbackFired(const AtomicString& eventType, double elapsedTime);

    bool animationsMatch(const Animation*) const;

    void setAnimation(const Animation* anim) { m_animation = const_cast<Animation*>(anim); }

    // Return true if this animation is overridden. This will only be the case for
    // ImplicitAnimations and is used to determine whether or not we should force
    // set the start time. If an animation is overridden, it will probably not get
    // back the AnimationStateInputStartTimeSet input.
    virtual bool overridden() const { return false; }

    // Does this animation/transition involve the given property?
    virtual bool affectsProperty(int property) const { return false; }
    bool isAnimatingProperty(int property, bool isRunningNow) const
    {
        if (isRunningNow)
            return (!waitingToStart() && !postActive()) && affectsProperty(property);

        return !postActive() && affectsProperty(property);
    }

    bool isTransformFunctionListValid() const { return m_transformFunctionListValid; }
    
    void pauseAtTime(double t);
    
protected:
    virtual void overrideAnimations() { }
    virtual void resumeOverriddenAnimations() { }

    CompositeAnimation* compositeAnimation() { return m_compAnim; }

    // These are called when the corresponding timer fires so subclasses can do any extra work
    virtual void onAnimationStart(double elapsedTime) { }
    virtual void onAnimationIteration(double elapsedTime) { }
    virtual void onAnimationEnd(double elapsedTime) { }
    virtual bool startAnimation(double beginTime) { return false; }
    virtual void endAnimation(bool reset) { }

    void primeEventTimers();

    static bool propertiesEqual(int prop, const RenderStyle* a, const RenderStyle* b);
    static int getPropertyAtIndex(int);
    static int getNumProperties();

    // Return true if we need to start software animation timers
    static bool blendProperties(const AnimationBase* anim, int prop, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress);
    
    static void setChanged(Node*);

protected:
    AnimState m_animState;

    bool m_isAnimating;       // transition/animation requires continual timer firing
    bool m_waitedForResponse;
    double m_startTime;
    double m_pauseTime;
    RenderObject* m_object;

    AnimationTimerCallback m_animationTimerCallback;
    RefPtr<Animation> m_animation;
    CompositeAnimation* m_compAnim;
    bool m_transformFunctionListValid;
};

} // namespace WebCore

#endif // AnimationBase_h
