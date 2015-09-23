// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/ninja_action_target_writer.h"
#include "tools/gn/substitution_list.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"

TEST(NinjaActionTargetWriter, WriteOutputFilesForBuildLine) {
  TestWithScope setup;
  Err err;

  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION_FOREACH);
  target.action_values().outputs() = SubstitutionList::MakeForTest(
      "//out/Debug/gen/a b{{source_name_part}}.h",
      "//out/Debug/gen/{{source_name_part}}.cc");

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaActionTargetWriter writer(&target, out);

  SourceFile source("//foo/bar.in");
  std::vector<OutputFile> output_files;
  writer.WriteOutputFilesForBuildLine(source, &output_files);

  EXPECT_EQ(" gen/a$ bbar.h gen/bar.cc", out.str());
}

// Tests an action with no sources.
TEST(NinjaActionTargetWriter, ActionNoSources) {
  TestWithScope setup;
  Err err;

  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));
  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION);

  target.action_values().set_script(SourceFile("//foo/script.py"));
  target.inputs().push_back(SourceFile("//foo/included.txt"));

  target.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/foo.out");

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  setup.settings()->set_target_os(Settings::LINUX);
  setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
      "/usr/bin/python")));

  std::ostringstream out;
  NinjaActionTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "rule __foo_bar___rule\n"
      "  command = /usr/bin/python ../../foo/script.py\n"
      "  description = ACTION //foo:bar()\n"
      "  restat = 1\n"
      "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
          "../../foo/included.txt\n"
      "\n"
      "build foo.out: __foo_bar___rule | obj/foo/bar.inputdeps.stamp\n"
      "\n"
      "build obj/foo/bar.stamp: stamp foo.out\n";
  EXPECT_EQ(expected, out.str());
}


// Tests an action with no sources and console = true
TEST(NinjaActionTargetWriter, ActionNoSourcesConsole) {
  TestWithScope setup;
  Err err;

  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));
  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION);

  target.action_values().set_script(SourceFile("//foo/script.py"));
  target.inputs().push_back(SourceFile("//foo/included.txt"));

  target.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/foo.out");
  target.action_values().set_console(true);

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  setup.settings()->set_target_os(Settings::LINUX);
  setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
      "/usr/bin/python")));

  std::ostringstream out;
  NinjaActionTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "rule __foo_bar___rule\n"
      "  command = /usr/bin/python ../../foo/script.py\n"
      "  description = ACTION //foo:bar()\n"
      "  restat = 1\n"
      "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
          "../../foo/included.txt\n"
      "\n"
      "build foo.out: __foo_bar___rule | obj/foo/bar.inputdeps.stamp\n"
      "  pool = console\n"
      "\n"
      "build obj/foo/bar.stamp: stamp foo.out\n";
  EXPECT_EQ(expected, out.str());
}

// Makes sure that we write sources as input dependencies for actions with
// both sources and inputs (ACTION_FOREACH treats the sources differently).
TEST(NinjaActionTargetWriter, ActionWithSources) {
  TestWithScope setup;
  Err err;

  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));
  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION);

  target.action_values().set_script(SourceFile("//foo/script.py"));

  target.sources().push_back(SourceFile("//foo/source.txt"));
  target.inputs().push_back(SourceFile("//foo/included.txt"));

  target.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/foo.out");

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // Posix.
  {
    setup.settings()->set_target_os(Settings::LINUX);
    setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
        "/usr/bin/python")));

    std::ostringstream out;
    NinjaActionTargetWriter writer(&target, out);
    writer.Run();

    const char expected_linux[] =
        "rule __foo_bar___rule\n"
        "  command = /usr/bin/python ../../foo/script.py\n"
        "  description = ACTION //foo:bar()\n"
        "  restat = 1\n"
        "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
            "../../foo/included.txt ../../foo/source.txt\n"
        "\n"
        "build foo.out: __foo_bar___rule | obj/foo/bar.inputdeps.stamp\n"
        "\n"
        "build obj/foo/bar.stamp: stamp foo.out\n";
    EXPECT_EQ(expected_linux, out.str());
  }

  // Windows.
  {
    // Note: we use forward slashes here so that the output will be the same on
    // Linux and Windows.
    setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
        "C:/python/python.exe")));
    setup.settings()->set_target_os(Settings::WIN);

    std::ostringstream out;
    NinjaActionTargetWriter writer(&target, out);
    writer.Run();

    const char expected_win[] =
        "rule __foo_bar___rule\n"
        "  command = C$:/python/python.exe gyp-win-tool action-wrapper environment.x86 __foo_bar___rule.$unique_name.rsp\n"
        "  description = ACTION //foo:bar()\n"
        "  restat = 1\n"
        "  rspfile = __foo_bar___rule.$unique_name.rsp\n"
        "  rspfile_content = C$:/python/python.exe ../../foo/script.py\n"
        "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
            "../../foo/included.txt ../../foo/source.txt\n"
        "\n"
        "build foo.out: __foo_bar___rule | obj/foo/bar.inputdeps.stamp\n"
        "\n"
        "build obj/foo/bar.stamp: stamp foo.out\n";
    EXPECT_EQ(expected_win, out.str());
  }
}

