# Copyright (c) 2010, Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
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
# A module for parsing results.html files generated by old-run-webkit-tests
# This class is one big hack and only needs to exist until we transition to new-run-webkit-tests.

from webkitpy.common.net.resultsjsonparser import ResultsJSONParser
from webkitpy.common.system.deprecated_logging import log
from webkitpy.thirdparty.BeautifulSoup import BeautifulSoup, SoupStrainer
from webkitpy.layout_tests.layout_package import test_results
from webkitpy.layout_tests.layout_package import test_failures


# FIXME: This should be unified with all the layout test results code in the layout_tests package
# This doesn't belong in common.net, but we don't have a better place for it yet.
def path_for_layout_test(test_name):
    return "LayoutTests/%s" % test_name


class ORWTResultsHTMLParser(object):
    """This class knows how to parse old-run-webkit-tests results.html files."""

    stderr_key = u'Tests that had stderr output:'
    fail_key = u'Tests where results did not match expected results:'
    timeout_key = u'Tests that timed out:'
    # FIXME: This may need to be made aware of WebKitTestRunner results for WebKit2.
    crash_key = u'Tests that caused the DumpRenderTree tool to crash:'
    missing_key = u'Tests that had no expected results (probably new):'
    webprocess_crash_key = u'Tests that caused the Web process to crash:'

    expected_keys = [
        stderr_key,
        fail_key,
        crash_key,
        webprocess_crash_key,
        timeout_key,
        missing_key,
    ]

    @classmethod
    def _failures_from_fail_row(self, row):
        # Look at all anchors in this row, and guess what type
        # of new-run-webkit-test failures they equate to.
        failures = set()
        test_name = None
        for anchor in row.findAll("a"):
            anchor_text = unicode(anchor.string)
            if not test_name:
                test_name = anchor_text
                continue
            if anchor_text in ["expected image", "image diffs"] or '%' in anchor_text:
                failures.add(test_failures.FailureImageHashMismatch())
            elif anchor_text in ["expected", "actual", "diff", "pretty diff"]:
                failures.add(test_failures.FailureTextMismatch())
            else:
                log("Unhandled link text in results.html parsing: %s.  Please file a bug against webkitpy." % anchor_text)
        # FIXME: Its possible the row contained no links due to ORWT brokeness.
        # We should probably assume some type of failure anyway.
        return failures

    @classmethod
    def _failures_from_row(cls, row, table_title):
        if table_title == cls.fail_key:
            return cls._failures_from_fail_row(row)
        if table_title == cls.crash_key:
            return [test_failures.FailureCrash()]
        if table_title == cls.webprocess_crash_key:
            return [test_failures.FailureCrash()]
        if table_title == cls.timeout_key:
            return [test_failures.FailureTimeout()]
        if table_title == cls.missing_key:
            return [test_failures.FailureMissingResult(), test_failures.FailureMissingImageHash(), test_failures.FailureMissingImage()]
        return None

    @classmethod
    def _test_result_from_row(cls, row, table_title):
        test_name = unicode(row.find("a").string)
        failures = cls._failures_from_row(row, table_title)
        # TestResult is a class designed to work with new-run-webkit-tests.
        # old-run-webkit-tests does not save quite enough information in results.html for us to parse.
        # FIXME: It's unclear if test_name should include LayoutTests or not.
        return test_results.TestResult(test_name, failures)

    @classmethod
    def _parse_results_table(cls, table):
        table_title = unicode(table.findPreviousSibling("p").string)
        if table_title not in cls.expected_keys:
            # This Exception should only ever be hit if run-webkit-tests changes its results.html format.
            raise Exception("Unhandled title: %s" % table_title)
        # Ignore stderr failures.  Everyone ignores them anyway.
        if table_title == cls.stderr_key:
            return []
        # FIXME: We might end with two TestResults object for the same test if it appears in more than one row.
        return [cls._test_result_from_row(row, table_title) for row in table.findAll("tr")]

    @classmethod
    def parse_results_html(cls, page):
        tables = BeautifulSoup(page).findAll("table")
        return sum([cls._parse_results_table(table) for table in tables], [])


# FIXME: This should be unified with ResultsSummary or other NRWT layout tests code
# in the layout_tests package.
# This doesn't belong in common.net, but we don't have a better place for it yet.
class LayoutTestResults(object):
    @classmethod
    def results_from_string(cls, string):
        if not string:
            return None
        # For now we try to parse first as json, then as results.html
        # eventually we will remove the html fallback support.
        test_results = ResultsJSONParser.parse_results_json(string)
        if not test_results:
            test_results = ORWTResultsHTMLParser.parse_results_html(string)
        if not test_results:
            return None
        return cls(test_results)

    def __init__(self, test_results):
        self._test_results = test_results
        self._failure_limit_count = None

    # FIXME: run-webkit-tests should store the --exit-after-N-failures value
    # (or some indication of early exit) somewhere in the results.html/results.json
    # file.  Until it does, callers should set the limit to
    # --exit-after-N-failures value used in that run.  Consumers of LayoutTestResults
    # may use that value to know if absence from the failure list means PASS.
    # https://bugs.webkit.org/show_bug.cgi?id=58481
    def set_failure_limit_count(self, limit):
        self._failure_limit_count = limit

    def failure_limit_count(self):
        return self._failure_limit_count

    def test_results(self):
        return self._test_results

    def results_matching_failure_types(self, failure_types):
        return [result for result in self._test_results if result.has_failure_matching_types(*failure_types)]

    def tests_matching_failure_types(self, failure_types):
        return [result.filename for result in self.results_matching_failure_types(failure_types)]

    def failing_test_results(self):
        # These should match the "fail", "crash", and "timeout" keys.
        failure_types = [test_failures.FailureTextMismatch, test_failures.FailureImageHashMismatch, test_failures.FailureCrash, test_failures.FailureTimeout]
        return self.results_matching_failure_types(failure_types)

    def failing_tests(self):
        return [result.filename for result in self.failing_test_results()]
