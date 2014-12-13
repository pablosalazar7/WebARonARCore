# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import StringIO
import sys

from telemetry import benchmark
from telemetry import user_story
from telemetry.core import exceptions
from telemetry.page import page as page_module
from telemetry.page import page_test
from telemetry.page import test_expectations
from telemetry.results import results_options
from telemetry.unittest_util import options_for_unittests
from telemetry.unittest_util import system_stub
from telemetry.user_story import shared_user_story_state
from telemetry.user_story import user_story_runner
from telemetry.user_story import user_story_set
from telemetry.util import cloud_storage
from telemetry.util import exception_formatter as exception_formatter_module
from telemetry.value import scalar
from telemetry.value import string

# This linter complains if we define classes nested inside functions.
# pylint: disable=bad-super-call


class FakePlatform(object):
  def CanMonitorThermalThrottling(self):
    return False


class TestSharedUserStoryState(shared_user_story_state.SharedUserStoryState):

  _platform = FakePlatform()

  @classmethod
  def SetTestPlatform(cls, platform):
    cls._platform = platform

  def __init__(self, test, options, user_story_setz):
    super(TestSharedUserStoryState, self).__init__(
        test, options, user_story_setz)
    self._test = test
    self._current_user_story = None

  @property
  def platform(self):
    return self._platform

  def WillRunUserStory(self, user_storyz):
    self._current_user_story = user_storyz

  def GetTestExpectationAndSkipValue(self, expectations):
    return 'pass', None

  def RunUserStory(self, results):
    self._test.RunPage(self._current_user_story, None, results)


  def DidRunUserStory(self, results):
    pass

  def TearDownState(self, results):
    pass


class FooUserStoryState(TestSharedUserStoryState):
  pass


class BarUserStoryState(TestSharedUserStoryState):
  pass


class DummyTest(page_test.PageTest):
  def RunPage(self, *_):
    pass

  def ValidateAndMeasurePage(self, page, tab, results):
    pass


class EmptyMetadataForTest(benchmark.BenchmarkMetadata):
  def __init__(self):
    super(EmptyMetadataForTest, self).__init__('')


class DummyLocalUserStory(user_story.UserStory):
  def __init__(self, shared_user_story_state_class, name=''):
    super(DummyLocalUserStory, self).__init__(
        shared_user_story_state_class, name=name)

  @property
  def is_local(self):
    return True

def _GetOptionForUnittest():
  options = options_for_unittests.GetCopy()
  options.output_formats = ['none']
  options.suppress_gtest_report = True
  parser = options.CreateParser()
  user_story_runner.AddCommandLineArgs(parser)
  options.MergeDefaultValues(parser.get_default_values())
  user_story_runner.ProcessCommandLineArgs(parser, options)
  return options


class FakeExceptionFormatterModule(object):
  @staticmethod
  def PrintFormattedException(
      exception_class=None, exception=None, tb=None, msg=None):
    pass


def GetNumberOfSuccessfulPageRuns(results):
  return len([run for run in results.all_page_runs if run.ok or run.skipped])


