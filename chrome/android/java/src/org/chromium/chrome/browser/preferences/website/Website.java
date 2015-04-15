// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.website;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * Website is a class for storing information about a website and its associated permissions.
 */
public class Website implements Serializable {

    static final int INVALID_CAMERA_OR_MICROPHONE_ACCESS = 0;
    static final int CAMERA_ACCESS_ALLOWED = 1;
    static final int MICROPHONE_AND_CAMERA_ACCESS_ALLOWED = 2;
    static final int MICROPHONE_ACCESS_ALLOWED = 3;
    static final int CAMERA_ACCESS_DENIED = 4;
    static final int MICROPHONE_AND_CAMERA_ACCESS_DENIED = 5;
    static final int MICROPHONE_ACCESS_DENIED = 6;

    private final WebsiteAddress mAddress;
    private final String mTitle;
    private String mSummary;
    private CookieInfo mCookieInfo;
    private GeolocationInfo mGeolocationInfo;
    private MidiInfo mMidiInfo;
    private ContentSettingException mImagesException;
    private ContentSettingException mJavaScriptException;
    private ContentSettingException mPopupException;
    private ProtectedMediaIdentifierInfo mProtectedMediaIdentifierInfo;
    private PushNotificationInfo mPushNotificationInfo;
    private VoiceAndVideoCaptureInfo mVoiceAndVideoCaptureInfo;
    private LocalStorageInfo mLocalStorageInfo;
    private final List<StorageInfo> mStorageInfo = new ArrayList<StorageInfo>();
    private int mStorageInfoCallbacksLeft;
    private FullscreenInfo mFullscreenInfo;

    public Website(WebsiteAddress address) {
        mAddress = address;
        mTitle = address.getTitle();
    }

    public WebsiteAddress getAddress() {
        return mAddress;
    }

    public String getTitle() {
        return mTitle;
    }

    public String getSummary() {
        return mSummary;
    }

    public int compareByAddressTo(Website to) {
        return this == to ? 0 : mAddress.compareTo(to.mAddress);
    }

    /**
     * A comparison function for sorting by storage (most used first).
     * @return which site uses more storage.
     */
    public int compareByStorageTo(Website to) {
        if (this == to) return 0;
        return getTotalUsage() < to.getTotalUsage() ? 1 : -1;
    }

    /**
     * Sets the CookieInfo object for this site.
     */
    public void setCookieInfo(CookieInfo info) {
        mCookieInfo = info;
        WebsiteAddress embedder = WebsiteAddress.create(info.getEmbedder());
        if (embedder != null) {
            mSummary = embedder.getTitle();
        }
    }

    public CookieInfo getCookieInfo() {
        return mCookieInfo;
    }

    /**
     * Gets the permission that governs cookie preferences.
     */
    public ContentSetting getCookiePermission() {
        return mCookieInfo != null ? mCookieInfo.getContentSetting() : null;
    }

    /**
     * Sets the permission that govers cookie preferences for this site.
     */
    public void setCookiePermission(ContentSetting value) {
        if (mCookieInfo != null) {
            mCookieInfo.setContentSetting(value);
        }
    }

    /**
     * Sets the GeoLocationInfo object for this Website.
     */
    public void setGeolocationInfo(GeolocationInfo info) {
        mGeolocationInfo = info;
        WebsiteAddress embedder = WebsiteAddress.create(info.getEmbedder());
        if (embedder != null) {
            mSummary = embedder.getTitle();
        }
    }

    public GeolocationInfo getGeolocationInfo() {
        return mGeolocationInfo;
    }

    /**
     * Returns what permission governs geolocation access.
     */
    public ContentSetting getGeolocationPermission() {
        return mGeolocationInfo != null ? mGeolocationInfo.getContentSetting() : null;
    }

    /**
     * Configure geolocation access setting for this site.
     */
    public void setGeolocationPermission(ContentSetting value) {
        if (mGeolocationInfo != null) {
            mGeolocationInfo.setContentSetting(value);
        }
    }

    /**
     * Sets the MidiInfo object for this Website.
     */
    public void setMidiInfo(MidiInfo info) {
        mMidiInfo = info;
        WebsiteAddress embedder = WebsiteAddress.create(info.getEmbedder());
        if (embedder != null) {
            mSummary = embedder.getTitle();
        }
    }

    public MidiInfo getMidiInfo() {
        return mMidiInfo;
    }

    /**
     * Returns what permission governs MIDI usage access.
     */
    public ContentSetting getMidiPermission() {
        return mMidiInfo != null ? mMidiInfo.getContentSetting() : null;
    }

