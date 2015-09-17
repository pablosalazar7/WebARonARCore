// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router;

import org.chromium.chrome.browser.media.router.cast.MediaSink;

import java.util.List;

/**
 * A complimentary interface to {@link MediaRouteProvider}. Media route providers use the
 * implementation to communicate back to the {@link ChromeMediaRouter}.
 */
public interface MediaRouteManager {
    /**
     * Called when the sinks found by the media route provider for
     * the particular |sourceUrn| have changed.
     * @param sourceId The id of the source (presentation URL) that the sinks are received for.
     * @param provider The {@link MediaRouteProvider} that found the sinks.
     * @param sinks The list of {@link MediaSink}s
     */
    void onSinksReceived(String sourceId, MediaRouteProvider provider, List<MediaSink> sinks);

    /**
     * Called when the route was created successfully.
     * @param mediaRouteId the id of the created route.
     * @param provider the provider that created and owns the route.
     * @param requestId the id of the route creation request.
     * @param wasLaunched whether the presentation on the other end of the route was launched or
     *                    just joined.
     */
    public void onRouteCreated(
            String mediaRouteId, int requestId, MediaRouteProvider provider, boolean wasLaunched);

    /**
     * Called when the route was failed to create.
     * @param errorText the error message to return to the page.
     * @param requestId the id of the route creation request.
     */
    public void onRouteCreationError(String errorText, int requestId);

    /**
     * Called when sending the message to the route finished.
     * @param success Indicates if the message was sent successfully.
     * @param callbackId The identifier of the callback to pass the result to.
     */
    public void onMessageSentResult(boolean success, int callbackId);

    /**
     * Called when a specified media route receives a message.
     * @param mediaRouteId The identifier of the media route.
     * @param message The message contents.
     */
    public void onMessage(String mediaRouteId, String message);
}
