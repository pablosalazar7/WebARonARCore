// Copyright (C) 2011 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module('flakiness_dashboard');

// FIXME(jparent): Rename this once it isn't globals.
function resetGlobals()
{
    allExpectations = null;
    allTests = null;
    g_expectationsByPlatform = {};
    g_resultsByBuilder = {};
    g_allExpectations = null;
    g_allTestsTrie = null;
    var historyInstance = new history.History(flakinessConfig);
    // FIXME(jparent): Remove this once global isn't used.
    g_history = historyInstance;
    g_testToResultsMap = {};

    for (var key in history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES)
        historyInstance.crossDashboardState[key] = history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES[key];

    LOAD_BUILDBOT_DATA([{
        name: 'ChromiumWebkit',
        url: 'dummyurl', 
        tests: {'layout-tests': {'builders': ['WebKit Linux', 'WebKit Linux (dbg)', 'WebKit Mac10.7', 'WebKit Win', 'WebKit Win (dbg)']}}
    }]);
 
    for (var group in LAYOUT_TESTS_BUILDER_GROUPS)
        LAYOUT_TESTS_BUILDER_GROUPS[group] = null;

    return historyInstance;
}

test('getParsedExpectationsCommentLine', 1, function() {
    var expectations  = getParsedExpectations('# Comment line crbug.com/12345 [ Release ] tests/foobar.html [ Failure ]');
    equal(expectations.length, 0, 'Number of expectations');
});

test('getParsedExpectationsSimpleInput', 4, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 [ Release ] tests/foobar.html [ Failure ]');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345 RELEASE', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'FAIL', 'expectations');
});

test('getParsedExpectationsSimpleInputWithComment', 4, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 [ Release ] tests/foobar.html [ Failure ] # Comment');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345 RELEASE', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'FAIL', 'expectations');
});

test('getParsedExpectationsOnlyBug', 1, function() {
    var expectations  = getParsedExpectations('crbug.com/12345');
    equal(expectations.length, 0, 'Number of expectations');
});

test('getParsedExpectationsTwoBugs', 4, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 tests/foobar.html [ Failure ]');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'FAIL', 'expectations');
});

test('getParsedExpectationsNoBug', 4, function() {
    var expectations  = getParsedExpectations('tests/foobar.html [ Failure ]');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, '', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'FAIL', 'expectations');
});

test('getParsedExpectationsBugPrefixInPath', 4, function() {
    var expectations = getParsedExpectations('Bug12345 Bug67890 tests/Bug.html [ Failure ]');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'Bug12345 Bug67890', 'modifiers');
    equal(expectations[0].path, 'tests/Bug.html', 'path');
    equal(expectations[0].expectations, 'FAIL', 'expectations');
});

test('getParsedExpectationsTwoModifiers', 4, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 [ Release Debug ] tests/foobar.html [ Failure ]');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345 RELEASE DEBUG', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'FAIL', 'expectations');
});

test('getParsedExpectationsUnknownModifier', 1, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 [ ImaginaryOS ] tests/foobar.html');
    equal(expectations.length, 0, 'Number of expectations');
});

test('getParsedExpectationsTwoPaths', 1, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 tests/foo.html tests/bar.html [ Failure ]');
    equal(expectations.length, 0, 'Number of expectations');
});

test('getParsedExpectationsNoPath', 1, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 [ Failure ]');
    equal(expectations.length, 0, 'Number of expectations');
});

test('getParsedExpectationsHashInPath', 1, function() {
    var expectations = getParsedExpectations('crbug.com/12345 # [ Failure ]');
    equal(expectations.length, 0, 'Number of expectations');
});

test('getParsedExpectationsTwoExpectations', 4, function() {
    expectations  = getParsedExpectations('crbug.com/12345 tests/foobar.html [ Pass Failure ]');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'PASS FAIL', 'expectations');
});

test('getParsedExpectationsNoExpectation', 4, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 tests/foobar.html');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345 Skip', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'Pass', 'expectations');
});

test('getParsedExpectationsNoExpectationWithComment', 4, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 tests/foobar.html # Comment');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345 Skip', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'Pass', 'expectations');
});

test('getParsedExpectationsExpectationConversionToModifier', 4, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 tests/foobar.html [ Rebaseline ]');
    equal(expectations.length, 1, 'Number of expectations');
    equal(expectations[0].modifiers, 'crbug.com/12345 REBASELINE Skip', 'modifiers');
    equal(expectations[0].path, 'tests/foobar.html', 'path');
    equal(expectations[0].expectations, 'Pass', 'expectations');
});

