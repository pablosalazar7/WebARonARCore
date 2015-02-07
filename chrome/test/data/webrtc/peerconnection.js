/**
 * Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * We need a STUN server for some API calls.
 * @private
 */
var STUN_SERVER = 'stun.l.google.com:19302';

/**
 * The one and only peer connection in this page.
 * @private
 */
var gPeerConnection = null;

/**
 * This stores ICE candidates generated on this side.
 * @private
 */
var gIceCandidates = [];

/**
 * Keeps track of whether we have seen crypto information in the SDP.
 * @private
 */
var gHasSeenCryptoInSdp = 'no-crypto-seen';

// Public interface to tests. These are expected to be called with
// ExecuteJavascript invocations from the browser tests and will return answers
// through the DOM automation controller.

/**
 * Creates a peer connection. Must be called before most other public functions
 * in this file.
 */
function preparePeerConnection() {
  if (gPeerConnection != null)
    throw failTest('creating peer connection, but we already have one.');

  gPeerConnection = createPeerConnection_(STUN_SERVER);
  returnToTest('ok-peerconnection-created');
}

/**
 * Asks this page to create a local offer.
 *
 * Returns a string on the format ok-(JSON encoded session description).
 *
 * @param {!Object} constraints Any createOffer constraints.
 */
function createLocalOffer(constraints) {
  peerConnection_().createOffer(
      function(localOffer) {
        success_('createOffer');
        setLocalDescription(peerConnection, localOffer);

        returnToTest('ok-' + stringifyDOMObject_(localOffer));
      },
      function(error) { failure_('createOffer', error); },
      constraints);
}

/**
 * Asks this page to accept an offer and generate an answer.
 *
 * Returns a string on the format ok-(JSON encoded session description).
 *
 * @param {!string} sessionDescJson A JSON-encoded session description of type
 *     'offer'.
 * @param {!Object} constraints Any createAnswer constraints.
 */
function receiveOfferFromPeer(sessionDescJson, constraints) {
  offer = parseJson_(sessionDescJson);
  if (!offer.type)
    failTest('Got invalid session description from peer: ' + sessionDescJson);
  if (offer.type != 'offer')
    failTest('Expected to receive offer from peer, got ' + offer.type);

  var sessionDescription = new RTCSessionDescription(offer);
  peerConnection_().setRemoteDescription(
      sessionDescription,
      function() { success_('setRemoteDescription'); },
      function(error) { failure_('setRemoteDescription', error); });

  peerConnection_().createAnswer(
      function(answer) {
        success_('createAnswer');
        setLocalDescription(peerConnection, answer);
        returnToTest('ok-' + stringifyDOMObject_(answer));
      },
      function(error) { failure_('createAnswer', error); },
      constraints);
}

/**
 * Asks this page to accept an answer generated by the peer in response to a
 * previous offer by this page
 *
 * Returns a string ok-accepted-answer on success.
 *
 * @param {!string} sessionDescJson A JSON-encoded session description of type
 *     'answer'.
 */
function receiveAnswerFromPeer(sessionDescJson) {
  answer = parseJson_(sessionDescJson);
  if (!answer.type)
    failTest('Got invalid session description from peer: ' + sessionDescJson);
  if (answer.type != 'answer')
    failTest('Expected to receive answer from peer, got ' + answer.type);

  var sessionDescription = new RTCSessionDescription(answer);
  peerConnection_().setRemoteDescription(
      sessionDescription,
      function() {
        success_('setRemoteDescription');
        returnToTest('ok-accepted-answer');
      },
      function(error) { failure_('setRemoteDescription', error); });
}

/**
 * Adds the local stream to the peer connection. You will have to re-negotiate
 * the call for this to take effect in the call.
 */
function addLocalStream() {
  addLocalStreamToPeerConnection(peerConnection_());
  returnToTest('ok-added');
}

/**
 * Loads a file with WebAudio and connects it to the peer connection.
 *
 * The loadAudioAndAddToPeerConnection will return ok-added to the test when
 * the sound is loaded and added to the peer connection. The sound will start
 * playing when you call playAudioFile.
 *
 * @param url URL pointing to the file to play. You can assume that you can
 *     serve files from the repository's file system. For instance, to serve a
 *     file from chrome/test/data/pyauto_private/webrtc/file.wav, pass in a path
 *     relative to this directory (e.g. ../pyauto_private/webrtc/file.wav).
 */
function addAudioFile(url) {
  loadAudioAndAddToPeerConnection(url, peerConnection_());
}

/**
 * Must be called after addAudioFile.
 */
function playAudioFile() {
  playPreviouslyLoadedAudioFile(peerConnection_());
  returnToTest('ok-playing');
}

