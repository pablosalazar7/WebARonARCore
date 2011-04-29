// To run these tests, load json_results.html in a browser.
// You should see a series of PASS lines.
if (window.layoutTestController)
    layoutTestController.dumpAsText();

var testStyles = document.createElement('style');
testStyles.innerText = ".test-pass { color: green; } .test-fail { color: red; }";
document.querySelector('head').appendChild(testStyles);

var g_testIndex = 0;
var g_log = ["You should see a serios of PASS lines."];

// Make async actually be sync for the sake of simpler testing.
function async(func, args)
{
    func.apply(null, args);
}

function mockResults()
{
    return {
        tests: {},
        "skipped": 0,
        "num_regressions": 0,
        "version": 0,
        "num_passes": 0,
        "fixable": 0,
        "num_flaky": 0,
        "layout_tests_dir": "/WEBKITROOT",
        "uses_expectations_file": true,
        "has_pretty_patch": false,
        "has_wdiff": false,
        "revision": 12345
    };
}

function mockExpectation(expected, actual)
{
    return {
        expected: expected,
        time_ms: 1,
        actual: actual,
        has_stderr: false
    };
}

function logPass(msg)
{
    g_log.push('TEST-' + g_testIndex + ': <span class="test-pass">' + msg + '</span>')
}

function logFail(msg)
{
    g_log.push('TEST-' + g_testIndex + ': <span class="test-fail">' + msg + '</span>')
}

function assertTrue(bool)
{
    if (bool)
        logPass('PASS');
    else
        logFail('FAIL');
}

function runTest(results, assertions)
{
    document.body.innerHTML = '';
    g_testIndex++;
    g_state = undefined;

    try {
        ADD_RESULTS(results);
        originalGeneratePage();
    } catch (e) {
        logFail("FAIL: uncaught exception " + e.toString());
    }
    
    try {
        assertions();
    } catch (e) {
        logFail("FAIL: uncaught exception executing assertions " + e.toString());
    }
}

function runDefaultSingleRowTest(test, expected, actual, isExpected, textResults, imageResults)
{
    results = mockResults();
    results.tests[test] = mockExpectation(expected, actual);
    runSingleRowTest(results, isExpected, textResults, imageResults);
}

function runSingleRowTest(results, isExpected, textResults, imageResults)
{
    for (var key in results.tests)
        var test = key;
    var expected = results.tests[test].expected;
    var actual = results.tests[test].actual;
    runTest(results, function() {
        if (isExpected)
            assertTrue(document.querySelector('tbody').className == 'expected');
        else
            assertTrue(document.querySelector('tbody').className.indexOf('expected') == -1);

        assertTrue(document.querySelector('tbody td:nth-child(1)').textContent == '+' + test);
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent == textResults);
        assertTrue(document.querySelector('tbody td:nth-child(3)').textContent == imageResults);
        assertTrue(document.querySelector('tbody td:nth-child(4)').textContent == actual);
        assertTrue(document.querySelector('tbody td:nth-child(5)').textContent == expected);
    });
    
}