test('getParsedExpectationsUnknownExpectation', 1, function() {
    var expectations  = getParsedExpectations('crbug.com/12345 tests/foobar.html [ PANIC ]');
    equal(expectations.length, 0, 'Number of expectations');
});

function stubResultsByBuilder(data)
{
    for (var builder in currentBuilders())
    {
        g_resultsByBuilder[builder] = data[builder] || {'tests': []};
    };
}

function runExpectationsTest(builder, test, expectations, modifiers)
{
    // Put in some dummy results. processExpectations expects the test to be
    // there.
    var tests = {};
    tests[test] = {'results': [[100, 'F']], 'times': [[100, 0]]};
    var results = {};
    results[builder] = {'tests': tests};
    stubResultsByBuilder(results);

    processExpectations();
    var resultsForTest = createResultsObjectForTest(test, builder);
    populateExpectationsData(resultsForTest);

    var message = 'Builder: ' + resultsForTest.builder + ' test: ' + resultsForTest.test;
    equal(resultsForTest.expectations, expectations, message);
    equal(resultsForTest.modifiers, modifiers, message);
}

test('releaseFail', 2, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');

    var builder = 'WebKit Win';
    var test = 'foo/1.html';
    var expectationsArray = [
        {'modifiers': 'RELEASE', 'expectations': 'FAIL'}
    ];
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('[ Release ] ' + test + ' [ Failure ]');
    runExpectationsTest(builder, test, 'FAIL', 'RELEASE');
});

test('releaseFailDebugCrashReleaseBuilder', 2, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
    var builder = 'WebKit Win';
    var test = 'foo/1.html';
    var expectationsArray = [
        {'modifiers': 'RELEASE', 'expectations': 'FAIL'},
        {'modifiers': 'DEBUG', 'expectations': 'CRASH'}
    ];
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('[ Release ] ' + test + ' [ Failure ]\n' +
        '[ Debug ] ' + test + ' [ Crash ]');
    runExpectationsTest(builder, test, 'FAIL', 'RELEASE');
});

test('releaseFailDebugCrashDebugBuilder', 2, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
    var builder = 'WebKit Win (dbg)';
    var test = 'foo/1.html';
    var expectationsArray = [
        {'modifiers': 'RELEASE', 'expectations': 'FAIL'},
        {'modifiers': 'DEBUG', 'expectations': 'CRASH'}
    ];
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('[ Release ] ' + test + ' [ Failure ]\n' +
        '[ Debug ] ' + test + ' [ Crash ]');
    runExpectationsTest(builder, test, 'CRASH', 'DEBUG');
});

test('overrideJustBuildType', 12, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
    var test = 'bar/1.html';
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('bar [ WontFix Failure Pass Timeout ]\n' +
        '[ Mac ] ' + test + ' [ WontFix Failure ]\n' +
        '[ Linux Debug ] ' + test + ' [ Crash ]');
    
    runExpectationsTest('WebKit Win', test, 'FAIL PASS TIMEOUT', 'WONTFIX');
    runExpectationsTest('WebKit Win (dbg)(3)', test, 'FAIL PASS TIMEOUT', 'WONTFIX');
    runExpectationsTest('WebKit Linux', test, 'FAIL PASS TIMEOUT', 'WONTFIX');
    runExpectationsTest('WebKit Linux (dbg)(3)', test, 'CRASH', 'LINUX DEBUG');
    runExpectationsTest('WebKit Mac10.7', test, 'FAIL', 'MAC WONTFIX');
    runExpectationsTest('WebKit Mac10.7 (dbg)(3)', test, 'FAIL', 'MAC WONTFIX');
});

test('platformAndBuildType', 42, function() {
    var historyInstance = new history.History(flakinessConfig);
    // FIXME(jparent): Change to use the flakiness_db's history object
    // once it exists, rather than tracking global.
    g_history = historyInstance;

    var runPlatformAndBuildTypeTest = function(builder, expectedPlatform, expectedBuildType) {
        g_perBuilderPlatformAndBuildType = {};
        buildInfo = platformAndBuildType(builder);
        var message = 'Builder: ' + builder;
        equal(buildInfo.platform, expectedPlatform, message);
        equal(buildInfo.buildType, expectedBuildType, message);
    }
    runPlatformAndBuildTypeTest('WebKit Win (deps)', 'CHROMIUM_XP', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Win (deps)(dbg)(1)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Win (deps)(dbg)(2)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux (deps)', 'CHROMIUM_LUCID', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Linux (deps)(dbg)(1)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux (deps)(dbg)(2)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (deps)', 'CHROMIUM_SNOWLEOPARD', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (deps)(dbg)(1)', 'CHROMIUM_SNOWLEOPARD', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (deps)(dbg)(2)', 'CHROMIUM_SNOWLEOPARD', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Win', 'CHROMIUM_XP', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Win7', 'CHROMIUM_WIN7', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Win (dbg)(1)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Win (dbg)(2)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux', 'CHROMIUM_LUCID', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Linux 32', 'CHROMIUM_LUCID', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Linux (dbg)(1)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux (dbg)(2)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Mac10.6', 'CHROMIUM_SNOWLEOPARD', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (dbg)', 'CHROMIUM_SNOWLEOPARD', 'DEBUG');
    runPlatformAndBuildTypeTest('XP Tests', 'CHROMIUM_XP', 'RELEASE');
    runPlatformAndBuildTypeTest('Interactive Tests (dbg)', 'CHROMIUM_XP', 'DEBUG');
});