TEST(NinjaActionTargetWriter, ForEach) {
  TestWithScope setup;
  Err err;

  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));

  // Some dependencies that the action can depend on. Use actions for these
  // so they have a nice platform-independent stamp file that can appear in the
  // output (rather than having to worry about how the current platform names
  // binaries).
  Target dep(setup.settings(), Label(SourceDir("//foo/"), "dep"));
  dep.set_output_type(Target::ACTION);
  dep.visibility().SetPublic();
  dep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep.OnResolved(&err));

  Target datadep(setup.settings(), Label(SourceDir("//foo/"), "datadep"));
  datadep.set_output_type(Target::ACTION);
  datadep.visibility().SetPublic();
  datadep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(datadep.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION_FOREACH);
  target.private_deps().push_back(LabelTargetPair(&dep));
  target.data_deps().push_back(LabelTargetPair(&datadep));

  target.sources().push_back(SourceFile("//foo/input1.txt"));
  target.sources().push_back(SourceFile("//foo/input2.txt"));

  target.action_values().set_script(SourceFile("//foo/script.py"));

  target.action_values().args() = SubstitutionList::MakeForTest(
      "-i",
      "{{source}}",
      "--out=foo bar{{source_name_part}}.o");
  target.action_values().outputs() = SubstitutionList::MakeForTest(
      "//out/Debug/{{source_name_part}}.out");

  target.inputs().push_back(SourceFile("//foo/included.txt"));

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // Posix.
  {
    setup.settings()->set_target_os(Settings::LINUX);
    setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
        "/usr/bin/python")));

    std::ostringstream out;
    NinjaActionTargetWriter writer(&target, out);
    writer.Run();

    const char expected_linux[] =
        "rule __foo_bar___rule\n"
        "  command = /usr/bin/python ../../foo/script.py -i ${in} "
            // Escaping is different between Windows and Posix.
#if defined(OS_WIN)
            "\"--out=foo$ bar${source_name_part}.o\"\n"
#else
            "--out=foo\\$ bar${source_name_part}.o\n"
#endif
        "  description = ACTION //foo:bar()\n"
        "  restat = 1\n"
        "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
            "../../foo/included.txt obj/foo/dep.stamp\n"
        "\n"
        "build input1.out: __foo_bar___rule ../../foo/input1.txt | "
            "obj/foo/bar.inputdeps.stamp\n"
        "  source_name_part = input1\n"
        "build input2.out: __foo_bar___rule ../../foo/input2.txt | "
            "obj/foo/bar.inputdeps.stamp\n"
        "  source_name_part = input2\n"
        "\n"
        "build obj/foo/bar.stamp: "
            "stamp input1.out input2.out || obj/foo/datadep.stamp\n";

    std::string out_str = out.str();
#if defined(OS_WIN)
    std::replace(out_str.begin(), out_str.end(), '\\', '/');
#endif
    EXPECT_EQ(expected_linux, out_str);
  }

  // Windows.
  {
    setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
        "C:/python/python.exe")));
    setup.settings()->set_target_os(Settings::WIN);

    std::ostringstream out;
    NinjaActionTargetWriter writer(&target, out);
    writer.Run();

    const char expected_win[] =
        "rule __foo_bar___rule\n"
        "  command = C$:/python/python.exe gyp-win-tool action-wrapper "
            "environment.x86 __foo_bar___rule.$unique_name.rsp\n"
        "  description = ACTION //foo:bar()\n"
        "  restat = 1\n"
        "  rspfile = __foo_bar___rule.$unique_name.rsp\n"
        "  rspfile_content = C$:/python/python.exe ../../foo/script.py -i "
#if defined(OS_WIN)
            "${in} \"--out=foo$ bar${source_name_part}.o\"\n"
#else
            "${in} --out=foo\\$ bar${source_name_part}.o\n"
