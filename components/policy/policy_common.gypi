# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  # GN version: //components/policy/core/common
  'dependencies': [
    '../base/base.gyp:base',
  ],
  'defines': [
    'POLICY_COMPONENT_IMPLEMENTATION',
  ],
  'include_dirs': [
    '..',
  ],
  'conditions': [
    ['configuration_policy==1', {
      'dependencies': [
        '../base/base.gyp:base_prefs',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../google_apis/google_apis.gyp:google_apis',
        '../net/net.gyp:net',
        '../third_party/re2/re2.gyp:re2',
        '../url/url.gyp:url_lib',
        'cloud_policy_proto',
        'json_schema',
        'policy',
      ],
      'sources': [
        'core/common/async_policy_loader.cc',
        'core/common/async_policy_loader.h',
        'core/common/async_policy_provider.cc',
        'core/common/async_policy_provider.h',
        'core/common/cloud/cloud_external_data_manager.cc',
        'core/common/cloud/cloud_external_data_manager.h',
        'core/common/cloud/cloud_policy_client.cc',
        'core/common/cloud/cloud_policy_client.h',
        'core/common/cloud/cloud_policy_client_registration_helper.cc',
        'core/common/cloud/cloud_policy_client_registration_helper.h',
        'core/common/cloud/cloud_policy_constants.cc',
        'core/common/cloud/cloud_policy_constants.h',
        'core/common/cloud/cloud_policy_core.cc',
        'core/common/cloud/cloud_policy_core.h',
        'core/common/cloud/cloud_policy_manager.cc',
        'core/common/cloud/cloud_policy_manager.h',
        'core/common/cloud/cloud_policy_refresh_scheduler.cc',
        'core/common/cloud/cloud_policy_refresh_scheduler.h',
        'core/common/cloud/cloud_policy_service.cc',
        'core/common/cloud/cloud_policy_service.h',
        'core/common/cloud/cloud_policy_store.cc',
        'core/common/cloud/cloud_policy_store.h',
        'core/common/cloud/cloud_policy_validator.cc',
        'core/common/cloud/cloud_policy_validator.h',
        'core/common/cloud/component_cloud_policy_service.cc',
        'core/common/cloud/component_cloud_policy_service.h',
        'core/common/cloud/component_cloud_policy_store.cc',
        'core/common/cloud/component_cloud_policy_store.h',
        'core/common/cloud/component_cloud_policy_updater.cc',
        'core/common/cloud/component_cloud_policy_updater.h',
        'core/common/cloud/device_management_service.cc',
        'core/common/cloud/device_management_service.h',
        'core/common/cloud/enterprise_metrics.cc',
        'core/common/cloud/enterprise_metrics.h',
        'core/common/cloud/external_policy_data_fetcher.cc',
        'core/common/cloud/external_policy_data_fetcher.h',
        'core/common/cloud/external_policy_data_updater.cc',
        'core/common/cloud/external_policy_data_updater.h',
        'core/common/cloud/policy_header_io_helper.cc',
        'core/common/cloud/policy_header_io_helper.h',
        'core/common/cloud/policy_header_service.cc',
        'core/common/cloud/policy_header_service.h',
        'core/common/cloud/resource_cache.cc',
        'core/common/cloud/resource_cache.h',
        'core/common/cloud/system_policy_request_context.cc',
        'core/common/cloud/system_policy_request_context.h',
        'core/common/cloud/user_cloud_policy_manager.cc',
        'core/common/cloud/user_cloud_policy_manager.h',
        'core/common/cloud/user_cloud_policy_store.cc',
        'core/common/cloud/user_cloud_policy_store.h',
        'core/common/cloud/user_cloud_policy_store_base.cc',
        'core/common/cloud/user_cloud_policy_store_base.h',
        'core/common/cloud/user_info_fetcher.cc',
        'core/common/cloud/user_info_fetcher.h',
        'core/common/config_dir_policy_loader.cc',
        'core/common/config_dir_policy_loader.h',
        'core/common/configuration_policy_provider.cc',
        'core/common/configuration_policy_provider.h',
        'core/common/external_data_fetcher.cc',
        'core/common/external_data_fetcher.h',
        'core/common/external_data_manager.h',
        'core/common/mac_util.cc',
        'core/common/mac_util.h',
        'core/common/policy_bundle.cc',
        'core/common/policy_bundle.h',
        'core/common/policy_details.h',
        'core/common/policy_load_status.cc',
        'core/common/policy_load_status.h',
        'core/common/policy_loader_ios.h',
        'core/common/policy_loader_ios.mm',
        'core/common/policy_loader_mac.h',
        'core/common/policy_loader_mac.mm',
        'core/common/policy_loader_win.cc',
        'core/common/policy_loader_win.h',
        'core/common/policy_map.cc',
        'core/common/policy_map.h',
        'core/common/policy_namespace.cc',
        'core/common/policy_namespace.h',
        'core/common/policy_pref_names.cc',
        'core/common/policy_pref_names.h',
        'core/common/policy_service.cc',
        'core/common/policy_service.h',
        'core/common/policy_service_impl.cc',
        'core/common/policy_service_impl.h',
        'core/common/policy_statistics_collector.cc',
        'core/common/policy_statistics_collector.h',
        'core/common/policy_switches.cc',
        'core/common/policy_switches.h',
        'core/common/policy_types.h',
        'core/common/preferences_mac.cc',
        'core/common/preferences_mac.h',
        'core/common/preg_parser_win.cc',
        'core/common/preg_parser_win.h',
        'core/common/registry_dict_win.cc',
        'core/common/registry_dict_win.h',
        'core/common/remote_commands/remote_command_job.cc',
        'core/common/remote_commands/remote_command_job.h',
        'core/common/remote_commands/remote_commands_factory.cc',
        'core/common/remote_commands/remote_commands_factory.h',
        'core/common/remote_commands/remote_commands_queue.cc',
        'core/common/remote_commands/remote_commands_queue.h',
        'core/common/remote_commands/remote_commands_service.cc',
        'core/common/remote_commands/remote_commands_service.h',
        'core/common/schema.cc',
        'core/common/schema.h',
        'core/common/schema_internal.h',
        'core/common/schema_map.cc',
        'core/common/schema_map.h',
        'core/common/schema_registry.cc',
        'core/common/schema_registry.h',
        'core/common/schema_registry_tracking_policy_provider.cc',
        'core/common/schema_registry_tracking_policy_provider.h',
        'policy_export.h',
      ],
      'conditions': [
        ['OS=="android"', {
          'sources!': [
            'core/common/async_policy_loader.cc',
            'core/common/async_policy_loader.h',
            'core/common/async_policy_provider.cc',
            'core/common/async_policy_provider.h',
          ],
        }],
        ['OS=="android" or OS=="ios"', {
          'sources': [
            'core/common/cloud/component_cloud_policy_service_stub.cc',
          ],
          'sources!': [
            'core/common/cloud/component_cloud_policy_service.cc',
            'core/common/cloud/component_cloud_policy_store.cc',
            'core/common/cloud/component_cloud_policy_store.h',
            'core/common/cloud/component_cloud_policy_updater.cc',
            'core/common/cloud/component_cloud_policy_updater.h',
            'core/common/cloud/external_policy_data_fetcher.cc',
            'core/common/cloud/external_policy_data_fetcher.h',
            'core/common/cloud/external_policy_data_updater.cc',
            'core/common/cloud/external_policy_data_updater.h',
            'core/common/cloud/resource_cache.cc',
            'core/common/cloud/resource_cache.h',
            'core/common/config_dir_policy_loader.cc',
            'core/common/config_dir_policy_loader.h',
            'core/common/policy_load_status.cc',
            'core/common/policy_load_status.h',
          ],
        }],
        ['chromeos==1', {
          'sources': [
            'core/common/proxy_policy_provider.cc',
            'core/common/proxy_policy_provider.h',
          ],
          'sources!': [
            'core/common/cloud/cloud_policy_client_registration_helper.cc',
            'core/common/cloud/cloud_policy_client_registration_helper.h',
            'core/common/cloud/user_cloud_policy_manager.cc',
            'core/common/cloud/user_cloud_policy_manager.h',
            'core/common/cloud/user_cloud_policy_store.cc',
            'core/common/cloud/user_cloud_policy_store.h',
          ],
        }],
        ['OS!="ios" and OS!="mac"', {
          'sources!': [
            'core/common/mac_util.cc',
            'core/common/mac_util.h',
          ],
        }],
      ],
    }, {  # configuration_policy==0
      # Some of the policy code is always enabled, so that other parts of
      # Chrome can always interface with the PolicyService without having
      # to #ifdef on ENABLE_CONFIGURATION_POLICY.
      'sources': [
        'core/common/external_data_fetcher.cc',
        'core/common/external_data_fetcher.h',
        'core/common/external_data_manager.h',
        'core/common/policy_map.cc',
        'core/common/policy_map.h',
        'core/common/policy_namespace.cc',
        'core/common/policy_namespace.h',
        'core/common/policy_pref_names.cc',
        'core/common/policy_pref_names.h',
        'core/common/policy_service.cc',
        'core/common/policy_service.h',
        'core/common/policy_service_stub.cc',
        'core/common/policy_service_stub.h',
      ],
    }],
  ],
}