test('realModifiers', 3, function() {
    equal(realModifiers('BUG(Foo) LINUX LION WIN DEBUG SLOW'), 'SLOW');
    equal(realModifiers('BUG(Foo) LUCID MAC XP RELEASE SKIP'), 'SKIP');
    equal(realModifiers('BUG(Foo)'), '');
});

test('allTestsWithSamePlatformAndBuildType', 1, function() {
    // FIXME: test that allTestsWithSamePlatformAndBuildType actually returns the right set of tests.
    var expectedPlatformsList = ['CHROMIUM_LION', 'CHROMIUM_SNOWLEOPARD', 'CHROMIUM_XP', 'CHROMIUM_VISTA', 'CHROMIUM_WIN7', 'CHROMIUM_LUCID',
                                 'CHROMIUM_ANDROID', 'APPLE_MAC_LION_WK1', 'APPLE_MAC_LION_WK2', 'APPLE_MAC_SNOWLEOPARD_WK1', 'APPLE_MAC_SNOWLEOPARD_WK2',
                                 'APPLE_WIN_XP', 'APPLE_WIN_WIN7',  'GTK_LINUX_WK1', 'GTK_LINUX_WK2', 'QT_LINUX', 'EFL_LINUX_WK1', 'EFL_LINUX_WK2'];
    var actualPlatformsList = Object.keys(g_allTestsByPlatformAndBuildType);
    deepEqual(expectedPlatformsList, actualPlatformsList);
});

test('filterBugs',4, function() {
    var filtered = filterBugs('Skip crbug.com/123 webkit.org/b/123 Slow Bug(Tony) Debug')
    equal(filtered.modifiers, 'Skip Slow Debug');
    equal(filtered.bugs, 'crbug.com/123 webkit.org/b/123 Bug(Tony)');

    filtered = filterBugs('Skip Slow Debug')
    equal(filtered.modifiers, 'Skip Slow Debug');
    equal(filtered.bugs, '');
});

