# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'webkit_media',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/media/media.gyp:yuv_convert',
        '<(DEPTH)/skia/skia.gyp:skia',
      ],
      'sources': [
        'android/audio_decoder_android.cc',
        'android/media_metadata_android.cc',
        'android/media_metadata_android.h',
        'android/stream_texture_factory_android.h',
        'android/webmediaplayer_android.cc',
        'android/webmediaplayer_android.h',
        'android/webmediaplayer_manager_android.cc',
        'android/webmediaplayer_manager_android.h',
        'android/webmediaplayer_proxy_android.cc',
        'android/webmediaplayer_proxy_android.h',
        'active_loader.cc',
        'active_loader.h',
        'audio_decoder.cc',
        'audio_decoder.h',
        'buffered_data_source.cc',
        'buffered_data_source.h',
        'buffered_resource_loader.cc',
        'buffered_resource_loader.h',
        'cache_util.cc',
        'cache_util.h',
        'crypto/key_systems.cc',
        'crypto/key_systems.h',
        'crypto/ppapi_decryptor.cc',
        'crypto/ppapi_decryptor.h',
        'crypto/proxy_decryptor.cc',
        'crypto/proxy_decryptor.h',
        'filter_helpers.cc',
        'filter_helpers.h',
        'media_stream_client.h',
        'preload.h',
        'skcanvas_video_renderer.cc',
        'skcanvas_video_renderer.h',
        'webmediaplayer_delegate.h',
        'webmediaplayer_impl.cc',
        'webmediaplayer_impl.h',
        'webmediaplayer_proxy.cc',
        'webmediaplayer_proxy.h',
        'webmediaplayer_util.cc',
        'webmediaplayer_util.h',
        'webvideoframe_impl.cc',
        'webvideoframe_impl.h',
      ],
      'conditions': [
        ['inside_chromium_build==0', {
          'dependencies': [
            '<(DEPTH)/webkit/support/setup_third_party.gyp:third_party_headers',
          ],
        }],
        ['OS == "android"', {
          'sources!': [
            'audio_decoder.cc',
            'audio_decoder.h',
            'filter_helpers.cc',
            'filter_helpers.h',
            'webmediaplayer_impl.cc',
            'webmediaplayer_impl.h',
            'webmediaplayer_proxy.cc',
            'webmediaplayer_proxy.h',
          ],
          'dependencies': [
            '<(DEPTH)/media/media.gyp:player_android',
          ],
        }, { # OS != "android"'
          'sources/': [
            ['exclude', '^android/'],
          ],
        }],
      ],
    },
    {
      'target_name': 'ppapi_cdm_wrapper',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/ppapi/ppapi.gyp:ppapi_cpp'
      ],
      'conditions': [
        ['os_posix==1 and OS!="mac"', {
          'cflags': ['-fvisibility=hidden'],
          'type': 'shared_library',
          # -gstabs, used in the official builds, causes an ICE. Simply remove
          # it.
          'cflags!': ['-gstabs'],
        }],
        ['OS=="win"', {
          'type': 'shared_library',
        }],
        ['OS=="mac"', {
          'type': 'loadable_module',
          'mac_bundle': 1,
          'product_extension': 'plugin',
          'xcode_settings': {
            'OTHER_LDFLAGS': [
              # Not to strip important symbols by -Wl,-dead_strip.
              '-Wl,-exported_symbol,_PPP_GetInterface',
              '-Wl,-exported_symbol,_PPP_InitializeModule',
              '-Wl,-exported_symbol,_PPP_ShutdownModule'
            ]},
        }],
      ],
      'sources': [
        'crypto/ppapi/cdm_wrapper.cc',
      ],
    }
  ],
}