function runTests()
{
    var results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('PASS', 'TEXT');
    runTest(results, function() {
        assertTrue(document.getElementById('image-results-header').textContent == '');
        assertTrue(document.getElementById('text-results-header').textContent != '');
    });
    
    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'MISSING');
    subtree['bar.html'].is_missing_text = true;
    subtree['bar.html'].is_missing_audio = true;
    subtree['bar.html'].is_missing_image = true;
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(document.querySelector('#new-tests-table .test-link').textContent == 'foo/bar.html');
        assertTrue(document.querySelector('#new-tests-table .result-link:nth-child(1)').textContent == 'audio result');
        assertTrue(document.querySelector('#new-tests-table .result-link:nth-child(2)').textContent == 'result');
        assertTrue(document.querySelector('#new-tests-table .result-link:nth-child(3)').textContent == 'png result');
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar.html'].has_stderr = true;
    runTest(results, function() {
        assertTrue(document.getElementById('results-table'));
        assertTrue(document.querySelector('#stderr-table .result-link').textContent == 'stderr');
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar1.html'] = mockExpectation('CRASH', 'PASS');
    subtree['bar2.html'] = mockExpectation('IMAGE', 'PASS');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));

        var testLinks = document.querySelectorAll('#passes-table .test-link');
        assertTrue(testLinks[0].textContent == 'foo/bar.html');
        assertTrue(testLinks[1].textContent == 'foo/bar1.html');
        assertTrue(testLinks[2].textContent == 'foo/bar2.html');

        var expectationTypes = document.querySelectorAll('#passes-table td:last-of-type');
        assertTrue(expectationTypes[0].textContent == 'TEXT');
        assertTrue(expectationTypes[1].textContent == 'CRASH');
        assertTrue(expectationTypes[2].textContent == 'IMAGE');
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar-missing.html'] = mockExpectation('TEXT', 'MISSING');
    subtree['bar-missing.html'].is_missing_text = true;
    subtree['bar-stderr.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar-stderr.html'].has_stderr = true;
    subtree['bar-unexpected-pass.html'] = mockExpectation('TEXT', 'PASS');
    runTest(results, function() {
        assertTrue(document.querySelectorAll('tbody tr').length == 5);
        expandAllExpectations();
        assertTrue(document.querySelectorAll('tbody tr').length == 10);
        var expandLinks = document.querySelectorAll('.expand-button-text');
        var enDash = '\u2013';
        for (var i = 0; i < expandLinks.length; i++) {
            assertTrue(expandLinks[i].textContent == enDash);
        }
        
        collapseAllExpectations();
        // Collapsed expectations stay in the dom, but are display:none.
        assertTrue(document.querySelectorAll('tbody tr').length == 10);
        var expandLinks = document.querySelectorAll('.expand-button-text');
        for (var i = 0; i < expandLinks.length; i++)
            assertTrue(expandLinks[i].textContent == '+');
            
        expandExpectations(expandLinks[1]);
        assertTrue(expandLinks[0].textContent == '+');
        assertTrue(expandLinks[1].textContent == enDash);

        collapseExpectations(expandLinks[1]);
        assertTrue(expandLinks[1].textContent == '+');
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar-expected-fail.html'] = mockExpectation('TEXT', 'TEXT');
    runTest(results, function() {
        assertTrue(document.querySelectorAll('.expected').length == 1);
        assertTrue(document.querySelector('.expected .test-link').textContent == 'foo/bar-expected-fail.html');

        expandAllExpectations();
        assertTrue(visibleExpandLinks().length == 1);
        assertTrue(document.querySelectorAll('.results-row').length == 1);
        
        document.querySelector('.unexpected-results').checked = false;

        assertTrue(visibleExpandLinks().length == 2);
        assertTrue(document.querySelectorAll('.results-row').length == 1);
        
        expandAllExpectations();
        assertTrue(document.querySelectorAll('.results-row').length == 2);
    });
  
    runDefaultSingleRowTest('bar-skip.html', 'TEXT', 'SKIP', true, '', '');
    runDefaultSingleRowTest('bar-flaky-fail.html', 'PASS FAIL', 'TEXT', true, 'expected actual diff ', '');
    runDefaultSingleRowTest('bar-flaky-fail-unexpected.html', 'PASS TEXT', 'IMAGE', false, '', 'expected actual diff ');
    runDefaultSingleRowTest('bar-crash.html', 'TEXT', 'CRASH', false, 'stack ', '');
    runDefaultSingleRowTest('bar-audio.html', 'TEXT', 'AUDIO', false, 'expected audio actual audio ', '');
    runDefaultSingleRowTest('bar-timeout.html', 'TEXT', 'TIMEOUT', false, 'expected actual diff ', '');
    runDefaultSingleRowTest('bar-image.html', 'TEXT', 'IMAGE', false, '', 'expected actual diff ');
    runDefaultSingleRowTest('bar-image-plus-text.html', 'TEXT', 'IMAGE+TEXT', false, 'expected actual diff ', 'expected actual diff ');

    results = mockResults();
    results.tests['bar-reftest.html'] = mockExpectation('PASS', 'IMAGE');
    results.tests['bar-reftest.html'].is_reftest = true;
    runSingleRowTest(results, false, '', 'ref html expected actual diff ');

    results = mockResults();
    results.tests['bar-reftest-mismatch.html'] = mockExpectation('PASS', 'IMAGE');
    results.tests['bar-reftest-mismatch.html'].is_mismatch_reftest = true;
    runSingleRowTest(results, false, '', 'ref mismatch html actual ');

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar-flaky-pass.html'] = mockExpectation('PASS TEXT', 'PASS');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(document.getElementById('passes-table'));
        assertTrue(document.body.textContent.indexOf('foo/bar-flaky-pass.html') != -1);
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar-flaky-fail.html'] = mockExpectation('PASS TEXT', 'IMAGE PASS');
    runTest(results, function() {
        assertTrue(document.getElementById('results-table'));
        assertTrue(document.body.textContent.indexOf('bar-flaky-fail.html') != -1);
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar-really-long-path-that-should-probably-wrap-because-otherwise-the-table-will-be-too-wide.html'] = mockExpectation('PASS', 'TEXT');
    runTest(results, function() {
        document.body.style.width = '800px';
        var links = document.querySelectorAll('tbody a');
        assertTrue(links[0].getClientRects().length == 2);
        assertTrue(links[1].getClientRects().length == 1);
        document.body.style.width = '';
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'TEXT');
    results.uses_expectations_file = false;
    runTest(results, function() {
        assertTrue(document.querySelectorAll('tbody td').length == 4);
        assertTrue(document.querySelector('tbody').className.indexOf('expected') == -1);
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'TEXT');
    results.has_pretty_patch = true;
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent.indexOf('pretty diff') != -1);
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent.indexOf('wdiff') == -1);
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'TEXT');
    results.has_wdiff = true;
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent.indexOf('wdiff') != -1);
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent.indexOf('pretty diff') == -1);
    });
    
    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar-1.html'] = mockExpectation('TEXT', 'CRASH');
    subtree['bar-5.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    subtree['bar-3.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar-2.html'] = mockExpectation('PASS', 'IMAGE');
    runTest(results, function() {
        // FIXME: This just ensures we don't get a JS error.
        // Verify that the sort is correct and that inline expanded expectations
        // move along with the test they're attached to.
        TableSorter.sortColumn(0);
        TableSorter.sortColumn(0);
        TableSorter.sortColumn(1);
        TableSorter.sortColumn(1);
        TableSorter.sortColumn(2);
        TableSorter.sortColumn(2);
        TableSorter.sortColumn(3);
        TableSorter.sortColumn(3);
        TableSorter.sortColumn(4);
        TableSorter.sortColumn(4);
        TableSorter.sortColumn(0);
        logPass('PASS');
    });

    results = mockResults();
    var subtree = results.tests['foo'] = {}
    subtree['bar-5.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    runTest(results, function() {
        expandAllExpectations();
        var png = document.querySelector('[src*="bar-5-expected.png"]');
        var x = png.offsetLeft + 1;
        var y = png.offsetTop + 1;
        var mockEvent = {
            target: png,
            clientX: x,
            clientY: y
        }
        PixelZoomer.showOnDelay = false;
        PixelZoomer.handleMouseMove(mockEvent);
        assertTrue(!!document.querySelector('.pixel-zoom-container'));
        assertTrue(document.querySelectorAll('.zoom-image-container').length == 3);
    });
    
    results = mockResults();
    var subtree = results.tests['fullscreen'] = {}
    subtree['full-screen-api.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    runTest(results, function() {
        // Use a regexp to match windows and unix-style paths.
        var expectedRegExp = new RegExp('^file.*' + results.layout_tests_dir + '/fullscreen/full-screen-api.html$');
        assertTrue(expectedRegExp.exec(document.querySelector('tbody td:first-child a').href));
    });

    var oldShouldUseTracLinks = shouldUseTracLinks;
    shouldUseTracLinks = function() { return true; };
    
    results = mockResults();
    var subtree = results.tests['fullscreen'] = {}
    subtree['full-screen-api.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    runTest(results, function() {
        var expectedHref = 'http://trac.webkit.org/export/' + results.revision + '/trunk/LayoutTests/fullscreen/full-screen-api.html';
        assertTrue(document.querySelector('tbody td:first-child a').href == expectedHref);
    });

    results = mockResults();
    var subtree = results.tests['fullscreen'] = {}
    subtree['full-screen-api.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    results.revision = '';
    runTest(results, function() {
        var expectedHref = 'http://trac.webkit.org/browser/trunk/LayoutTests/fullscreen/full-screen-api.html';
        assertTrue(document.querySelector('tbody td:first-child a').href == expectedHref);
    });

    shouldUseTracLinks = oldShouldUseTracLinks;

    runDefaultSingleRowTest('bar-flaky-crash.html', 'TEXT', 'TIMEOUT CRASH AUDIO', false, 'expected actual diff stack expected audio actual audio ', '');

    document.body.innerHTML = '<pre>' + g_log.join('\n') + '</pre>';
}

var originalGeneratePage = generatePage;
generatePage = runTests;