test('getExpectations', 16, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
 
    stubResultsByBuilder({
        'WebKit Win' : {
            'tests': {
                'foo/test1.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
                'foo/test2.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
                'foo/test3.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
                'test1.html': {'results': [[100, 'F']], 'times': [[100, 0]]}
            }
        }
    });

    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('Bug(123) foo [ Failure Pass Crash ]\n' +
        'Bug(Foo) [ Release ] foo/test1.html [ Failure ]\n' +
        '[ Debug ] foo/test1.html [ Crash ]\n' +
        'Bug(456) foo/test2.html [ Failure ]\n' +
        '[ Linux Debug ] foo/test2.html [ Crash ]\n' +
        '[ Release ] test1.html [ Failure ]\n' +
        '[ Debug ] test1.html [ Crash ]\n');
    g_expectationsByPlatform['CHROMIUM_ANDROID'] = getParsedExpectations('Bug(654) foo/test2.html [ Crash ]\n');

    g_expectationsByPlatform['GTK'] = getParsedExpectations('Bug(42) foo/test2.html [ ImageOnlyFailure ]\n' +
        '[ Debug ] test1.html [ Crash ]\n');
    g_expectationsByPlatform['GTK_LINUX_WK1'] = getParsedExpectations('[ Release ] foo/test1.html [ ImageOnlyFailure ]\n' +
        'Bug(789) foo/test2.html [ Crash ]\n');
    g_expectationsByPlatform['GTK_LINUX_WK2'] = getParsedExpectations('Bug(987) foo/test2.html [ Failure ]\n');

    processExpectations();
    
    var expectations = getExpectations('foo/test1.html', 'CHROMIUM_XP', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('foo/test1.html', 'CHROMIUM_LUCID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(Foo) RELEASE","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_LUCID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(456)","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_LION', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(456)","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_LUCID', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"LINUX DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_ANDROID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(654)","expectations":"CRASH"}');

    var expectations = getExpectations('test1.html', 'CHROMIUM_ANDROID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"RELEASE","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test3.html', 'CHROMIUM_LUCID', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(123)","expectations":"FAIL PASS CRASH"}');

    var expectations = getExpectations('test1.html', 'CHROMIUM_XP', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('test1.html', 'CHROMIUM_LUCID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"RELEASE","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test1.html', 'GTK_LINUX_WK1', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"RELEASE","expectations":"IMAGE"}');

    var expectations = getExpectations('foo/test2.html', 'GTK_LINUX_WK1', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(789)","expectations":"CRASH"}');

    var expectations = getExpectations('test1.html', 'GTK_LINUX_WK1', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('foo/test2.html', 'GTK_LINUX_WK2', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(987)","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'GTK_LINUX_WK2', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(987)","expectations":"FAIL"}');

    var expectations = getExpectations('test1.html', 'GTK_LINUX_WK2', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');
});

test('substringList', 2, function() {
    var historyInstance = new history.History(flakinessConfig);
    // FIXME(jparent): Remove this once global isn't used.
    g_history = historyInstance;
    historyInstance.crossDashboardState.testType = 'gtest';
    historyInstance.dashboardSpecificState.tests = 'test.FLAKY_foo test.FAILS_foo1 test.DISABLED_foo2 test.MAYBE_foo3 test.foo4';
    equal(substringList().toString(), 'test.foo,test.foo1,test.foo2,test.foo3,test.foo4');

    historyInstance.crossDashboardState.testType = 'layout-tests';
    historyInstance.dashboardSpecificState.tests = 'foo/bar.FLAKY_foo.html';
    equal(substringList().toString(), 'foo/bar.FLAKY_foo.html');
});

test('htmlForTestsWithExpectationsButNoFailures', 4, function() {
    var historyInstance = new history.History(defaultDashboardSpecificStateValues, generatePage, handleValidHashParameter);
    // FIXME(jparent): Remove this once global isn't used.
    g_history = historyInstance;
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
    var builder = 'WebKit Win';
    g_perBuilderWithExpectationsButNoFailures[builder] = ['passing-test1.html', 'passing-test2.html'];
    g_perBuilderSkippedPaths[builder] = ['skipped-test1.html'];
    g_resultsByBuilder[builder] = { buildNumbers: [5, 4, 3, 1] };

    historyInstance.dashboardSpecificState.showUnexpectedPasses = true;
    historyInstance.dashboardSpecificState.showSkipped = true;

    historyInstance.crossDashboardState.group = '@ToT - chromium.org';
    historyInstance.crossDashboardState.testType = 'layout-tests';
    
    var container = document.createElement('div');
    container.innerHTML = htmlForTestsWithExpectationsButNoFailures(builder);
    equal(container.querySelectorAll('#passing-tests > div').length, 2);
    equal(container.querySelectorAll('#skipped-tests > div').length, 1);
    
    historyInstance.dashboardSpecificState.showUnexpectedPasses = false;
    historyInstance.dashboardSpecificState.showSkipped = false;
    
    var container = document.createElement('div');
    container.innerHTML = htmlForTestsWithExpectationsButNoFailures(builder);
    equal(container.querySelectorAll('#passing-tests > div').length, 0);
    equal(container.querySelectorAll('#skipped-tests > div').length, 0);
});

test('headerForTestTableHtml', 1, function() {
    var container = document.createElement('div');
    container.innerHTML = headerForTestTableHtml();
    equal(container.querySelectorAll('input').length, 5);
});

test('htmlForTestTypeSwitcherGroup', 6, function() {
    var historyInstance = new history.History(flakinessConfig);
    // FIXME(jparent): Remove this once global isn't used.
    g_history = historyInstance;
    var container = document.createElement('div');
    historyInstance.crossDashboardState.testType = 'ui_tests';
    container.innerHTML = ui.html.testTypeSwitcher(true);
    var selects = container.querySelectorAll('select');
    equal(selects.length, 2);
    var group = selects[1];
    equal(group.parentNode.textContent.indexOf('Group:'), 0);
    equal(group.children.length, 3);

    historyInstance.crossDashboardState.testType = 'layout-tests';
    container.innerHTML = ui.html.testTypeSwitcher(true);
    var selects = container.querySelectorAll('select');
    equal(selects.length, 2);
    var group = selects[1];
    equal(group.parentNode.textContent.indexOf('Group:'), 0);
    equal(group.children.length, 3);
});

test('htmlForIndividualTestOnAllBuilders', 1, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
    equal(htmlForIndividualTestOnAllBuilders('foo/nonexistant.html'), '<div class="not-found">Test not found. Either it does not exist, is skipped or passes on all platforms.</div>');
});

test('htmlForIndividualTestOnAllBuildersWithResultsLinksNonexistant', 1, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
    equal(htmlForIndividualTestOnAllBuildersWithResultsLinks('foo/nonexistant.html'),
        '<div class="not-found">Test not found. Either it does not exist, is skipped or passes on all platforms.</div>' +
        '<div class=expectations test=foo/nonexistant.html>' +
            '<div>' +
                '<span class=link onclick="g_history.setQueryParameter(\'showExpectations\', true)">Show results</span> | ' +
                '<span class=link onclick="g_history.setQueryParameter(\'showLargeExpectations\', true)">Show large thumbnails</span> | ' +
                '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b>' +
            '</div>' +
        '</div>');
});

test('htmlForIndividualTestOnAllBuildersWithResultsLinks', 1, function() {
    resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');

    var builderName = 'WebKit Linux';
    var test = 'dummytest.html';
    g_testToResultsMap[test] = [createResultsObjectForTest(test, builderName)];

    equal(htmlForIndividualTestOnAllBuildersWithResultsLinks(test),
        '<table class=test-table><thead><tr>' +
                '<th sortValue=test><div class=table-header-content><span></span><span class=header-text>test</span></div></th>' +
                '<th sortValue=bugs><div class=table-header-content><span></span><span class=header-text>bugs</span></div></th>' +
                '<th sortValue=modifiers><div class=table-header-content><span></span><span class=header-text>modifiers</span></div></th>' +
                '<th sortValue=expectations><div class=table-header-content><span></span><span class=header-text>expectations</span></div></th>' +
                '<th sortValue=slowest><div class=table-header-content><span></span><span class=header-text>slowest run</span></div></th>' +
                '<th sortValue=flakiness colspan=10000><div class=table-header-content><span></span><span class=header-text>flakiness (numbers are runtimes in seconds)</span></div></th>' +
            '</tr></thead>' +
            '<tbody></tbody>' +
        '</table>' +
        '<div>The following builders either don\'t run this test (e.g. it\'s skipped) or all runs passed:</div>' +
        '<div class=skipped-builder-list>' +
            '<div class=skipped-builder>WebKit Linux (dbg)</div><div class=skipped-builder>WebKit Mac10.7</div><div class=skipped-builder>WebKit Win</div><div class=skipped-builder>WebKit Win (dbg)</div>' +
        '</div>' +
        '<div class=expectations test=dummytest.html>' +
            '<div><span class=link onclick="g_history.setQueryParameter(\'showExpectations\', true)">Show results</span> | ' +
            '<span class=link onclick="g_history.setQueryParameter(\'showLargeExpectations\', true)">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>');
});

test('htmlForIndividualTests', 4, function() {
    var historyInstance = resetGlobals();
    loadBuildersList('@ToT - chromium.org', 'layout-tests');
    var test1 = 'foo/nonexistant.html';
    var test2 = 'bar/nonexistant.html';

    historyInstance.dashboardSpecificState.showChrome = false;

    var tests = [test1, test2];
    equal(htmlForIndividualTests(tests),
        '<h2><a href="' + TEST_URL_BASE_PATH_IN_VERSION_CONTROL + 'foo/nonexistant.html" target="_blank">foo/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuilders(test1) + 
        '<div class=expectations test=foo/nonexistant.html>' +
            '<div><span class=link onclick=\"g_history.setQueryParameter(\'showExpectations\', true)\">Show results</span> | ' +
            '<span class=link onclick=\"g_history.setQueryParameter(\'showLargeExpectations\', true)\">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>' +
        '<hr>' +
        '<h2><a href="' + TEST_URL_BASE_PATH_IN_VERSION_CONTROL + 'bar/nonexistant.html" target="_blank">bar/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuilders(test2) +
        '<div class=expectations test=bar/nonexistant.html>' +
            '<div><span class=link onclick=\"g_history.setQueryParameter(\'showExpectations\', true)\">Show results</span> | ' +
            '<span class=link onclick=\"g_history.setQueryParameter(\'showLargeExpectations\', true)\">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>');

    tests = [test1];
    equal(htmlForIndividualTests(tests), htmlForIndividualTestOnAllBuilders(test1) +
        '<div class=expectations test=foo/nonexistant.html>' +
            '<div><span class=link onclick=\"g_history.setQueryParameter(\'showExpectations\', true)\">Show results</span> | ' +
            '<span class=link onclick=\"g_history.setQueryParameter(\'showLargeExpectations\', true)\">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>');

    historyInstance.dashboardSpecificState.showChrome = true;

    equal(htmlForIndividualTests(tests),
        '<h2><a href="' + TEST_URL_BASE_PATH_IN_VERSION_CONTROL + 'foo/nonexistant.html" target="_blank">foo/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuildersWithResultsLinks(test1));

    tests = [test1, test2];
    equal(htmlForIndividualTests(tests),
        '<h2><a href="' + TEST_URL_BASE_PATH_IN_VERSION_CONTROL + 'foo/nonexistant.html" target="_blank">foo/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuildersWithResultsLinks(test1) + '<hr>' +
        '<h2><a href="' + TEST_URL_BASE_PATH_IN_VERSION_CONTROL + 'bar/nonexistant.html" target="_blank">bar/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuildersWithResultsLinks(test2));
});

test('htmlForSingleTestRow', 1, function() {
    var historyInstance = resetGlobals();
    var builder = 'dummyBuilder';
    BUILDER_TO_MASTER[builder] = CHROMIUM_WEBKIT_BUILDER_MASTER;
    var test = createResultsObjectForTest('foo/exists.html', builder);
    historyInstance.dashboardSpecificState.showCorrectExpectations = true;
    g_resultsByBuilder[builder] = {buildNumbers: [2, 1], blinkRevision: [1234, 1233]};
    test.rawResults = [[1, 'F'], [2, 'I']];
    test.rawTimes = [[1, 0], [2, 5]];
    var expected = '<tr>' +
        '<td class="test-link"><span class="link" onclick="g_history.setQueryParameter(\'tests\',\'foo/exists.html\');">foo/exists.html</span>' +
        '<td class=options-container><a href="https://bugs.webkit.org/enter_bug.cgi?assigned_to=webkit-unassigned%40lists.webkit.org&product=WebKit&form_name=enter_bug&component=Tools%20%2F%20Tests&short_desc=Layout%20Test%20foo%2Fexists.html%20is%20failing&comment=The%20following%20layout%20test%20is%20failing%20on%20%5Binsert%20platform%5D%0A%0Afoo%2Fexists.html%0A%0AProbable%20cause%3A%0A%0A%5Binsert%20probable%20cause%5D" class="file-bug">FILE BUG</a>' +
        '<td class=options-container>' +
            '<td class=options-container>' +
                '<td><td title="TEXT. Click for more info." class="results F" onclick=\'showPopupForBuild(event, "dummyBuilder",0,"foo/exists.html")\'>&nbsp;' +
                '<td title="IMAGE. Click for more info." class="results I" onclick=\'showPopupForBuild(event, "dummyBuilder",1,"foo/exists.html")\'>5';

    equal(htmlForSingleTestRow(test), expected);
});

test('lookupVirtualTestSuite', 2, function() {
    equal(lookupVirtualTestSuite('fast/canvas/foo.html'), '');
    equal(lookupVirtualTestSuite('platform/chromium/virtual/gpu/fast/canvas/foo.html'), 'platform/chromium/virtual/gpu/fast/canvas');
});

test('baseTest', 2, function() {
    equal(baseTest('fast/canvas/foo.html', ''), 'fast/canvas/foo.html');
    equal(baseTest('platform/chromium/virtual/gpu/fast/canvas/foo.html', 'platform/chromium/virtual/gpu/fast/canvas'), 'fast/canvas/foo.html');
});

// FIXME: Create builders_tests.js and move this there.

test('isChromiumWebkitTipOfTreeTestRunner', 1, function() {
    var builderList = ["WebKit Linux", "WebKit Linux (dbg)", "WebKit Linux 32", "WebKit Mac10.6", "WebKit Mac10.6 (dbg)",
        "WebKit Mac10.6 (deps)", "WebKit Mac10.7", "WebKit Win", "WebKit Win (dbg)(1)", "WebKit Win (dbg)(2)", "WebKit Win (deps)",
        "WebKit Win7", "Linux (Content Shell)"];
    var expectedBuilders = ["WebKit Linux", "WebKit Linux (dbg)", "WebKit Linux 32", "WebKit Mac10.6",
        "WebKit Mac10.6 (dbg)", "WebKit Mac10.7", "WebKit Win", "WebKit Win (dbg)(1)", "WebKit Win (dbg)(2)", "WebKit Win7"];
    deepEqual(builderList.filter(isChromiumWebkitTipOfTreeTestRunner), expectedBuilders);
});

test('isChromiumWebkitDepsTestRunner', 1, function() {
    var builderList = ["Chrome Frame Tests", "GPU Linux (NVIDIA)", "GPU Linux (dbg) (NVIDIA)", "GPU Mac", "GPU Mac (dbg)", "GPU Win7 (NVIDIA)", "GPU Win7 (dbg) (NVIDIA)", "Linux Perf", "Linux Tests",
        "Linux Valgrind", "Mac Builder (dbg)", "Mac10.6 Perf", "Mac10.6 Tests", "Vista Perf", "Vista Tests", "WebKit Linux", "WebKit Linux ASAN",  "WebKit Linux (dbg)", "WebKit Linux (deps)", "WebKit Linux 32",
        "WebKit Mac10.6", "WebKit Mac10.6 (dbg)", "WebKit Mac10.6 (deps)", "WebKit Mac10.7", "WebKit Win", "WebKit Win (dbg)(1)", "WebKit Win (dbg)(2)", "WebKit Win (deps)",
        "WebKit Win7", "Win (dbg)", "Win Builder"];
    var expectedBuilders = ["WebKit Linux (deps)", "WebKit Mac10.6 (deps)", "WebKit Win (deps)"];
    deepEqual(builderList.filter(isChromiumWebkitDepsTestRunner), expectedBuilders);
});

test('builderGroupIsToTWebKitAttribute', 2, function() {
    var dummyMaster = new builders.BuilderMaster('Chromium', 'dummyurl', {'layout-tests': {'builders': ['WebKit Linux', 'WebKit Linux (dbg)', 'WebKit Mac10.7', 'WebKit Win']}});
    var testBuilderGroups = {
        '@ToT - dummy.org': new BuilderGroup(BuilderGroup.TOT_WEBKIT),
        '@DEPS - dummy.org': new BuilderGroup(BuilderGroup.DEPS_WEBKIT),
    }

    var testJSONData = "{ \"Dummy Builder 1\": null, \"Dummy Builder 2\": null }";
    requestBuilderList(testBuilderGroups, 'ChromiumWebkit', '@ToT - dummy.org', testBuilderGroups['@ToT - dummy.org'], 'layout-tests');
    equal(testBuilderGroups['@ToT - dummy.org'].isToTWebKit, true);
    requestBuilderList(testBuilderGroups, 'ChromiumWebkit', '@DEPS - dummy.org', testBuilderGroups['@DEPS - dummy.org'], 'layout-tests');
    equal(testBuilderGroups['@DEPS - dummy.org'].isToTWebKit, false);
});

test('requestBuilderListAddsBuilderGroupEntry', 1, function() {
    var testBuilderGroups = { '@ToT - dummy.org': null };
    var builderGroup = new BuilderGroup(BuilderGroup.TOT_WEBKIT);
    var groupName = '@ToT - dummy.org';
    requestBuilderList(testBuilderGroups, 'ChromiumWebkit', groupName, builderGroup, 'layout-tests');

    equal(testBuilderGroups['@ToT - dummy.org'], builderGroup);
})

test('sortTests', 4, function() {
    var test1 = createResultsObjectForTest('foo/test1.html', 'dummyBuilder');
    var test2 = createResultsObjectForTest('foo/test2.html', 'dummyBuilder');
    var test3 = createResultsObjectForTest('foo/test3.html', 'dummyBuilder');
    test1.modifiers = 'b';
    test2.modifiers = 'a';
    test3.modifiers = '';

    var tests = [test1, test2, test3];
    sortTests(tests, 'modifiers', FORWARD);
    deepEqual(tests, [test2, test1, test3]);
    sortTests(tests, 'modifiers', BACKWARD);
    deepEqual(tests, [test3, test1, test2]);

    test1.bugs = 'b';
    test2.bugs = 'a';
    test3.bugs = '';

    var tests = [test1, test2, test3];
    sortTests(tests, 'bugs', FORWARD);
    deepEqual(tests, [test2, test1, test3]);
    sortTests(tests, 'bugs', BACKWARD);
    deepEqual(tests, [test3, test1, test2]);
});

test('popup', 2, function() {
    ui.popup.show(document.body, 'dummy content');
    ok(document.querySelector('#popup'));
    ui.popup.hide();
    ok(!document.querySelector('#popup'));
});

test('gpuResultsPath', 3, function() {
  equal(gpuResultsPath('777777', 'Win7 Release (ATI)'), '777777_Win7_Release_ATI_');
  equal(gpuResultsPath('123', 'GPU Linux (dbg)(NVIDIA)'), '123_GPU_Linux_dbg_NVIDIA_');
  equal(gpuResultsPath('12345', 'GPU Mac'), '12345_GPU_Mac');
});

test('TestTrie', 3, function() {
    var builders = {
        "Dummy Chromium Windows Builder": true,
        "Dummy GTK Linux Builder": true,
        "Dummy Apple Mac Lion Builder": true
    };

    var resultsByBuilder = {
        "Dummy Chromium Windows Builder": {
            tests: {
                "foo": true,
                "foo/bar/1.html": true,
                "foo/bar/baz": true
            }
        },
        "Dummy GTK Linux Builder": {
            tests: {
                "bar": true,
                "foo/1.html": true,
                "foo/bar/2.html": true,
                "foo/bar/baz/1.html": true,
            }
        },
        "Dummy Apple Mac Lion Builder": {
            tests: {
                "foo/bar/3.html": true,
                "foo/bar/baz/foo": true,
            }
        }
    };
    var expectedTrie = {
        "foo": {
            "bar": {
                "1.html": true,
                "2.html": true,
                "3.html": true,
                "baz": {
                    "1.html": true,
                    "foo": true
                }
            },
            "1.html": true
        },
        "bar": true
    }

    var trie = new TestTrie(builders, resultsByBuilder);
    deepEqual(trie._trie, expectedTrie);

    var leafsOfCompleteTrieTraversal = [];
    var expectedLeafs = ["foo/bar/1.html", "foo/bar/baz/1.html", "foo/bar/baz/foo", "foo/bar/2.html", "foo/bar/3.html", "foo/1.html", "bar"];
    trie.forEach(function(triePath) {
        leafsOfCompleteTrieTraversal.push(triePath);
    });
    deepEqual(leafsOfCompleteTrieTraversal, expectedLeafs);

    var leafsOfPartialTrieTraversal = [];
    expectedLeafs = ["foo/bar/1.html", "foo/bar/baz/1.html", "foo/bar/baz/foo", "foo/bar/2.html", "foo/bar/3.html"];
    trie.forEach(function(triePath) {
        leafsOfPartialTrieTraversal.push(triePath);
    }, "foo/bar");
    deepEqual(leafsOfPartialTrieTraversal, expectedLeafs);
});

test('changeTestTypeInvalidatesGroup', 1, function() {
    var historyInstance = resetGlobals();
    var originalGroup = '@ToT - chromium.org';
    var originalTestType = 'layout-tests';
    loadBuildersList(originalGroup, originalTestType);
    historyInstance.crossDashboardState.group = originalGroup;
    historyInstance.crossDashboardState.testType = originalTestType;

    historyInstance.invalidateQueryParameters({'testType': 'ui_tests'});
    notEqual(historyInstance.crossDashboardState.group, originalGroup, "group should have been invalidated");   
});

test('shouldHideTest', 10, function() {
    var historyInstance = new history.History(flakinessConfig);
    historyInstance.parseParameters();
    // FIXME(jparent): Change to use the flakiness_dashboard's history object
    // once it exists, rather than tracking global.
    g_history = historyInstance;
    var test = createResultsObjectForTest('foo/test.html', 'dummyBuilder');

    equal(shouldHideTest(test), true, 'default layout test, hide it.');
    historyInstance.dashboardSpecificState.showCorrectExpectations = true;
    equal(shouldHideTest(test), false, 'show correct expectations.');
    historyInstance.dashboardSpecificState.showCorrectExpectations = false;

    test = createResultsObjectForTest('foo/test.html', 'dummyBuilder');
    test.isWontFixSkip = true;
    equal(shouldHideTest(test), true, 'by default hide these too');
    historyInstance.dashboardSpecificState.showWontFixSkip = true;
    equal(shouldHideTest(test), false, 'now we should show it');
    historyInstance.dashboardSpecificState.showWontFixSkip = false;

    test = createResultsObjectForTest('foo/test.html', 'dummyBuilder');
    test.isFlaky = true;
    equal(shouldHideTest(test), false, 'we show flaky tests by default');
    historyInstance.dashboardSpecificState.showFlaky = false;
    equal(shouldHideTest(test), true, 'do not show flaky test');
    historyInstance.dashboardSpecificState.showFlaky = true;

    test = createResultsObjectForTest('foo/test.html', 'dummyBuilder');
    test.slowestNonTimeoutCrashTime = MIN_SECONDS_FOR_SLOW_TEST + 1;
    equal(shouldHideTest(test), true, 'we hide slow tests by default');
    historyInstance.dashboardSpecificState.showSlow = true;
    equal(shouldHideTest(test), false, 'now show slow test');
    historyInstance.dashboardSpecificState.showSlow = false;

    test = createResultsObjectForTest('foo/test.html', 'dummyBuilder');
    historyInstance.crossDashboardState.testType = 'not layout tests';
    equal(shouldHideTest(test), false, 'show all non layout tests');
    test.isWontFixSkip = true;
    equal(shouldHideTest(test), false, 'show all non layout tests, even if wont fix');
});