class UserStoryRunnerTest(unittest.TestCase):

  def setUp(self):
    self.options = _GetOptionForUnittest()
    self.expectations = test_expectations.TestExpectations()
    self.results = results_options.CreateResults(
        EmptyMetadataForTest(), self.options)
    self._user_story_runner_logging_stub = None

  def SuppressExceptionFormatting(self):
    ''' Fake out exception formatter to avoid spamming the unittest stdout. '''
    user_story_runner.exception_formatter = FakeExceptionFormatterModule
    self._user_story_runner_logging_stub = system_stub.Override(
      user_story_runner, ['logging'])

  def RestoreExceptionFormatter(self):
    user_story_runner.exception_formatter = exception_formatter_module
    if self._user_story_runner_logging_stub:
      self._user_story_runner_logging_stub.Restore()
      self._user_story_runner_logging_stub = None

  def tearDown(self):
    self.RestoreExceptionFormatter()

  def testGetUserStoryGroupsWithSameSharedUserStoryClass(self):
    us = user_story_set.UserStorySet()
    us.AddUserStory(DummyLocalUserStory(FooUserStoryState))
    us.AddUserStory(DummyLocalUserStory(FooUserStoryState))
    us.AddUserStory(DummyLocalUserStory(BarUserStoryState))
    us.AddUserStory(DummyLocalUserStory(FooUserStoryState))
    story_groups = (
        user_story_runner.GetUserStoryGroupsWithSameSharedUserStoryClass(
            us))
    self.assertEqual(len(story_groups), 3)
    self.assertEqual(story_groups[0].shared_user_story_state_class,
                     FooUserStoryState)
    self.assertEqual(story_groups[1].shared_user_story_state_class,
                     BarUserStoryState)
    self.assertEqual(story_groups[2].shared_user_story_state_class,
                     FooUserStoryState)

  def testSuccessfulUserStoryTest(self):
    us = user_story_set.UserStorySet()
    us.AddUserStory(DummyLocalUserStory(FooUserStoryState))
    us.AddUserStory(DummyLocalUserStory(FooUserStoryState))
    us.AddUserStory(DummyLocalUserStory(BarUserStoryState))
    user_story_runner.Run(
        DummyTest(), us, self.expectations, self.options, self.results)
    self.assertEquals(0, len(self.results.failures))
    self.assertEquals(3, GetNumberOfSuccessfulPageRuns(self.results))

  def testTearDownIsCalledOnceForEachUserStoryGroupWithPageSetRepeat(self):
    self.options.pageset_repeat = 3
    us = user_story_set.UserStorySet()
    fooz_init_call_counter = [0]
    fooz_tear_down_call_counter = [0]
    barz_init_call_counter = [0]
    barz_tear_down_call_counter = [0]
    class FoozUserStoryState(FooUserStoryState):
      def __init__(self, test, options, user_story_setz):
        super(FoozUserStoryState, self).__init__(
          test, options, user_story_setz)
        fooz_init_call_counter[0] += 1
      def TearDownState(self, _results):
        fooz_tear_down_call_counter[0] += 1

    class BarzUserStoryState(BarUserStoryState):
      def __init__(self, test, options, user_story_setz):
        super(BarzUserStoryState, self).__init__(
          test, options, user_story_setz)
        barz_init_call_counter[0] += 1
      def TearDownState(self, _results):
        barz_tear_down_call_counter[0] += 1

    us.AddUserStory(DummyLocalUserStory(FoozUserStoryState))
    us.AddUserStory(DummyLocalUserStory(FoozUserStoryState))
    us.AddUserStory(DummyLocalUserStory(BarzUserStoryState))
    us.AddUserStory(DummyLocalUserStory(BarzUserStoryState))
    user_story_runner.Run(
        DummyTest(), us, self.expectations, self.options, self.results)
    self.assertEquals(0, len(self.results.failures))
    self.assertEquals(12, GetNumberOfSuccessfulPageRuns(self.results))
    self.assertEquals(1, fooz_init_call_counter[0])
    self.assertEquals(1, fooz_tear_down_call_counter[0])
    self.assertEquals(1, barz_init_call_counter[0])
    self.assertEquals(1, barz_tear_down_call_counter[0])

  def testAppCrashExceptionCausesFailureValue(self):
    self.SuppressExceptionFormatting()
    us = user_story_set.UserStorySet()
    class SharedUserStoryThatCausesAppCrash(TestSharedUserStoryState):
      def WillRunUserStory(self, user_storyz):
        raise exceptions.AppCrashException()

    us.AddUserStory(DummyLocalUserStory(SharedUserStoryThatCausesAppCrash))
    user_story_runner.Run(
        DummyTest(), us, self.expectations, self.options, self.results)
    self.assertEquals(1, len(self.results.failures))
    self.assertEquals(0, GetNumberOfSuccessfulPageRuns(self.results))

  def testUnknownExceptionIsFatal(self):
    self.SuppressExceptionFormatting()
    us = user_story_set.UserStorySet()

    class UnknownException(Exception):
      pass

    class Test(page_test.PageTest):
      def __init__(self, *args):
        super(Test, self).__init__(*args)
        self.run_count = 0

      def RunPage(self, *_):
        old_run_count = self.run_count
        self.run_count += 1
        if old_run_count == 0:
          raise UnknownException

      def ValidateAndMeasurePage(self, page, tab, results):
        pass

    us.AddUserStory(DummyLocalUserStory(TestSharedUserStoryState))
    us.AddUserStory(DummyLocalUserStory(TestSharedUserStoryState))
    test = Test()
    with self.assertRaises(UnknownException):
      user_story_runner.Run(
          test, us, self.expectations, self.options, self.results)

  def testRaiseBrowserGoneExceptionFromRunPage(self):
    self.SuppressExceptionFormatting()
    us = user_story_set.UserStorySet()

    class Test(page_test.PageTest):
      def __init__(self, *args):
        super(Test, self).__init__(*args)
        self.run_count = 0

      def RunPage(self, *_):
        old_run_count = self.run_count
        self.run_count += 1
        if old_run_count == 0:
          raise exceptions.BrowserGoneException('i am a browser instance')

      def ValidateAndMeasurePage(self, page, tab, results):
        pass

    us.AddUserStory(DummyLocalUserStory(TestSharedUserStoryState))
    us.AddUserStory(DummyLocalUserStory(TestSharedUserStoryState))
    test = Test()
    user_story_runner.Run(
        test, us, self.expectations, self.options, self.results)
    self.assertEquals(2, test.run_count)
    self.assertEquals(1, len(self.results.failures))
    self.assertEquals(1, GetNumberOfSuccessfulPageRuns(self.results))

  def testAppCrashThenRaiseInTearDownFatal(self):
    self.SuppressExceptionFormatting()
    us = user_story_set.UserStorySet()

    class DidRunTestError(Exception):
      pass

    class TestTearDownSharedUserStoryState(TestSharedUserStoryState):
      def TearDownState(self, results):
        self._test.DidRunTest('app', results)

    class Test(page_test.PageTest):
      def __init__(self, *args):
        super(Test, self).__init__(*args)
        self.run_count = 0
        self._unit_test_events = []  # track what was called when

      def RunPage(self, *_):
        old_run_count = self.run_count
        self.run_count += 1
        if old_run_count == 0:
          self._unit_test_events.append('app-crash')
          raise exceptions.AppCrashException

      def ValidateAndMeasurePage(self, page, tab, results):
        pass

      def DidRunTest(self, _, __):
        self._unit_test_events.append('did-run-test')
        raise DidRunTestError

    us.AddUserStory(DummyLocalUserStory(TestTearDownSharedUserStoryState))
    us.AddUserStory(DummyLocalUserStory(TestTearDownSharedUserStoryState))
    test = Test()

    with self.assertRaises(DidRunTestError):
      user_story_runner.Run(
          test, us, self.expectations, self.options, self.results)
    self.assertEqual(['app-crash', 'did-run-test'], test._unit_test_events)
    # The AppCrashException gets added as a failure.
    self.assertEquals(1, len(self.results.failures))

  def testDiscardFirstResult(self):
    us = user_story_set.UserStorySet()
    us.AddUserStory(DummyLocalUserStory(TestSharedUserStoryState))
    us.AddUserStory(DummyLocalUserStory(TestSharedUserStoryState))
    class Measurement(page_test.PageTest):
      @property
      def discard_first_result(self):
        return True

      def RunPage(self, page, _, results):
        results.AddValue(string.StringValue(page, 'test', 't', page.name))

      def ValidateAndMeasurePage(self, page, tab, results):
        pass

    results = results_options.CreateResults(
        EmptyMetadataForTest(), self.options)
    user_story_runner.Run(
        Measurement(), us, self.expectations, self.options, results)

    self.assertEquals(0, GetNumberOfSuccessfulPageRuns(results))
    self.assertEquals(0, len(results.failures))
    self.assertEquals(0, len(results.all_page_specific_values))


    results = results_options.CreateResults(
        EmptyMetadataForTest(), self.options)
    self.options.page_repeat = 1
    self.options.pageset_repeat = 2
    user_story_runner.Run(
        Measurement(), us, self.expectations, self.options, results)
    self.assertEquals(2, GetNumberOfSuccessfulPageRuns(results))
    self.assertEquals(0, len(results.failures))
    self.assertEquals(2, len(results.all_page_specific_values))

    results = results_options.CreateResults(
        EmptyMetadataForTest(), self.options)
    self.options.page_repeat = 2
    self.options.pageset_repeat = 1
    user_story_runner.Run(
        Measurement(), us, self.expectations, self.options, results)
    self.assertEquals(2, GetNumberOfSuccessfulPageRuns(results))
    self.assertEquals(0, len(results.failures))
    self.assertEquals(2, len(results.all_page_specific_values))

    results = results_options.CreateResults(
        EmptyMetadataForTest(), self.options)
    self.options.page_repeat = 1
    self.options.pageset_repeat = 1
    user_story_runner.Run(
        Measurement(), us, self.expectations, self.options, results)
    self.assertEquals(0, GetNumberOfSuccessfulPageRuns(results))
    self.assertEquals(0, len(results.failures))
    self.assertEquals(0, len(results.all_page_specific_values))

  def testPagesetRepeat(self):
    us = user_story_set.UserStorySet()
    us.AddUserStory(DummyLocalUserStory(
        TestSharedUserStoryState, name='blank'))
    us.AddUserStory(DummyLocalUserStory(
        TestSharedUserStoryState, name='green'))

    class Measurement(page_test.PageTest):
      i = 0
      def RunPage(self, page, _, results):
        self.i += 1
        results.AddValue(scalar.ScalarValue(
            page, 'metric', 'unit', self.i))

      def ValidateAndMeasurePage(self, page, tab, results):
        pass

    self.options.page_repeat = 1
    self.options.pageset_repeat = 2
    self.options.output_formats = ['buildbot']
    output = StringIO.StringIO()
    real_stdout = sys.stdout
    sys.stdout = output
    try:
      results = results_options.CreateResults(
        EmptyMetadataForTest(), self.options)
      user_story_runner.Run(
          Measurement(), us, self.expectations, self.options, results)
      results.PrintSummary()
      contents = output.getvalue()
      self.assertEquals(4, GetNumberOfSuccessfulPageRuns(results))
      self.assertEquals(0, len(results.failures))
      self.assertIn('RESULT metric: blank= [1,3] unit', contents)
      self.assertIn('RESULT metric: green= [2,4] unit', contents)
      self.assertIn('*RESULT metric: metric= [1,2,3,4] unit', contents)
    finally:
      sys.stdout = real_stdout

  def testCheckArchives(self):
    uss = user_story_set.UserStorySet()
    uss.AddUserStory(page_module.Page(
        'http://www.testurl.com', self, uss.base_dir))
    # Page set missing archive_data_file.
    self.assertFalse(user_story_runner._CheckArchives(
        uss.archive_data_file, uss.wpr_archive_info, uss.user_stories))

    uss = user_story_set.UserStorySet(
        archive_data_file='missing_archive_data_file.json')
    uss.AddUserStory(page_module.Page(
        'http://www.testurl.com', self, uss.base_dir))
    # Page set missing json file specified in archive_data_file.
    self.assertFalse(user_story_runner._CheckArchives(
        uss.archive_data_file, uss.wpr_archive_info, uss.user_stories))

    uss = user_story_set.UserStorySet(
        archive_data_file='../../unittest_data/test.json',
        cloud_storage_bucket=cloud_storage.PUBLIC_BUCKET)
    uss.AddUserStory(page_module.Page(
        'http://www.testurl.com', self, uss.base_dir))
    # Page set with valid archive_data_file.
    self.assertTrue(user_story_runner._CheckArchives(
        uss.archive_data_file, uss.wpr_archive_info, uss.user_stories))
    uss.AddUserStory(page_module.Page(
        'http://www.google.com', self, uss.base_dir))
    # Page set with an archive_data_file which exists but is missing a page.
    self.assertFalse(user_story_runner._CheckArchives(
        uss.archive_data_file, uss.wpr_archive_info, uss.user_stories))

    uss = user_story_set.UserStorySet(
        archive_data_file='../../unittest_data/test_missing_wpr_file.json',
        cloud_storage_bucket=cloud_storage.PUBLIC_BUCKET)
    uss.AddUserStory(page_module.Page(
        'http://www.testurl.com', self, uss.base_dir))
    uss.AddUserStory(page_module.Page(
        'http://www.google.com', self, uss.base_dir))
    # Page set with an archive_data_file which exists and contains all pages
    # but fails to find a wpr file.
    self.assertFalse(user_story_runner._CheckArchives(
        uss.archive_data_file, uss.wpr_archive_info, uss.user_stories))