    /**
     * Configure Midi usage access setting for this site.
     */
    public void setMidiPermission(ContentSetting value) {
        if (mMidiInfo != null) {
            mMidiInfo.setContentSetting(value);
        }
    }

    /**
     * Returns what permission governs Images access.
     */
    public ContentSetting getImagesPermission() {
        return mImagesException != null ? mImagesException.getContentSetting() : null;
    }

    /**
     * Configure Images permission access setting for this site.
     */
    public void setImagesPermission(ContentSetting value) {
        if (mImagesException != null) {
            mImagesException.setContentSetting(value);
        }
    }

    /**
     * Sets the Images exception info for this Website.
     */
    public void setImagesException(ContentSettingException exception) {
        mImagesException = exception;
    }

    /**
     * Returns what permission governs JavaScript access.
     */
    public ContentSetting getJavaScriptPermission() {
        return mJavaScriptException != null
                ? mJavaScriptException.getContentSetting() : null;
    }

    /**
     * Configure JavaScript permission access setting for this site.
     */
    public void setJavaScriptPermission(ContentSetting value) {
        if (mJavaScriptException != null) {
            mJavaScriptException.setContentSetting(value);
        }
    }

    /**
     * Sets the JavaScript exception info for this Website.
     */
    public void setJavaScriptException(ContentSettingException exception) {
        mJavaScriptException = exception;
    }

    /**
     * Sets the Popup exception info for this Website.
     */
    public void setPopupException(ContentSettingException exception) {
        mPopupException = exception;
    }

    public ContentSettingException getPopupException() {
        return mPopupException;
    }

    /**
     * Returns what permission governs Popup permission.
     */
    public ContentSetting getPopupPermission() {
        if (mPopupException != null) return mPopupException.getContentSetting();
        return null;
    }

    /**
     * Configure Popup permission access setting for this site.
     */
    public void setPopupPermission(ContentSetting value) {
        if (mPopupException != null) {
            mPopupException.setContentSetting(value);
        }
    }

    /**
     * Sets protected media identifier access permission information class.
     */
    public void setProtectedMediaIdentifierInfo(ProtectedMediaIdentifierInfo info) {
        mProtectedMediaIdentifierInfo = info;
        WebsiteAddress embedder = WebsiteAddress.create(info.getEmbedder());
        if (embedder != null) {
            mSummary = embedder.getTitle();
        }
    }

    public ProtectedMediaIdentifierInfo getProtectedMediaIdentifierInfo() {
        return mProtectedMediaIdentifierInfo;
    }

    /**
     * Returns what permission governs Protected Media Identifier access.
     */
    public ContentSetting getProtectedMediaIdentifierPermission() {
        return mProtectedMediaIdentifierInfo != null
                ? mProtectedMediaIdentifierInfo.getContentSetting() : null;
    }

    /**
     * Configure Protected Media Identifier access setting for this site.
     */
    public void setProtectedMediaIdentifierPermission(ContentSetting value) {
        if (mProtectedMediaIdentifierInfo != null) {
            mProtectedMediaIdentifierInfo.setContentSetting(value);
        }
    }

    /**
     * Sets Push Notification access permission information class.
     */
    public void setPushNotificationInfo(PushNotificationInfo info) {
        mPushNotificationInfo = info;
    }

    public PushNotificationInfo getPushNotificationInfo() {
        return mPushNotificationInfo;
    }

    /**
     * Returns what setting governs push notification access.
     */
    public ContentSetting getPushNotificationPermission() {
        return mPushNotificationInfo != null ? mPushNotificationInfo.getContentSetting() : null;
    }

    /**
     * Configure push notification setting for this site.
     */
    public void setPushNotificationPermission(ContentSetting value) {
        if (mPushNotificationInfo != null) {
            mPushNotificationInfo.setContentSetting(value);
        }
    }

    /**
     * Sets voice and video capture info class.
     */
    public void setVoiceAndVideoCaptureInfo(VoiceAndVideoCaptureInfo info) {
        mVoiceAndVideoCaptureInfo = info;
        WebsiteAddress embedder = WebsiteAddress.create(info.getEmbedder());
        if (embedder != null) {
            mSummary = embedder.getTitle();
        }
    }

    public VoiceAndVideoCaptureInfo getVoiceAndVideoCaptureInfo() {
        return mVoiceAndVideoCaptureInfo;
    }

    /**
     * Returns what setting governs voice capture access.
     */
    public ContentSetting getVoiceCapturePermission() {
        return mVoiceAndVideoCaptureInfo != null
                ? mVoiceAndVideoCaptureInfo.getVoiceCapturePermission() : null;
    }

