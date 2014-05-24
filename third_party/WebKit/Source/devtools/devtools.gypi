#
# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#         * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#         * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#         * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

{
    'variables': {
        # If debug_devtools is set to 1, JavaScript files for DevTools are
        # stored as is. Otherwise, a concatenated file is stored.
        'debug_devtools%': 0,
        'devtools_core_js_files': [
            '<@(devtools_core_base_js_files)',
            '<@(devtools_common_js_files)',
            '<@(devtools_sdk_js_files)',
            '<@(devtools_ui_js_files)',
            '<@(devtools_components_js_files)',
        ],
        'devtools_core_base_js_files': [
            'front_end/inspector.html',
            'front_end/Tests.js',
            'front_end/EditFileSystemDialog.js',
            'front_end/ForwardedInputEventHandler.js',
            'front_end/InspectorFrontendAPI.js',
            'front_end/InspectorFrontendEventSink.js',
            'front_end/InspectorFrontendHostStub.js',
            'front_end/jsdifflib.js',
            'front_end/ResponsiveDesignView.js',
            'front_end/ScreencastView.js',
            'front_end/SettingsScreen.js',
            'front_end/TestController.js',
            'front_end/dialog.css',
            'front_end/inspector.css',
            'front_end/tabbedPane.css',
            'front_end/inspectorSyntaxHighlight.css',
            'front_end/popover.css',
            '<@(devtools_main_js_files)',
            '<@(devtools_standalone_files)',
        ],
        'devtools_common_js_files': [
            'front_end/common/Color.js',
            'front_end/common/CompletionDictionary.js',
            'front_end/common/DOMExtension.js',
            'front_end/common/Geometry.js',
            'front_end/common/ModuleManager.js',
            'front_end/common/modules.js',
            'front_end/common/Object.js',
            'front_end/common/ParsedURL.js',
            'front_end/common/Platform.js',
            'front_end/common/Progress.js',
            'front_end/common/Settings.js',
            'front_end/common/TextRange.js',
            'front_end/common/UIString.js',
            'front_end/common/UserMetrics.js',
            'front_end/common/utilities.js',
            'front_end/common/WebInspector.js',
        ],
        'devtools_sdk_js_files': [
            'front_end/sdk/ApplicationCacheModel.js',
            'front_end/sdk/CompilerScriptMapping.js',
            'front_end/sdk/ConsoleModel.js',
            'front_end/sdk/ContentProvider.js',
            'front_end/sdk/ContentProviderBasedProjectDelegate.js',
            'front_end/sdk/ContentProviders.js',
            'front_end/sdk/CookieParser.js',
            'front_end/sdk/CPUProfileModel.js',
            'front_end/sdk/CPUProfilerModel.js',
            'front_end/sdk/CSSMetadata.js',
            'front_end/sdk/CSSParser.js',
            'front_end/sdk/CSSStyleModel.js',
            'front_end/sdk/CSSStyleSheetMapping.js',
            'front_end/sdk/BreakpointManager.js',
            'front_end/sdk/DOMModel.js',
            'front_end/sdk/DOMStorage.js',
            'front_end/sdk/Database.js',
            'front_end/sdk/DebuggerModel.js',
            'front_end/sdk/DebuggerScriptMapping.js',
            'front_end/sdk/DefaultScriptMapping.js',
            'front_end/sdk/FileManager.js',
            'front_end/sdk/FileSystemMapping.js',
            'front_end/sdk/FileSystemModel.js',
            'front_end/sdk/FileSystemWorkspaceBinding.js',
            'front_end/sdk/FileUtils.js',
            'front_end/sdk/HAREntry.js',
            'front_end/sdk/IndexedDBModel.js',
            'front_end/sdk/InspectorBackend.js',
            'front_end/sdk/IsolatedFileSystemManager.js',
            'front_end/sdk/IsolatedFileSystem.js',
            'front_end/sdk/LayerTreeModel.js',
            'front_end/sdk/Linkifier.js',
            'front_end/sdk/LiveEditSupport.js',
            'front_end/sdk/NetworkLog.js',
            'front_end/sdk/NetworkManager.js',
            'front_end/sdk/NetworkRequest.js',
            'front_end/sdk/NetworkUISourceCodeProvider.js',
            'front_end/sdk/NetworkWorkspaceBinding.js',
            'front_end/sdk/NotificationService.js',
            'front_end/sdk/OverridesSupport.js',
            'front_end/sdk/PaintProfiler.js',
            'front_end/sdk/PowerProfiler.js',
            'front_end/sdk/PresentationConsoleMessageHelper.js',
            'front_end/sdk/RemoteObject.js',
            'front_end/sdk/Resource.js',
            'front_end/sdk/ResourceScriptMapping.js',
            'front_end/sdk/ResourceTreeModel.js',
            'front_end/sdk/ResourceType.js',
            'front_end/sdk/ResourceUtils.js',
            'front_end/sdk/RuntimeModel.js',
            'front_end/sdk/SASSSourceMapping.js',
            'front_end/sdk/Script.js',
            'front_end/sdk/ScriptSnippetModel.js',
            'front_end/sdk/SearchConfig.js',
            'front_end/sdk/SnippetStorage.js',
            'front_end/sdk/SourceMap.js',
            'front_end/sdk/SourceMapping.js',
            'front_end/sdk/StylesSourceMapping.js',
            'front_end/sdk/Target.js',
            'front_end/sdk/TempFile.js',
            'front_end/sdk/TimelineManager.js',
            'front_end/sdk/UISourceCode.js',
            'front_end/sdk/WorkerManager.js',
            'front_end/sdk/WorkerTargetManager.js',
            'front_end/sdk/Workspace.js',
            'front_end/sdk/WorkspaceController.js',
        ],
        'devtools_ui_js_files': [
            'front_end/ui/ActionRegistry.js',
            'front_end/ui/Checkbox.js',
            'front_end/ui/Context.js',
            'front_end/ui/ContextMenu.js',
            'front_end/ui/DataGrid.js',
            'front_end/ui/Dialog.js',
            'front_end/ui/DropDownMenu.js',
            'front_end/ui/EmptyView.js',
            'front_end/ui/InplaceEditor.js',
            'front_end/ui/KeyboardShortcut.js',
            'front_end/ui/PieChart.js',
            'front_end/ui/Popover.js',
            'front_end/ui/ProgressIndicator.js',
            'front_end/ui/ResizerWidget.js',
            'front_end/ui/SidebarPane.js',
            'front_end/ui/SidebarTreeElement.js',
            'front_end/ui/ShortcutRegistry.js',
            'front_end/ui/ShowMoreDataGridNode.js',
            'front_end/ui/SoftContextMenu.js',
            'front_end/ui/SplitView.js',
            'front_end/ui/StackView.js',
            'front_end/ui/StatusBarButton.js',
            'front_end/ui/SuggestBox.js',
            'front_end/ui/TabbedPane.js',
            'front_end/ui/TextEditor.js',
            'front_end/ui/TextPrompt.js',
            'front_end/ui/TextUtils.js',
            'front_end/ui/UIUtils.js',
            'front_end/ui/View.js',
            'front_end/ui/ViewportControl.js',
            'front_end/ui/ZoomManager.js',
            'front_end/ui/treeoutline.js',
        ],
        'devtools_components_js_files': [
            'front_end/components/CookiesTable.js',
            'front_end/components/DockController.js',
            'front_end/components/Drawer.js',
            'front_end/components/DOMBreakpointsSidebarPane.js',
            'front_end/components/DOMPresentationUtils.js',
            'front_end/components/ExecutionContextSelector.js',
            'front_end/components/ExtensionServerProxy.js',
            'front_end/components/FilterBar.js',
            'front_end/components/FilterSuggestionBuilder.js',
            'front_end/components/FlameChart.js',
            'front_end/components/HandlerRegistry.js',
            'front_end/components/HelpScreen.js',
            'front_end/components/InspectElementModeController.js',
            'front_end/components/InspectedPagePlaceholder.js',
            'front_end/components/InspectorView.js',
            'front_end/components/NativeBreakpointsSidebarPane.js',
            'front_end/components/ObjectPopoverHelper.js',
            'front_end/components/ObjectPropertiesSection.js',
            'front_end/components/OverviewGrid.js',
            'front_end/components/Panel.js',
            'front_end/components/PropertiesSection.js',
            'front_end/components/SearchableView.js',
            'front_end/components/Section.js',
            'front_end/components/SettingsUI.js',
            'front_end/components/ShortcutsScreen.js',
            'front_end/components/TimelineGrid.js',
            'front_end/components/WorkerFrontendManager.js',
        ],
        'devtools_main_js_files': [
            'front_end/main/HelpScreenUntilReload.js',
            'front_end/main/Main.js',
        ],
        'devtools_module_json_files': [
            'front_end/audits/module.json',
            'front_end/components/module.json',
            'front_end/console/module.json',
            'front_end/devices/module.json',
            'front_end/elements/module.json',
            'front_end/extensions/module.json',
            'front_end/layers/module.json',
            'front_end/main/module.json',
            'front_end/network/module.json',
            'front_end/profiler/module.json',
            'front_end/resources/module.json',
            'front_end/search/module.json',
            'front_end/settings/module.json',
            'front_end/source_frame/module.json',
            'front_end/sources/module.json',
            'front_end/timeline/module.json',
        ],
        'all_devtools_files': [
            '<@(devtools_core_js_files)',
            '<@(devtools_modules_js_files)',
            '<@(devtools_cm_css_files)',
            '<@(devtools_cm_js_files)',
            '<@(devtools_module_json_files)',
        ],
        'devtools_standalone_files': [
            'front_end/accelerometer.css',
            'front_end/auditsPanel.css',
            'front_end/breadcrumbList.css',
            'front_end/breakpointsList.css',
            'front_end/dataGrid.css',
            'front_end/devicesView.css',
            'front_end/elementsPanel.css',
            'front_end/filter.css',
            'front_end/filteredItemSelectionDialog.css',
            'front_end/flameChart.css',
            'front_end/heapProfiler.css',
            'front_end/helpScreen.css',
            'front_end/indexedDBViews.css',
            'front_end/inspectorCommon.css',
            'front_end/navigatorView.css',
            'front_end/networkLogView.css',
            'front_end/networkPanel.css',
            'front_end/overrides.css',
            'front_end/panelEnablerView.css',
            'front_end/profilesPanel.css',
            'front_end/resourceView.css',
            'front_end/resourcesPanel.css',
            'front_end/responsiveDesignView.css',
            'front_end/revisionHistory.css',
            'front_end/screencastView.css',
            'front_end/sidebarPane.css',
            'front_end/sourcesPanel.css',
            'front_end/sourcesView.css',
            'front_end/spectrum.css',
            'front_end/splitView.css',
            'front_end/suggestBox.css',
            'front_end/timelinePanel.css',
            'front_end/canvasProfiler.css',
            'front_end/layersPanel.css',
        ],
        'devtools_console_js_files': [
            'front_end/console/ConsolePanel.js',
            'front_end/console/ConsoleView.js',
            'front_end/console/ConsoleViewMessage.js',
        ],
        'devtools_search_js_files': [
            'front_end/search/AdvancedSearchView.js',
            'front_end/search/FileBasedSearchResultsPane.js',
            'front_end/search/SourcesSearchScope.js',
        ],
        'devtools_devices_js_files': [
            'front_end/devices/DevicesView.js',
        ],
        'devtools_elements_js_files': [
            'front_end/elements/DOMSyntaxHighlighter.js',
            'front_end/elements/ElementsTreeOutline.js',
            'front_end/elements/ElementsPanel.js',
            'front_end/elements/EventListenersSidebarPane.js',
            'front_end/elements/MetricsSidebarPane.js',
            'front_end/elements/OverridesView.js',
            'front_end/elements/PlatformFontsSidebarPane.js',
            'front_end/elements/PropertiesSidebarPane.js',
            'front_end/elements/RenderingOptionsView.js',
            'front_end/elements/Spectrum.js',
            'front_end/elements/StylesSidebarPane.js',
        ],
        'devtools_extensions_js_files': [
            'front_end/extensions/ExtensionAuditCategory.js',
            'front_end/extensions/ExtensionPanel.js',
            'front_end/extensions/ExtensionRegistryStub.js',
            'front_end/extensions/ExtensionServer.js',
            'front_end/extensions/ExtensionView.js',
            '<@(devtools_extension_api_files)',
        ],
        'devtools_resources_js_files': [
            'front_end/resources/ApplicationCacheItemsView.js',
            'front_end/resources/CookieItemsView.js',
            'front_end/resources/DOMStorageItemsView.js',
            'front_end/resources/DatabaseQueryView.js',
            'front_end/resources/DatabaseTableView.js',
            'front_end/resources/DirectoryContentView.js',
            'front_end/resources/FileContentView.js',
            'front_end/resources/FileSystemView.js',
            'front_end/resources/IndexedDBViews.js',
            'front_end/resources/ResourcesPanel.js',
        ],
        'devtools_source_frame_js_files': [
            'front_end/source_frame/CodeMirrorTextEditor.js',
            'front_end/source_frame/CodeMirrorUtils.js',
            'front_end/source_frame/FontView.js',
            'front_end/source_frame/GoToLineDialog.js',
            'front_end/source_frame/ImageView.js',
            'front_end/source_frame/ResourceView.js',
            'front_end/source_frame/SourceFrame.js',
        ],
        'devtools_network_js_files': [
            'front_end/network/NetworkItemView.js',
            'front_end/network/RequestCookiesView.js',
            'front_end/network/RequestHeadersView.js',
            'front_end/network/RequestHTMLView.js',
            'front_end/network/RequestJSONView.js',
            'front_end/network/RequestPreviewView.js',
            'front_end/network/RequestResponseView.js',
            'front_end/network/RequestTimingView.js',
            'front_end/network/RequestView.js',
            'front_end/network/ResourceWebSocketFrameView.js',
            'front_end/network/NetworkPanel.js',
        ],
        'devtools_sources_js_files': [
            'front_end/sources/BreakpointsSidebarPane.js',
            'front_end/sources/CSSSourceFrame.js',
            'front_end/sources/CallStackSidebarPane.js',
            'front_end/sources/EditingLocationHistoryManager.js',
            'front_end/sources/FilePathScoreFunction.js',
            'front_end/sources/FilteredItemSelectionDialog.js',
            'front_end/sources/InplaceFormatterEditorAction.js',
            'front_end/sources/JavaScriptSourceFrame.js',
            'front_end/sources/NavigatorView.js',
            'front_end/sources/Placard.js',
            'front_end/sources/RevisionHistoryView.js',
            'front_end/sources/ScopeChainSidebarPane.js',
            'front_end/sources/ScriptFormatter.js',
            'front_end/sources/ScriptFormatterEditorAction.js',
            'front_end/sources/SimpleHistoryManager.js',
            'front_end/sources/SourcesNavigator.js',
            'front_end/sources/SourcesPanel.js',
            'front_end/sources/SourcesView.js',
            'front_end/sources/StyleSheetOutlineDialog.js',
            'front_end/sources/TabbedEditorContainer.js',
            'front_end/sources/UISourceCodeFrame.js',
            'front_end/sources/WatchExpressionsSidebarPane.js',
            'front_end/sources/WorkersSidebarPane.js',
            'front_end/sources/ThreadsToolbar.js',
        ],
        'devtools_timeline_js_files': [
            'front_end/timeline/CountersGraph.js',
            'front_end/timeline/Layers3DView.js',
            'front_end/timeline/MemoryCountersGraph.js',
            'front_end/timeline/TimelineFrameModel.js',
            'front_end/timeline/TimelineJSProfile.js',
            'front_end/timeline/TimelineModel.js',
            'front_end/timeline/TimelinePresentationModel.js',
            'front_end/timeline/TimelineOverviewPane.js',
            'front_end/timeline/TimelineEventOverview.js',
            'front_end/timeline/TimelineFlameChart.js',
            'front_end/timeline/TimelineFrameOverview.js',
            'front_end/timeline/TimelineMemoryOverview.js',
            'front_end/timeline/TimelineUIUtils.js',
            'front_end/timeline/TimelineView.js',
            'front_end/timeline/TimelinePowerGraph.js',
            'front_end/timeline/TimelinePowerOverview.js',
            'front_end/timeline/TimelinePanel.js',
            'front_end/timeline/TimelineTracingView.js',
            'front_end/timeline/TimelineLayersView.js',
            'front_end/timeline/TracingModel.js',
            'front_end/timeline/TransformController.js'
        ],
        'devtools_profiler_js_files': [
            'front_end/profiler/CPUProfileBottomUpDataGrid.js',
            'front_end/profiler/CPUProfileDataGrid.js',
            'front_end/profiler/CPUProfileFlameChart.js',
            'front_end/profiler/CPUProfileTopDownDataGrid.js',
            'front_end/profiler/CPUProfileView.js',
            'front_end/profiler/HeapSnapshotCommon.js',
            'front_end/profiler/HeapSnapshotDataGrids.js',
            'front_end/profiler/HeapSnapshotGridNodes.js',
            'front_end/profiler/HeapSnapshotProxy.js',
            'front_end/profiler/HeapSnapshotView.js',
            'front_end/profiler/ProfilesPanel.js',
            'front_end/profiler/ProfileLauncherView.js',
            'front_end/profiler/CanvasProfileView.js',
            'front_end/profiler/CanvasReplayStateView.js',
        ],
        'devtools_heap_snapshot_worker_js_files': [
            'front_end/ui/TextUtils.js',
            'front_end/common/UIString.js',
            'front_end/common/utilities.js',
            'front_end/profiler/HeapSnapshotCommon.js',
            'front_end/profiler/heap_snapshot_worker/AllocationProfile.js',
            'front_end/profiler/heap_snapshot_worker/HeapSnapshot.js',
            'front_end/profiler/heap_snapshot_worker/HeapSnapshotLoader.js',
            'front_end/profiler/heap_snapshot_worker/HeapSnapshotWorker.js',
            'front_end/profiler/heap_snapshot_worker/HeapSnapshotWorkerDispatcher.js',
            'front_end/profiler/heap_snapshot_worker/JSHeapSnapshot.js',
        ],
        'devtools_audits_js_files': [
            'front_end/audits/AuditCategories.js',
            'front_end/audits/AuditCategory.js',
            'front_end/audits/AuditController.js',
            'front_end/audits/AuditFormatters.js',
            'front_end/audits/AuditLauncherView.js',
            'front_end/audits/AuditResultView.js',
            'front_end/audits/AuditRules.js',
            'front_end/audits/AuditsPanel.js',
        ],
        'devtools_cm_css_files': [
            'front_end/cm/cmdevtools.css',
            'front_end/cm/codemirror.css',
        ],
        'devtools_cm_js_files': [
            'front_end/cm/clike.js',
            'front_end/cm/closebrackets.js',
            'front_end/cm/codemirror.js',
            'front_end/cm/coffeescript.js',
            'front_end/cm/comment.js',
            'front_end/cm/css.js',
            'front_end/cm/headlesscodemirror.js',
            'front_end/cm/htmlembedded.js',
            'front_end/cm/htmlmixed.js',
            'front_end/cm/javascript.js',
            'front_end/cm/markselection.js',
            'front_end/cm/matchbrackets.js',
            'front_end/cm/overlay.js',
            'front_end/cm/php.js',
            'front_end/cm/python.js',
            'front_end/cm/shell.js',
            'front_end/cm/xml.js',
        ],
        'devtools_modules_js_files': [
            '<@(devtools_console_js_files)',
            '<@(devtools_search_js_files)',
            '<@(devtools_devices_js_files)',
            '<@(devtools_elements_js_files)',
            '<@(devtools_extensions_js_files)',
            '<@(devtools_resources_js_files)',
            '<@(devtools_network_js_files)',
            '<@(devtools_source_frame_js_files)',
            '<@(devtools_sources_js_files)',
            '<@(devtools_timeline_js_files)',
            '<@(devtools_profiler_js_files)',
            '<@(devtools_audits_js_files)',
            '<@(devtools_layers_js_files)',
            '<@(devtools_heap_snapshot_worker_js_files)',
            '<@(devtools_temp_storage_shared_worker_js_files)',
            '<@(devtools_script_formatter_worker_js_files)',
            '<@(devtools_uglify_files)',
        ],
        'devtools_uglify_files': [
            'front_end/UglifyJS/parse-js.js',
        ],
        'devtools_image_files': [
            'front_end/Images/addIcon.png',
            'front_end/Images/applicationCache.png',
            'front_end/Images/back.png',
            'front_end/Images/breakpoint.png',
            'front_end/Images/breakpoint_2x.png',
            'front_end/Images/breakpointConditional.png',
            'front_end/Images/breakpointConditional_2x.png',
            'front_end/Images/checker.png',
            'front_end/Images/cookie.png',
            'front_end/Images/database.png',
            'front_end/Images/databaseTable.png',
            'front_end/Images/deleteIcon.png',
            'front_end/Images/domain.png',
            'front_end/Images/forward.png',
            'front_end/Images/fileSystem.png',
            'front_end/Images/frame.png',
            'front_end/Images/graphLabelCalloutLeft.png',
            'front_end/Images/graphLabelCalloutRight.png',
            'front_end/Images/indexedDB.png',
            'front_end/Images/indexedDBObjectStore.png',
            'front_end/Images/indexedDBIndex.png',
            'front_end/Images/localStorage.png',
            'front_end/Images/navigationControls.png',
            'front_end/Images/navigationControls_2x.png',
            'front_end/Images/paneAddButtons.png',
            'front_end/Images/paneElementStateButtons.png',
            'front_end/Images/paneFilterButtons.png',
            'front_end/Images/paneRefreshButtons.png',
            'front_end/Images/paneSettingsButtons.png',
            'front_end/Images/popoverArrows.png',
            'front_end/Images/popoverBackground.png',
            'front_end/Images/profileGroupIcon.png',
            'front_end/Images/profileIcon.png',
            'front_end/Images/profileSmallIcon.png',
            'front_end/Images/radioDot.png',
            'front_end/Images/resourceCSSIcon.png',
            'front_end/Images/resourceDocumentIcon.png',
            'front_end/Images/resourceDocumentIconSmall.png',
            'front_end/Images/resourceJSIcon.png',
            'front_end/Images/resourcePlainIcon.png',
            'front_end/Images/resourcePlainIconSmall.png',
            'front_end/Images/resourcesTimeGraphIcon.png',
            'front_end/Images/searchNext.png',
            'front_end/Images/searchPrev.png',
            'front_end/Images/sessionStorage.png',
            'front_end/Images/settingsListRemove.png',
            'front_end/Images/settingsListRemove_2x.png',
            'front_end/Images/statusbarButtonGlyphs.png',
            'front_end/Images/statusbarButtonGlyphs_2x.png',
            'front_end/Images/statusbarResizerHorizontal.png',
            'front_end/Images/statusbarResizerVertical.png',
            'front_end/Images/thumbActiveHoriz.png',
            'front_end/Images/thumbActiveVert.png',
            'front_end/Images/thumbHoriz.png',
            'front_end/Images/thumbVert.png',
            'front_end/Images/thumbHoverHoriz.png',
            'front_end/Images/thumbHoverVert.png',
            'front_end/Images/toolbarItemSelected.png',
            'front_end/Images/trackHoriz.png',
            'front_end/Images/trackVert.png',
        ],
        'devtools_layers_js_files': [
            'front_end/layers/LayersPanel.js',
            'front_end/layers/LayerTreeOutline.js',
            'front_end/layers/LayerDetailsView.js',
            'front_end/layers/PaintProfilerView.js'
        ],
        'devtools_extension_api_files': [
            'front_end/extensions/ExtensionAPI.js',
        ],
        'devtools_temp_storage_shared_worker_js_files': [
            'front_end/temp_storage_shared_worker/TempStorageSharedWorker.js',
        ],
        'devtools_script_formatter_worker_js_files': [
            'front_end/script_formatter_worker/CSSFormatter.js',
            'front_end/script_formatter_worker/JavaScriptFormatter.js',
            'front_end/script_formatter_worker/ScriptFormatterWorker.js',
            'front_end/common/utilities.js',
        ],
    },
}