/**
 * Hangs up a started call. Returns ok-call-hung-up on success.
 */
function hangUp() {
  peerConnection_().close();
  gPeerConnection = null;
  returnToTest('ok-call-hung-up');
}

/**
 * Retrieves all ICE candidates generated on this side. Must be called after
 * ICE candidate generation is triggered (for instance by running a call
 * negotiation). This function will wait if necessary if we're not done
 * generating ICE candidates on this side.
 *
 * Returns a JSON-encoded array of RTCIceCandidate instances to the test.
 */
function getAllIceCandidates() {
  if (peerConnection_().iceGatheringState != 'complete') {
    console.log('Still ICE gathering - waiting...');
    setTimeout(getAllIceCandidates, 100);
    return;
  }

  returnToTest(stringifyDOMObject_(gIceCandidates));
}

/**
 * Receives ICE candidates from the peer.
 *
 * Returns ok-received-candidates to the test on success.
 *
 * @param iceCandidatesJson a JSON-encoded array of RTCIceCandidate instances.
 */
function receiveIceCandidates(iceCandidatesJson) {
  var iceCandidates = parseJson_(iceCandidatesJson);
  if (!iceCandidates.length)
    throw failTest('Received invalid ICE candidate list from peer: ' +
        iceCandidatesJson);

  iceCandidates.forEach(function(iceCandidate) {
    if (!iceCandidate.candidate)
      failTest('Received invalid ICE candidate from peer: ' +
          iceCandidatesJson);

    peerConnection_().addIceCandidate(new RTCIceCandidate(iceCandidate,
        function() { success_('addIceCandidate'); },
        function(error) { failure_('addIceCandidate', error); }
    ));
  });

  returnToTest('ok-received-candidates');
}

/**
 * Returns
 */
function hasSeenCryptoInSdp() {
  returnToTest(gHasSeenCryptoInSdp);
}

// Internals.

/** @private */
function createPeerConnection_(stun_server) {
  servers = {iceServers: [{url: 'stun:' + stun_server}]};
  try {
    peerConnection = new RTCPeerConnection(servers, {});
  } catch (exception) {
    throw failTest('Failed to create peer connection: ' + exception);
  }
  peerConnection.onaddstream = addStreamCallback_;
  peerConnection.onremovestream = removeStreamCallback_;
  peerConnection.onicecandidate = iceCallback_;
  return peerConnection;
}

/** @private */
function peerConnection_() {
  if (gPeerConnection == null)
    throw failTest('Trying to use peer connection, but none was created.');
  return gPeerConnection;
}

/** @private */
function success_(method) {
  debug(method + '(): success.');
}

/** @private */
function failure_(method, error) {
  throw failTest(method + '() failed: ' + stringifyDOMObject_(error));
}

/** @private */
function iceCallback_(event) {
  if (event.candidate)
    gIceCandidates.push(event.candidate);
}

/** @private */
function setLocalDescription(peerConnection, sessionDescription) {
  if (sessionDescription.sdp.search('a=crypto') != -1 ||
      sessionDescription.sdp.search('a=fingerprint') != -1)
    gHasSeenCryptoInSdp = 'crypto-seen';

  peerConnection.setLocalDescription(
    sessionDescription,
    function() { success_('setLocalDescription'); },
    function(error) { failure_('setLocalDescription', error); });
}

/** @private */
function addStreamCallback_(event) {
  debug('Receiving remote stream...');
  var videoTag = document.getElementById('remote-view');
  attachMediaStream(videoTag, event.stream);
}

/** @private */
function removeStreamCallback_(event) {
  debug('Call ended.');
  document.getElementById('remote-view').src = '';
}

/**
 * Stringifies a DOM object.
 *
 * This function stringifies not only own properties but also DOM attributes
 * which are on a prototype chain.  Note that JSON.stringify only stringifies
 * own properties.
 * @private
 */
function stringifyDOMObject_(object)
{
  function deepCopy(src) {
    if (typeof src != "object")
      return src;
    var dst = Array.isArray(src) ? [] : {};
    for (var property in src) {
      dst[property] = deepCopy(src[property]);
    }
    return dst;
  }
  return JSON.stringify(deepCopy(object));
}

/**
 * Parses JSON-encoded session descriptions and ICE candidates.
 * @private
 */
function parseJson_(json) {
  // Escape since the \r\n in the SDP tend to get unescaped.
  jsonWithEscapedLineBreaks = json.replace(/\r\n/g, '\\r\\n');
  try {
    return JSON.parse(jsonWithEscapedLineBreaks);
  } catch (exception) {
    failTest('Failed to parse JSON: ' + jsonWithEscapedLineBreaks + ', got ' +
             exception);
  }
}
