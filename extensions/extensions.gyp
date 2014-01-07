# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'extensions_common',
      'type': 'static_library',
      'dependencies': [
        # TODO(benwells): figure out what to do with the api target and
        # api resources compiled into the chrome resource bundle.
        # http://crbug.com/162530
        '../chrome/chrome_resources.gyp:chrome_resources',
        # TODO(jamescook|derat): Pull strings into extensions module.
        '../chrome/chrome_resources.gyp:chrome_strings',
        '../chrome/common/extensions/api/api.gyp:api',
        '../components/components.gyp:url_matcher',
        '../content/content.gyp:content_common',
        '../third_party/re2/re2.gyp:re2',
      ],
      'include_dirs': [
        '..',
        '<(INTERMEDIATE_DIR)',
      ],
      'sources': [
        'common/common_manifest_handlers.cc',
        'common/common_manifest_handlers.h',
        'common/crx_file.cc',
        'common/crx_file.h',
        'common/csp_validator.cc',
        'common/csp_validator.h',
        'common/draggable_region.cc',
        'common/draggable_region.h',
        'common/error_utils.cc',
        'common/error_utils.h',
        'common/event_filter.cc',
        'common/event_filter.h',
        'common/event_filtering_info.cc',
        'common/event_filtering_info.h',
        'common/event_matcher.cc',
        'common/event_matcher.h',
        'common/extension.cc',
        'common/extension.h',
        'common/extension_api.cc',
        'common/extension_api.h',
        'common/extension_api_stub.cc',
        'common/extension_paths.cc',
        'common/extension_paths.h',
        'common/extension_resource.cc',
        'common/extension_resource.h',
        'common/extension_set.cc',
        'common/extension_set.h',
        'common/extension_urls.cc',
        'common/extension_urls.h',
        'common/extensions_client.cc',
        'common/extensions_client.h',
        'common/feature_switch.cc',
        'common/feature_switch.h',
        'common/features/feature.cc',
        'common/features/feature.h',
        'common/features/feature_provider.cc',
        'common/features/feature_provider.h',
        'common/file_util.cc',
        'common/file_util.h',
        'common/id_util.cc',
        'common/id_util.h',
        'common/install_warning.cc',
        'common/install_warning.h',
        'common/manifest.cc',
        'common/manifest.h',
        'common/manifest_constants.cc',
        'common/manifest_constants.h',
        'common/manifest_handler.cc',
        'common/manifest_handler.h',
        'common/manifest_handlers/background_info.cc',
        'common/manifest_handlers/background_info.h',
        'common/manifest_handlers/csp_info.cc',
        'common/manifest_handlers/csp_info.h',
        'common/manifest_handlers/incognito_info.cc',
        'common/manifest_handlers/incognito_info.h',
        'common/manifest_handlers/kiosk_mode_info.cc',
        'common/manifest_handlers/kiosk_mode_info.h',
        'common/manifest_handlers/offline_enabled_info.cc',
        'common/manifest_handlers/offline_enabled_info.h',
        'common/manifest_handlers/requirements_info.h',
        'common/manifest_handlers/requirements_info.cc',
        'common/manifest_handlers/sandboxed_page_info.cc',
        'common/manifest_handlers/sandboxed_page_info.h',
        'common/manifest_handlers/shared_module_info.cc',
        'common/manifest_handlers/shared_module_info.h',
        'common/one_shot_event.cc',
        'common/one_shot_event.h',
        'common/permissions/api_permission.cc',
        'common/permissions/api_permission.h',
        'common/permissions/api_permission_set.cc',
        'common/permissions/api_permission_set.h',
        'common/permissions/base_set_operators.h',
        'common/permissions/manifest_permission.cc',
        'common/permissions/manifest_permission.h',
        'common/permissions/manifest_permission_set.cc',
        'common/permissions/manifest_permission_set.h',
        'common/permissions/permission_message.cc',
        'common/permissions/permission_message.h',
        'common/permissions/permission_message_provider.cc',
        'common/permissions/permission_message_provider.h',
        'common/permissions/permission_set.cc',
        'common/permissions/permission_set.h',
        'common/permissions/permissions_data.cc',
        'common/permissions/permissions_data.h',
        'common/permissions/permissions_info.cc',
        'common/permissions/permissions_info.h',
        'common/permissions/permissions_provider.h',
        'common/stack_frame.cc',
        'common/stack_frame.h',
        'common/switches.cc',
        'common/switches.h',
        'common/url_pattern.cc',
        'common/url_pattern.h',
        'common/url_pattern_set.cc',
        'common/url_pattern_set.h',
        'common/user_script.cc',
        'common/user_script.h',
        'common/view_type.cc',
        'common/view_type.h',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
      'conditions': [
        ['enable_extensions==1', {
          'sources!': [
            'common/extension_api_stub.cc',
          ],
        }, {  # enable_extensions == 0
          'sources!': [
            'common/extension_api.cc',
          ],
        }],
      ],
    },
    {
      'target_name': 'extensions_browser',
      'type': 'static_library',
      'dependencies': [
        'extensions_common',
        # TODO(jamescook|derat): Pull strings into extensions module.
        '../chrome/chrome_resources.gyp:chrome_strings',
        '../chrome/common/extensions/api/api.gyp:api',
        '../content/content.gyp:content_browser',
        '../skia/skia.gyp:skia',
      ],
      'include_dirs': [
        '..',
        '<(INTERMEDIATE_DIR)',
        # Needed to access generated API headers.
        '<(SHARED_INTERMEDIATE_DIR)',
        # Needed for grit.
        '<(SHARED_INTERMEDIATE_DIR)/chrome',
      ],
      'sources': [
        'browser/admin_policy.cc',
        'browser/admin_policy.h',
        'browser/app_sorting.h',
        'browser/event_listener_map.cc',
        'browser/event_listener_map.h',
        'browser/event_router.cc',
        'browser/event_router.h',
        'browser/extension_prefs_scope.h',
        'browser/extension_error.cc',
        'browser/extension_error.h',
        'browser/extension_function.cc',
        'browser/extension_function.h',
        'browser/extension_registry.cc',
        'browser/extension_registry.h',
        'browser/extension_registry_factory.cc',
        'browser/extension_registry_factory.h',
        'browser/extensions_browser_client.cc',
        'browser/extensions_browser_client.h',
        'browser/external_provider_interface.h',
        'browser/info_map.cc',
        'browser/info_map.h',
        'browser/file_highlighter.cc',
        'browser/file_highlighter.h',
        'browser/file_reader.cc',
        'browser/file_reader.h',
        'browser/lazy_background_task_queue.cc',
        'browser/lazy_background_task_queue.h',
        'browser/management_policy.cc',
        'browser/management_policy.h',
        'browser/pending_extension_info.cc',
        'browser/pending_extension_info.h',
        'browser/pending_extension_manager.cc',
        'browser/pending_extension_manager.h',
        'browser/pref_names.cc',
        'browser/pref_names.h',
        'browser/process_manager.cc',
        'browser/process_manager.h',
        'browser/process_map.cc',
        'browser/process_map.h',
        'browser/quota_service.cc',
        'browser/quota_service.h',
        'browser/update_observer.h',
        'browser/view_type_utils.cc',
        'browser/view_type_utils.h',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
    {
      'target_name': 'extensions_test_support',
      'type': 'static_library',
      'dependencies': [
        'extensions_browser',
        'extensions_common',
        '../base/base.gyp:base',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'browser/test_management_policy.cc',
        'browser/test_management_policy.h',
        'common/extension_builder.cc',
        'common/extension_builder.h',
        'common/test_util.cc',
        'common/test_util.h',
        'common/value_builder.cc',
        'common/value_builder.h',
      ],
      # Disable c4267 warnings until we fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],
    },
  ]
}