    /**
     * Returns what setting governs video capture access.
     */
    public ContentSetting getVideoCapturePermission() {
        return mVoiceAndVideoCaptureInfo != null
                ? mVoiceAndVideoCaptureInfo.getVideoCapturePermission() : null;
    }

    /**
     * Configure voice capture setting for this site.
     */
    public void setVoiceCapturePermission(ContentSetting value) {
        if (mVoiceAndVideoCaptureInfo != null) {
            mVoiceAndVideoCaptureInfo.setVoiceCapturePermission(value);
        }
    }

    /**
     * Configure video capture setting for this site.
     */
    public void setVideoCapturePermission(ContentSetting value) {
        if (mVoiceAndVideoCaptureInfo != null) {
            mVoiceAndVideoCaptureInfo.setVideoCapturePermission(value);
        }
    }

    /**
     * Returns the type of media that is being captured (audio/video/both).
     */
    public int getMediaAccessType() {
        ContentSetting voice = getVoiceCapturePermission();
        ContentSetting video = getVideoCapturePermission();
        if (video != null) {
            if (voice == null) {
                if (video == ContentSetting.ALLOW) {
                    return CAMERA_ACCESS_ALLOWED;
                } else {
                    return CAMERA_ACCESS_DENIED;
                }
            } else {
                if (video != voice) {
                    return INVALID_CAMERA_OR_MICROPHONE_ACCESS;
                }
                if (video == ContentSetting.ALLOW && voice == ContentSetting.ALLOW) {
                    return MICROPHONE_AND_CAMERA_ACCESS_ALLOWED;
                } else {
                    return MICROPHONE_AND_CAMERA_ACCESS_DENIED;
                }
            }
        } else {
            if (voice == null) return INVALID_CAMERA_OR_MICROPHONE_ACCESS;
            if (voice == ContentSetting.ALLOW) {
                return MICROPHONE_ACCESS_ALLOWED;
            } else {
                return MICROPHONE_ACCESS_DENIED;
            }
        }
    }

    public void setLocalStorageInfo(LocalStorageInfo info) {
        mLocalStorageInfo = info;
    }

    public LocalStorageInfo getLocalStorageInfo() {
        return mLocalStorageInfo;
    }

    public void addStorageInfo(StorageInfo info) {
        mStorageInfo.add(info);
    }

    public List<StorageInfo> getStorageInfo() {
        return new ArrayList<StorageInfo>(mStorageInfo);
    }

    public void clearAllStoredData(final StoredDataClearedCallback callback) {
        if (mLocalStorageInfo != null) {
            mLocalStorageInfo.clear();
            mLocalStorageInfo = null;
        }
        mStorageInfoCallbacksLeft = mStorageInfo.size();
        if (mStorageInfoCallbacksLeft > 0) {
            for (StorageInfo info : mStorageInfo) {
                info.clear(new WebsitePreferenceBridge.StorageInfoClearedCallback() {
                    @Override
                    public void onStorageInfoCleared() {
                        if (--mStorageInfoCallbacksLeft == 0) callback.onStoredDataCleared();
                    }
                });
            }
            mStorageInfo.clear();
        } else {
            callback.onStoredDataCleared();
        }
    }

    /**
     * An interface to implement to get a callback when storage info has been cleared.
     */
    public interface StoredDataClearedCallback {
        public void onStoredDataCleared();
    }

    public long getTotalUsage() {
        long usage = 0;
        if (mLocalStorageInfo != null) {
            usage += mLocalStorageInfo.getSize();
        }
        for (StorageInfo info : mStorageInfo) {
            usage += info.getSize();
        }
        return usage;
    }

    /**
     * Set fullscreen permission information class.
     *
     * @param info Fullscreen information about the website.
     */
    public void setFullscreenInfo(FullscreenInfo info) {
        mFullscreenInfo = info;
        WebsiteAddress embedder = WebsiteAddress.create(info.getEmbedder());
        if (embedder != null) {
            mSummary = embedder.getTitle();
        }
    }

    /**
     * @return fullscreen information of the site.
     */
    public FullscreenInfo getFullscreenInfo() {
        return mFullscreenInfo;
    }

    /**
     * @return what permission governs fullscreen access.
     */
    public ContentSetting getFullscreenPermission() {
        return mFullscreenInfo != null ? mFullscreenInfo.getContentSetting() : null;
    }

    /**
     * Configure fullscreen setting for this site.
     *
     * @param value Content setting for fullscreen permission.
     */
    public void setFullscreenPermission(ContentSetting value) {
        if (mFullscreenInfo != null) {
            mFullscreenInfo.setContentSetting(value);
        }
    }
}
