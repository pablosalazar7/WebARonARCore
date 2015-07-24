// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/secure_display/elide_url.h"

#include "base/ios/ios_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/text_utils.h"
#include "url/gurl.h"

using base::UTF8ToUTF16;
using gfx::GetStringWidthF;
using gfx::kEllipsis;

namespace {

struct Testcase {
  const std::string input;
  const std::string output;
};

#if !defined(OS_ANDROID)
void RunUrlTest(Testcase* testcases, size_t num_testcases) {
  static const gfx::FontList font_list;
  for (size_t i = 0; i < num_testcases; ++i) {
    const GURL url(testcases[i].input);
    // Should we test with non-empty language list?
    // That's kinda redundant with net_util_unittests.
    const float available_width =
        GetStringWidthF(UTF8ToUTF16(testcases[i].output), font_list);
    EXPECT_EQ(UTF8ToUTF16(testcases[i].output),
              secure_display::ElideUrl(url, font_list, available_width,
                                       std::string()));
  }
}

// Test eliding of commonplace URLs.
TEST(TextEliderTest, TestGeneralEliding) {
  const std::string kEllipsisStr(kEllipsis);
  Testcase testcases[] = {
      {"http://www.google.com/intl/en/ads/", "www.google.com/intl/en/ads/"},
      {"http://www.google.com/intl/en/ads/", "www.google.com/intl/en/ads/"},
      {"http://www.google.com/intl/en/ads/",
       "google.com/intl/" + kEllipsisStr + "/ads/"},
      {"http://www.google.com/intl/en/ads/",
       "google.com/" + kEllipsisStr + "/ads/"},
      {"http://www.google.com/intl/en/ads/", "google.com/" + kEllipsisStr},
      {"http://www.google.com/intl/en/ads/", "goog" + kEllipsisStr},
      {"https://subdomain.foo.com/bar/filename.html",
       "subdomain.foo.com/bar/filename.html"},
      {"https://subdomain.foo.com/bar/filename.html",
       "subdomain.foo.com/" + kEllipsisStr + "/filename.html"},
      {"http://subdomain.foo.com/bar/filename.html",
       kEllipsisStr + "foo.com/" + kEllipsisStr + "/filename.html"},
      {"http://www.google.com/intl/en/ads/?aLongQueryWhichIsNotRequired",
       "www.google.com/intl/en/ads/?aLongQ" + kEllipsisStr},
  };

  RunUrlTest(testcases, arraysize(testcases));
}

// When there is very little space available, the elision code will shorten
// both path AND file name to an ellipsis - ".../...". To avoid this result,
// there is a hack in place that simply treats them as one string in this
// case.
TEST(TextEliderTest, TestTrailingEllipsisSlashEllipsisHack) {
  const std::string kEllipsisStr(kEllipsis);

  // Very little space, would cause double ellipsis.
  gfx::FontList font_list;
  GURL url("http://battersbox.com/directory/foo/peter_paul_and_mary.html");
  float available_width = GetStringWidthF(
      UTF8ToUTF16("battersbox.com/" + kEllipsisStr + "/" + kEllipsisStr),
      font_list);

  // Create the expected string, after elision. Depending on font size, the
  // directory might become /dir... or /di... or/d... - it never should be
  // shorter than that. (If it is, the font considers d... to be longer
  // than .../... -  that should never happen).
  ASSERT_GT(GetStringWidthF(UTF8ToUTF16(kEllipsisStr + "/" + kEllipsisStr),
                            font_list),
            GetStringWidthF(UTF8ToUTF16("d" + kEllipsisStr), font_list));
  GURL long_url("http://battersbox.com/directorynameisreallylongtoforcetrunc");
  base::string16 expected = secure_display::ElideUrl(
      long_url, font_list, available_width, std::string());
  // Ensure that the expected result still contains part of the directory name.
  ASSERT_GT(expected.length(), std::string("battersbox.com/d").length());
  EXPECT_EQ(expected, secure_display::ElideUrl(url, font_list, available_width,
                                               std::string()));

  // More space available - elide directories, partially elide filename.
  Testcase testcases[] = {
      {"http://battersbox.com/directory/foo/peter_paul_and_mary.html",
       "battersbox.com/" + kEllipsisStr + "/peter" + kEllipsisStr},
  };
  RunUrlTest(testcases, arraysize(testcases));
}

// Test eliding of empty strings, URLs with ports, passwords, queries, etc.
TEST(TextEliderTest, TestMoreEliding) {
  const std::string kEllipsisStr(kEllipsis);
  Testcase testcases[] = {
      {"http://www.google.com/foo?bar", "www.google.com/foo?bar"},
      {"http://xyz.google.com/foo?bar", "xyz.google.com/foo?" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar", "xyz.google.com/foo" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar", "xyz.google.com/fo" + kEllipsisStr},
      {"http://a.b.com/pathname/c?d", "a.b.com/" + kEllipsisStr + "/c?d"},
      {"", ""},
      {"http://foo.bar..example.com...hello/test/filename.html",
       "foo.bar..example.com...hello/" + kEllipsisStr + "/filename.html"},
      {"http://foo.bar../", "foo.bar.."},
      {"http://xn--1lq90i.cn/foo", "\xe5\x8c\x97\xe4\xba\xac.cn/foo"},
      {"http://me:mypass@secrethost.com:99/foo?bar#baz",
       "secrethost.com:99/foo?bar#baz"},
      {"http://me:mypass@ss%xxfdsf.com/foo", "ss%25xxfdsf.com/foo"},
      {"mailto:elgoato@elgoato.com", "mailto:elgoato@elgoato.com"},
      {"javascript:click(0)", "javascript:click(0)"},
      {"https://chess.eecs.berkeley.edu:4430/login/arbitfilename",
       "chess.eecs.berkeley.edu:4430/login/arbitfilename"},
      {"https://chess.eecs.berkeley.edu:4430/login/arbitfilename",
       kEllipsisStr + "berkeley.edu:4430/" + kEllipsisStr + "/arbitfilename"},

      // Unescaping.
      {"http://www/%E4%BD%A0%E5%A5%BD?q=%E4%BD%A0%E5%A5%BD#\xe4\xbd\xa0",
       "www/\xe4\xbd\xa0\xe5\xa5\xbd?q=\xe4\xbd\xa0\xe5\xa5\xbd#\xe4\xbd\xa0"},

      // Invalid unescaping for path. The ref will always be valid UTF-8. We
      // don't
      // bother to do too many edge cases, since these are handled by the
      // escaper
      // unittest.
      {"http://www/%E4%A0%E5%A5%BD?q=%E4%BD%A0%E5%A5%BD#\xe4\xbd\xa0",
       "www/%E4%A0%E5%A5%BD?q=\xe4\xbd\xa0\xe5\xa5\xbd#\xe4\xbd\xa0"},
  };

  RunUrlTest(testcases, arraysize(testcases));
}

// Test eliding of file: URLs.
TEST(TextEliderTest, TestFileURLEliding) {
  const std::string kEllipsisStr(kEllipsis);
  Testcase testcases[] = {
    {"file:///C:/path1/path2/path3/filename",
     "file:///C:/path1/path2/path3/filename"},
    {"file:///C:/path1/path2/path3/filename", "C:/path1/path2/path3/filename"},
// GURL parses "file:///C:path" differently on windows than it does on posix.
#if defined(OS_WIN)
    {"file:///C:path1/path2/path3/filename",
     "C:/path1/path2/" + kEllipsisStr + "/filename"},
    {"file:///C:path1/path2/path3/filename",
     "C:/path1/" + kEllipsisStr + "/filename"},
    {"file:///C:path1/path2/path3/filename",
     "C:/" + kEllipsisStr + "/filename"},
#endif  // defined(OS_WIN)
    {"file://filer/foo/bar/file", "filer/foo/bar/file"},
    {"file://filer/foo/bar/file", "filer/foo/" + kEllipsisStr + "/file"},
    {"file://filer/foo/bar/file", "filer/" + kEllipsisStr + "/file"},
    {"file://filer/foo/", "file://filer/foo/"},
    {"file://filer/foo/", "filer/foo/"},
    {"file://filer/foo/", "filer" + kEllipsisStr},
    // Eliding file URLs with nothing after the ':' shouldn't crash.
    {"file:///aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:", "aaa" + kEllipsisStr},
    {"file:///aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:/", "aaa" + kEllipsisStr},
  };

  RunUrlTest(testcases, arraysize(testcases));
}

TEST(TextEliderTest, TestHostEliding) {
#if defined(OS_IOS)
  // TODO(eugenebut): Disable test on iOS9 crbug.com/513703
  if (base::ios::IsRunningOnIOS9OrLater()) {
    LOG(WARNING) << "Test disabled on iOS9.";
    return;
  }
#endif
  const std::string kEllipsisStr(kEllipsis);
  Testcase testcases[] = {
    {"http://google.com", "google.com"},
    {"http://subdomain.google.com", kEllipsisStr + ".google.com"},
    {"http://reallyreallyreallylongdomainname.com",
     "reallyreallyreallylongdomainname.com"},
    {"http://a.b.c.d.e.f.com", kEllipsisStr + "f.com"},
    {"http://foo", "foo"},
    {"http://foo.bar", "foo.bar"},
    {"http://subdomain.foo.bar", kEllipsisStr + "in.foo.bar"},
// IOS width calculations are off by a letter from other platforms for
// some strings from other platforms, probably for strings with too
// many kerned letters on the default font set.
#if !defined(OS_IOS)
    {"http://subdomain.reallylongdomainname.com",
     kEllipsisStr + "ain.reallylongdomainname.com"},
    {"http://a.b.c.d.e.f.com", kEllipsisStr + ".e.f.com"},
#endif  // !defined(OS_IOS)
  };

  for (size_t i = 0; i < arraysize(testcases); ++i) {
    const float available_width =
        GetStringWidthF(UTF8ToUTF16(testcases[i].output), gfx::FontList());
    EXPECT_EQ(UTF8ToUTF16(testcases[i].output),
              secure_display::ElideHost(GURL(testcases[i].input),
                                        gfx::FontList(), available_width));
  }

  // Trying to elide to a really short length will still keep the full TLD+1
  EXPECT_EQ(
      base::ASCIIToUTF16("google.com"),
      secure_display::ElideHost(GURL("http://google.com"), gfx::FontList(), 2));
  EXPECT_EQ(base::UTF8ToUTF16(kEllipsisStr + ".google.com"),
            secure_display::ElideHost(GURL("http://subdomain.google.com"),
                                      gfx::FontList(), 2));
  EXPECT_EQ(
      base::ASCIIToUTF16("foo.bar"),
      secure_display::ElideHost(GURL("http://foo.bar"), gfx::FontList(), 2));
}

#endif  // !defined(OS_ANDROID)

TEST(TextEliderTest, FormatUrlForSecurityDisplay) {
  struct OriginTestData {
    const char* const description;
    const char* const input;
    const wchar_t* const output;
  };

  const OriginTestData tests[] = {
      {"Empty URL", "", L""},
      {"HTTP URL", "http://www.google.com/", L"http://www.google.com"},
      {"HTTPS URL", "https://www.google.com/", L"https://www.google.com"},
      {"Standard HTTP port", "http://www.google.com:80/",
       L"http://www.google.com"},
      {"Standard HTTPS port", "https://www.google.com:443/",
       L"https://www.google.com"},
      {"Standard HTTP port, IDN Chinese",
       "http://\xe4\xb8\xad\xe5\x9b\xbd.icom.museum:80",
       L"http://xn--fiqs8s.icom.museum"},
      {"HTTP URL, IDN Hebrew (RTL)",
       "http://"
       "\xd7\x90\xd7\x99\xd7\xa7\xd7\x95\xd7\xb4\xd7\x9d."
       "\xd7\x99\xd7\xa9\xd7\xa8\xd7\x90\xd7\x9c.museum/",
       L"http://xn--4dbklr2c8d.xn--4dbrk0ce.museum"},
      {"HTTP URL with query string, IDN Arabic (RTL)",
       "http://\xd9\x85\xd8\xb5\xd8\xb1.icom.museum/foo.html?yes=no",
       L"http://xn--wgbh1c.icom.museum"},
      {"Non-standard HTTP port", "http://www.google.com:9000/",
       L"http://www.google.com:9000"},
      {"Non-standard HTTPS port", "https://www.google.com:9000/",
       L"https://www.google.com:9000"},
      {"File URI", "file:///usr/example/file.html",
       L"file:///usr/example/file.html"},
      {"File URI with hostname", "file://localhost/usr/example/file.html",
       L"file:///usr/example/file.html"},
      {"UNC File URI 1", "file:///CONTOSO/accounting/money.xls",
       L"file:///CONTOSO/accounting/money.xls"},
      {"UNC File URI 2",
       "file:///C:/Program%20Files/Music/Web%20Sys/main.html?REQUEST=RADIO",
       L"file:///C:/Program%20Files/Music/Web%20Sys/main.html"},
      {"HTTP URL with path", "http://www.google.com/test.html",
       L"http://www.google.com"},
      {"HTTPS URL with path", "https://www.google.com/test.html",
       L"https://www.google.com"},
      {"Unusual secure scheme (wss)", "wss://www.google.com/",
       L"wss://www.google.com"},
      {"Unusual non-secure scheme (gopher)", "gopher://www.google.com/",
       L"gopher://www.google.com"},
      {"Unlisted scheme (chrome)", "chrome://version", L"chrome://version"},
      {"HTTP IP address", "http://173.194.65.103", L"http://173.194.65.103"},
      {"HTTPS IP address", "https://173.194.65.103", L"https://173.194.65.103"},
      {"HTTP IPv6 address", "http://[FE80:0000:0000:0000:0202:B3FF:FE1E:8329]/",
       L"http://[fe80::202:b3ff:fe1e:8329]"},
      {"HTTPS IPv6 address with port", "https://[2001:db8:0:1]:443/",
       L"https://[2001:db8:0:1]"},
      {"HTTPS IP address, non-default port", "https://173.194.65.103:8443",
       L"https://173.194.65.103:8443"},
      {"HTTP filesystem: URL with path",
       "filesystem:http://www.google.com/temporary/test.html",
       L"filesystem:http://www.google.com"},
      {"File filesystem: URL with path",
       "filesystem:file://localhost/temporary/stuff/test.html?z=fun&goat=billy",
       L"filesystem:file:///temporary/stuff/test.html"},
      {"Invalid scheme 1", "twelve://www.cyber.org/wow.php",
       L"twelve://www.cyber.org/wow.php"},
      {"Invalid scheme 2", "://www.cyber.org/wow.php",
       L"://www.cyber.org/wow.php"},
      {"Invalid host 1", "https://www.cyber../wow.php", L"https://www.cyber.."},
      {"Invalid host 2", "https://www...cyber/wow.php", L"https://www...cyber"},
      {"Invalid port 1", "https://173.194.65.103:000",
       L"https://173.194.65.103:0"},
      {"Invalid port 2", "https://173.194.65.103:gruffle",
       L"https://173.194.65.103:gruffle"},
      {"Invalid port 3", "https://173.194.65.103:/hello.aspx",
       L"https://173.194.65.103"},
      {"Trailing dot in DNS name", "https://www.example.com./get/goat",
       L"https://www.example.com."},
      {"Blob URL",
       "blob:http%3A//www.html5rocks.com/4d4ff040-6d61-4446-86d3-13ca07ec9ab9",
       L"blob:http%3A//www.html5rocks.com/"
       L"4d4ff040-6d61-4446-86d3-13ca07ec9ab9"},
  };

  const char languages[] = "zh-TW,en-US,en,am,ar-EG,ar";
  for (size_t i = 0; i < arraysize(tests); ++i) {
    base::string16 formatted = secure_display::FormatUrlForSecurityDisplay(
        GURL(tests[i].input), std::string());
    EXPECT_EQ(base::WideToUTF16(tests[i].output), formatted)
        << tests[i].description;
    base::string16 formatted_with_languages =
        secure_display::FormatUrlForSecurityDisplay(GURL(tests[i].input),
                                                    languages);
    EXPECT_EQ(base::WideToUTF16(tests[i].output), formatted_with_languages)
        << tests[i].description;
  }

  base::string16 formatted =
      secure_display::FormatUrlForSecurityDisplay(GURL(), std::string());
  EXPECT_EQ(base::string16(), formatted)
      << "Explicitly test the 0-argument GURL constructor";
}

}  // namespace
