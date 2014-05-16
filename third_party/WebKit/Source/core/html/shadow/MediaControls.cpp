/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
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

#include "config.h"
#include "core/html/shadow/MediaControls.h"

#include "bindings/v8/ExceptionStatePlaceholder.h"
#include "core/events/MouseEvent.h"
#include "core/frame/Settings.h"
#include "core/html/HTMLMediaElement.h"
#include "core/html/MediaController.h"
#include "core/rendering/RenderTheme.h"

namespace WebCore {

static const double timeWithoutMouseMovementBeforeHidingMediaControls = 3;

MediaControls::MediaControls(HTMLMediaElement& mediaElement)
    : HTMLDivElement(mediaElement.document())
    , m_mediaElement(mediaElement)
    , m_panel(0)
    , m_textDisplayContainer(0)
    , m_overlayPlayButton(0)
    , m_overlayEnclosure(0)
    , m_playButton(0)
    , m_currentTimeDisplay(0)
    , m_timeline(0)
    , m_muteButton(0)
    , m_volumeSlider(0)
    , m_toggleClosedCaptionsButton(0)
    , m_fullScreenButton(0)
    , m_durationDisplay(0)
    , m_enclosure(0)
    , m_hideMediaControlsTimer(this, &MediaControls::hideMediaControlsTimerFired)
    , m_isMouseOverControls(false)
    , m_isPausedForScrubbing(false)
{
}

PassRefPtr<MediaControls> MediaControls::create(HTMLMediaElement& mediaElement)
{
    RefPtr<MediaControls> controls = adoptRef(new MediaControls(mediaElement));

    if (controls->initializeControls())
        return controls.release();

    return nullptr;
}

bool MediaControls::initializeControls()
{
    TrackExceptionState exceptionState;

    if (document().settings() && document().settings()->mediaControlsOverlayPlayButtonEnabled()) {
        RefPtr<MediaControlOverlayEnclosureElement> overlayEnclosure = MediaControlOverlayEnclosureElement::create(*this);
        RefPtr<MediaControlOverlayPlayButtonElement> overlayPlayButton = MediaControlOverlayPlayButtonElement::create(*this);
        m_overlayPlayButton = overlayPlayButton.get();
        overlayEnclosure->appendChild(overlayPlayButton.release(), exceptionState);
        if (exceptionState.hadException())
            return false;

        m_overlayEnclosure = overlayEnclosure.get();
        appendChild(overlayEnclosure.release(), exceptionState);
        if (exceptionState.hadException())
            return false;
    }

    // Create an enclosing element for the panel so we can visually offset the controls correctly.
    RefPtr<MediaControlPanelEnclosureElement> enclosure = MediaControlPanelEnclosureElement::create(*this);

    RefPtr<MediaControlPanelElement> panel = MediaControlPanelElement::create(*this);

    RefPtr<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(*this);
    m_playButton = playButton.get();
    panel->appendChild(playButton.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    RefPtr<MediaControlTimelineElement> timeline = MediaControlTimelineElement::create(*this);
    m_timeline = timeline.get();
    panel->appendChild(timeline.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    RefPtr<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(*this);
    m_currentTimeDisplay = currentTimeDisplay.get();
    m_currentTimeDisplay->hide();
    panel->appendChild(currentTimeDisplay.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    RefPtr<MediaControlTimeRemainingDisplayElement> durationDisplay = MediaControlTimeRemainingDisplayElement::create(*this);
    m_durationDisplay = durationDisplay.get();
    panel->appendChild(durationDisplay.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    RefPtr<MediaControlMuteButtonElement> muteButton = MediaControlMuteButtonElement::create(*this);
    m_muteButton = muteButton.get();
    panel->appendChild(muteButton.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    RefPtr<MediaControlVolumeSliderElement> slider = MediaControlVolumeSliderElement::create(*this);
    m_volumeSlider = slider.get();
    panel->appendChild(slider.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    RefPtr<MediaControlToggleClosedCaptionsButtonElement> toggleClosedCaptionsButton = MediaControlToggleClosedCaptionsButtonElement::create(*this);
    m_toggleClosedCaptionsButton = toggleClosedCaptionsButton.get();
    panel->appendChild(toggleClosedCaptionsButton.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    RefPtr<MediaControlFullscreenButtonElement> fullscreenButton = MediaControlFullscreenButtonElement::create(*this);
    m_fullScreenButton = fullscreenButton.get();
    panel->appendChild(fullscreenButton.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    m_panel = panel.get();
    enclosure->appendChild(panel.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    m_enclosure = enclosure.get();
    appendChild(enclosure.release(), exceptionState);
    if (exceptionState.hadException())
        return false;

    return true;
}

void MediaControls::reset()
{
    double duration = mediaElement().duration();
    m_durationDisplay->setInnerText(RenderTheme::theme().formatMediaControlsTime(duration), ASSERT_NO_EXCEPTION);
    m_durationDisplay->setCurrentValue(duration);

    updatePlayState();

    updateCurrentTimeDisplay();

    m_timeline->setDuration(duration);
    m_timeline->setPosition(mediaElement().currentTime());

    if (!mediaElement().hasAudio())
        m_volumeSlider->hide();
    else
        m_volumeSlider->show();
    updateVolume();

    refreshClosedCaptionsButtonVisibility();

    if (mediaElement().hasVideo())
        m_fullScreenButton->show();
    else
        m_fullScreenButton->hide();

    makeOpaque();
}

void MediaControls::show()
{
    makeOpaque();
    m_panel->setIsDisplayed(true);
    m_panel->show();
}

void MediaControls::hide()
{
    m_panel->setIsDisplayed(false);
    m_panel->hide();
}

void MediaControls::makeOpaque()
{
    m_panel->makeOpaque();
}

void MediaControls::makeTransparent()
{
    m_panel->makeTransparent();
}

bool MediaControls::shouldHideMediaControls()
{
    return !m_panel->hovered();
}

void MediaControls::playbackStarted()
{
    m_currentTimeDisplay->show();
    m_durationDisplay->hide();

    updatePlayState();
    m_timeline->setPosition(mediaElement().currentTime());
    updateCurrentTimeDisplay();

    startHideMediaControlsTimer();
}

void MediaControls::playbackProgressed()
{
    m_timeline->setPosition(mediaElement().currentTime());
    updateCurrentTimeDisplay();

    if (!m_isMouseOverControls && mediaElement().hasVideo())
        makeTransparent();
}

void MediaControls::playbackStopped()
{
    updatePlayState();
    m_timeline->setPosition(mediaElement().currentTime());
    updateCurrentTimeDisplay();
    makeOpaque();

    stopHideMediaControlsTimer();
}

void MediaControls::updatePlayState()
{
    if (m_isPausedForScrubbing)
        return;

    if (m_overlayPlayButton)
        m_overlayPlayButton->updateDisplayType();
    m_playButton->updateDisplayType();
}

void MediaControls::beginScrubbing()
{
    if (!mediaElement().togglePlayStateWillPlay()) {
        m_isPausedForScrubbing = true;
        mediaElement().togglePlayState();
    }
}

void MediaControls::endScrubbing()
{
    if (m_isPausedForScrubbing) {
        m_isPausedForScrubbing = false;
        if (mediaElement().togglePlayStateWillPlay())
            mediaElement().togglePlayState();
    }
}

void MediaControls::updateCurrentTimeDisplay()
{
    double now = mediaElement().currentTime();
    double duration = mediaElement().duration();

    // After seek, hide duration display and show current time.
    if (now > 0) {
        m_currentTimeDisplay->show();
        m_durationDisplay->hide();
    }

    // Allow the theme to format the time.
    m_currentTimeDisplay->setInnerText(RenderTheme::theme().formatMediaControlsCurrentTime(now, duration), IGNORE_EXCEPTION);
    m_currentTimeDisplay->setCurrentValue(now);
}

void MediaControls::updateVolume()
{
    m_muteButton->updateDisplayType();
    if (m_muteButton->renderer())
        m_muteButton->renderer()->repaint();

    if (mediaElement().muted())
        m_volumeSlider->setVolume(0);
    else
        m_volumeSlider->setVolume(mediaElement().volume());
}

void MediaControls::changedClosedCaptionsVisibility()
{
    m_toggleClosedCaptionsButton->updateDisplayType();
}

void MediaControls::refreshClosedCaptionsButtonVisibility()
{
    if (mediaElement().hasClosedCaptions())
        m_toggleClosedCaptionsButton->show();
    else
        m_toggleClosedCaptionsButton->hide();
}

void MediaControls::closedCaptionTracksChanged()
{
    refreshClosedCaptionsButtonVisibility();
}

void MediaControls::enteredFullscreen()
{
    m_fullScreenButton->setIsFullscreen(true);
    stopHideMediaControlsTimer();
    startHideMediaControlsTimer();
}

void MediaControls::exitedFullscreen()
{
    m_fullScreenButton->setIsFullscreen(false);
    stopHideMediaControlsTimer();
    startHideMediaControlsTimer();
}

void MediaControls::defaultEventHandler(Event* event)
{
    HTMLDivElement::defaultEventHandler(event);

    if (event->type() == EventTypeNames::mouseover) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = true;
            if (!mediaElement().togglePlayStateWillPlay()) {
                makeOpaque();
                if (shouldHideMediaControls())
                    startHideMediaControlsTimer();
            }
        }
        return;
    }

    if (event->type() == EventTypeNames::mouseout) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = false;
            stopHideMediaControlsTimer();
        }
        return;
    }

    if (event->type() == EventTypeNames::mousemove) {
        // When we get a mouse move, show the media controls, and start a timer
        // that will hide the media controls after a 3 seconds without a mouse move.
        makeOpaque();
        if (shouldHideMediaControls())
            startHideMediaControlsTimer();
        return;
    }
}

void MediaControls::hideMediaControlsTimerFired(Timer<MediaControls>*)
{
    if (mediaElement().togglePlayStateWillPlay())
        return;

    if (!shouldHideMediaControls())
        return;

    makeTransparent();
}

void MediaControls::startHideMediaControlsTimer()
{
    m_hideMediaControlsTimer.startOneShot(timeWithoutMouseMovementBeforeHidingMediaControls, FROM_HERE);
}

void MediaControls::stopHideMediaControlsTimer()
{
    m_hideMediaControlsTimer.stop();
}

const AtomicString& MediaControls::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls"));
    return id;
}

bool MediaControls::containsRelatedTarget(Event* event)
{
    if (!event->isMouseEvent())
        return false;
    EventTarget* relatedTarget = toMouseEvent(event)->relatedTarget();
    if (!relatedTarget)
        return false;
    return contains(relatedTarget->toNode());
}

void MediaControls::createTextTrackDisplay()
{
    if (m_textDisplayContainer)
        return;

    RefPtr<MediaControlTextTrackContainerElement> textDisplayContainer = MediaControlTextTrackContainerElement::create(*this);
    m_textDisplayContainer = textDisplayContainer.get();

    // Insert it before (behind) all other control elements.
    if (m_overlayEnclosure && m_overlayPlayButton)
        m_overlayEnclosure->insertBefore(textDisplayContainer.release(), m_overlayPlayButton);
    else
        insertBefore(textDisplayContainer.release(), m_enclosure);
}

void MediaControls::showTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->show();
}

void MediaControls::hideTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->hide();
}

void MediaControls::updateTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();

    m_textDisplayContainer->updateDisplay();
}

}