#endif
        "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
            "../../foo/included.txt obj/foo/dep.stamp\n"
        "\n"
        "build input1.out: __foo_bar___rule ../../foo/input1.txt | "
            "obj/foo/bar.inputdeps.stamp\n"
        "  unique_name = 0\n"
        "  source_name_part = input1\n"
        "build input2.out: __foo_bar___rule ../../foo/input2.txt | "
            "obj/foo/bar.inputdeps.stamp\n"
        "  unique_name = 1\n"
        "  source_name_part = input2\n"
        "\n"
        "build obj/foo/bar.stamp: "
            "stamp input1.out input2.out || obj/foo/datadep.stamp\n";
    EXPECT_EQ(expected_win, out.str());
  }
}

TEST(NinjaActionTargetWriter, ForEachWithDepfile) {
  TestWithScope setup;
  Err err;

  setup.build_settings()->SetBuildDir(SourceDir("//out/Debug/"));
  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::ACTION_FOREACH);

  target.sources().push_back(SourceFile("//foo/input1.txt"));
  target.sources().push_back(SourceFile("//foo/input2.txt"));

  target.action_values().set_script(SourceFile("//foo/script.py"));

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  SubstitutionPattern depfile;
  ASSERT_TRUE(
      depfile.Parse("//out/Debug/gen/{{source_name_part}}.d", nullptr, &err));
  target.action_values().set_depfile(depfile);

  target.action_values().args() = SubstitutionList::MakeForTest(
      "-i",
      "{{source}}",
      "--out=foo bar{{source_name_part}}.o");
  target.action_values().outputs() = SubstitutionList::MakeForTest(
      "//out/Debug/{{source_name_part}}.out");

  target.inputs().push_back(SourceFile("//foo/included.txt"));

  // Posix.
  {
    setup.settings()->set_target_os(Settings::LINUX);
    setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
        "/usr/bin/python")));

    std::ostringstream out;
    NinjaActionTargetWriter writer(&target, out);
    writer.Run();

    const char expected_linux[] =
        "rule __foo_bar___rule\n"
        "  command = /usr/bin/python ../../foo/script.py -i ${in} "
#if defined(OS_WIN)
            "\"--out=foo$ bar${source_name_part}.o\"\n"
#else
            "--out=foo\\$ bar${source_name_part}.o\n"
#endif
        "  description = ACTION //foo:bar()\n"
        "  restat = 1\n"
        "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
            "../../foo/included.txt\n"
        "\n"
        "build input1.out: __foo_bar___rule ../../foo/input1.txt"
            " | obj/foo/bar.inputdeps.stamp\n"
        "  source_name_part = input1\n"
        "  depfile = gen/input1.d\n"
        "build input2.out: __foo_bar___rule ../../foo/input2.txt"
            " | obj/foo/bar.inputdeps.stamp\n"
        "  source_name_part = input2\n"
        "  depfile = gen/input2.d\n"
        "\n"
        "build obj/foo/bar.stamp: stamp input1.out input2.out\n";
    EXPECT_EQ(expected_linux, out.str());
  }

  // Windows.
  {
    setup.build_settings()->set_python_path(base::FilePath(FILE_PATH_LITERAL(
        "C:/python/python.exe")));
    setup.settings()->set_target_os(Settings::WIN);

    std::ostringstream out;
    NinjaActionTargetWriter writer(&target, out);
    writer.Run();

    const char expected_win[] =
        "rule __foo_bar___rule\n"
        "  command = C$:/python/python.exe gyp-win-tool action-wrapper "
            "environment.x86 __foo_bar___rule.$unique_name.rsp\n"
        "  description = ACTION //foo:bar()\n"
        "  restat = 1\n"
        "  rspfile = __foo_bar___rule.$unique_name.rsp\n"
        "  rspfile_content = C$:/python/python.exe ../../foo/script.py -i "
#if defined(OS_WIN)
            "${in} \"--out=foo$ bar${source_name_part}.o\"\n"
#else
            "${in} --out=foo\\$ bar${source_name_part}.o\n"
#endif
        "build obj/foo/bar.inputdeps.stamp: stamp ../../foo/script.py "
            "../../foo/included.txt\n"
        "\n"
        "build input1.out: __foo_bar___rule ../../foo/input1.txt"
            " | obj/foo/bar.inputdeps.stamp\n"
        "  unique_name = 0\n"
        "  source_name_part = input1\n"
        "  depfile = gen/input1.d\n"
        "build input2.out: __foo_bar___rule ../../foo/input2.txt"
            " | obj/foo/bar.inputdeps.stamp\n"
        "  unique_name = 1\n"
        "  source_name_part = input2\n"
        "  depfile = gen/input2.d\n"
        "\n"
        "build obj/foo/bar.stamp: stamp input1.out input2.out\n";
    EXPECT_EQ(expected_win, out.str());
  }
}
