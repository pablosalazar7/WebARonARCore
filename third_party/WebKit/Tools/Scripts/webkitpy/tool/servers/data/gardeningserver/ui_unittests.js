module("iu");

var kExampleResultsByTest = {
    "scrollbars/custom-scrollbar-with-incomplete-style.html": {
        "Mock Builder": {
            "expected": "IMAGE",
            "actual": "CRASH"
        },
        "Mock Linux": {
            "expected": "TEXT",
            "actual": "CRASH"
        }
    },
    "userscripts/another-test.html": {
        "Mock Builder": {
            "expected": "PASS",
            "actual": "TEXT"
        }
    }
}

test("summarizeTest", 3, function() {
    var testName = 'userscripts/another-test.html';
    var summary = ui.summarizeTest(testName, kExampleResultsByTest[testName]);
    var summaryHTML = summary.html();
    ok(summaryHTML.indexOf('scrollbars/custom-scrollbar-with-incomplete-style.html') == -1);
    ok(summaryHTML.indexOf('userscripts/another-test.html') != -1);
    ok(summaryHTML.indexOf('Mock Builder') != -1);
});

test("summarizeRegressionRange", 2, function() {
    var summaryWithMultipleRevisions = ui.summarizeRegressionRange(90424, 90426);
    summaryWithMultipleRevisions.wrap('<wrapper></wrapper>');
    equal(summaryWithMultipleRevisions.parent().html(), '<a class="regression-range" href="http://trac.webkit.org/log/trunk/?rev=90427&amp;stop_rev=90424&amp;limit=100&amp;verbose=on">Regression 90427:90424</a>');

    var summaryWithOneRevision = ui.summarizeRegressionRange(90425, 90426);
    summaryWithOneRevision.wrap('<wrapper></wrapper>');
    equal(summaryWithOneRevision.parent().html(), '<a class="regression-range" href="http://trac.webkit.org/log/trunk/?rev=90427&amp;stop_rev=90425&amp;limit=100&amp;verbose=on">Regression 90427:90425</a>');
});

test("results", 1, function() {
    var testResults = ui.results([
        'http://example.com/layout-test-results/foo-bar-expected.txt',
        'http://example.com/layout-test-results/foo-bar-actual.txt',
        'http://example.com/layout-test-results/foo-bar-diff.txt',
        'http://example.com/layout-test-results/foo-bar-expected.png',
        'http://example.com/layout-test-results/foo-bar-actual.png',
        'http://example.com/layout-test-results/foo-bar-diff.png',
    ]);
    equal(testResults.html(),
        '<iframe src="http://example.com/layout-test-results/foo-bar-expected.txt" class="expected"></iframe>' +
        '<iframe src="http://example.com/layout-test-results/foo-bar-actual.txt" class="actual"></iframe>' +
        '<iframe src="http://example.com/layout-test-results/foo-bar-diff.txt" class="diff"></iframe>' +
        '<img src="http://example.com/layout-test-results/foo-bar-expected.png" class="expected">' +
        '<img src="http://example.com/layout-test-results/foo-bar-actual.png" class="actual">' +
        '<img src="http://example.com/layout-test-results/foo-bar-diff.png" class="diff">');
});
