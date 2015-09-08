// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/security_state_model.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ssl/ssl_blocking_page.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/cert_store.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/cert/x509_certificate.h"

namespace {

const base::FilePath::CharType kDocRoot[] =
    FILE_PATH_LITERAL("chrome/test/data");

void CheckHTTPSSecurityInfo(
    content::WebContents* contents,
    SecurityStateModel::SecurityLevel security_level,
    SecurityStateModel::SHA1DeprecationStatus sha1_status,
    SecurityStateModel::MixedContentStatus mixed_content_status,
    bool expect_cert_error) {
  ASSERT_TRUE(contents);

  SecurityStateModel* model = SecurityStateModel::FromWebContents(contents);
  ASSERT_TRUE(model);
  const SecurityStateModel::SecurityInfo& security_info =
      model->security_info();
  EXPECT_EQ(security_level, security_info.security_level);
  EXPECT_EQ(sha1_status, security_info.sha1_deprecation_status);
  EXPECT_EQ(mixed_content_status, security_info.mixed_content_status);
  EXPECT_TRUE(security_info.sct_verify_statuses.empty());
  EXPECT_TRUE(security_info.scheme_is_cryptographic);
  EXPECT_EQ(expect_cert_error,
            net::IsCertStatusError(security_info.cert_status));
  EXPECT_GT(security_info.security_bits, 0);

  int cert_id = security_info.cert_id;
  content::CertStore* cert_store = content::CertStore::GetInstance();
  scoped_refptr<net::X509Certificate> cert;
  EXPECT_TRUE(cert_store->RetrieveCert(cert_id, &cert));
}

class SecurityStateModelTest : public CertVerifierBrowserTest {
 public:
  SecurityStateModelTest()
      : https_server_(net::SpawnedTestServer::TYPE_HTTPS,
                      SSLOptions(SSLOptions::CERT_OK),
                      base::FilePath(kDocRoot)) {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Browser will both run and display insecure content.
    command_line->AppendSwitch(switches::kAllowRunningInsecureContent);
  }

  void ProceedThroughInterstitial(content::WebContents* tab) {
    content::InterstitialPage* interstitial_page = tab->GetInterstitialPage();
    ASSERT_TRUE(interstitial_page);
    ASSERT_EQ(SSLBlockingPage::kTypeForTesting,
              interstitial_page->GetDelegateForTesting()->GetTypeForTesting());
    content::WindowedNotificationObserver observer(
        content::NOTIFICATION_LOAD_STOP,
        content::Source<content::NavigationController>(&tab->GetController()));
    interstitial_page->Proceed();
    observer.Wait();
  }

  static bool GetFilePathWithHostAndPortReplacement(
      const std::string& original_file_path,
      const net::HostPortPair& host_port_pair,
      std::string* replacement_path) {
    std::vector<net::SpawnedTestServer::StringPair> replacement_text;
    replacement_text.push_back(
        make_pair("REPLACE_WITH_HOST_AND_PORT", host_port_pair.ToString()));
    return net::SpawnedTestServer::GetFilePathWithReplacements(
        original_file_path, replacement_text, replacement_path);
  }

 protected:
  void SetUpMockCertVerifierForHttpsServer(net::CertStatus cert_status,
                                           int net_result) {
    scoped_refptr<net::X509Certificate> cert(https_server_.GetCertificate());
    net::CertVerifyResult verify_result;
    verify_result.is_issued_by_known_root = true;
    verify_result.verified_cert = cert;
    verify_result.cert_status = cert_status;

    mock_cert_verifier()->AddResultForCert(cert.get(), verify_result,
                                           net_result);
  }

  net::SpawnedTestServer https_server_;

 private:
  typedef net::SpawnedTestServer::SSLOptions SSLOptions;

  DISALLOW_COPY_AND_ASSIGN(SecurityStateModelTest);
};

IN_PROC_BROWSER_TEST_F(SecurityStateModelTest, HttpPage) {
  ASSERT_TRUE(test_server()->Start());
  ui_test_utils::NavigateToURL(browser(),
                               test_server()->GetURL("files/ssl/google.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(contents);

  SecurityStateModel* model = SecurityStateModel::FromWebContents(contents);
  ASSERT_TRUE(model);
  const SecurityStateModel::SecurityInfo& security_info =
      model->security_info();
  EXPECT_EQ(SecurityStateModel::NONE, security_info.security_level);
  EXPECT_EQ(SecurityStateModel::NO_DEPRECATED_SHA1,
            security_info.sha1_deprecation_status);
  EXPECT_EQ(SecurityStateModel::NO_MIXED_CONTENT,
            security_info.mixed_content_status);
  EXPECT_TRUE(security_info.sct_verify_statuses.empty());
  EXPECT_FALSE(security_info.scheme_is_cryptographic);
  EXPECT_FALSE(net::IsCertStatusError(security_info.cert_status));
  EXPECT_EQ(0, security_info.cert_id);
  EXPECT_EQ(-1, security_info.security_bits);
  EXPECT_EQ(0, security_info.connection_status);
}

IN_PROC_BROWSER_TEST_F(SecurityStateModelTest, HttpsPage) {
  ASSERT_TRUE(https_server_.Start());
  SetUpMockCertVerifierForHttpsServer(0, net::OK);

  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL("files/ssl/google.html"));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURE,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::NO_MIXED_CONTENT,
                         false /* expect cert status error */);
}

IN_PROC_BROWSER_TEST_F(SecurityStateModelTest, SHA1Broken) {
  ASSERT_TRUE(https_server_.Start());
  // The test server uses a long-lived cert by default, so a SHA1
  // signature in it will register as a "broken" condition rather than
  // "warning".
  SetUpMockCertVerifierForHttpsServer(net::CERT_STATUS_SHA1_SIGNATURE_PRESENT,
                                      net::OK);

  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL("files/ssl/google.html"));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::DEPRECATED_SHA1_BROKEN,
                         SecurityStateModel::NO_MIXED_CONTENT,
                         false /* expect cert status error */);
}

IN_PROC_BROWSER_TEST_F(SecurityStateModelTest, MixedContent) {
  ASSERT_TRUE(test_server()->Start());
  ASSERT_TRUE(https_server_.Start());
  SetUpMockCertVerifierForHttpsServer(0, net::OK);

  // Navigate to an HTTPS page that displays mixed content.
  std::string replacement_path;
  ASSERT_TRUE(GetFilePathWithHostAndPortReplacement(
      "files/ssl/page_displays_insecure_content.html",
      test_server()->host_port_pair(), &replacement_path));
  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL(replacement_path));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::NONE,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::DISPLAYED_MIXED_CONTENT,
                         false /* expect cert status error */);

  // Navigate to an HTTPS page that displays mixed content dynamically.
  ASSERT_TRUE(GetFilePathWithHostAndPortReplacement(
      "files/ssl/page_with_dynamic_insecure_content.html",
      test_server()->host_port_pair(), &replacement_path));
  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL(replacement_path));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURE,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::NO_MIXED_CONTENT,
                         false /* expect cert status error */);
  // Load the insecure image.
  bool js_result = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(), "loadBadImage();",
      &js_result));
  EXPECT_TRUE(js_result);
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::NONE,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::DISPLAYED_MIXED_CONTENT,
                         false /* expect cert status error */);

  // Navigate to an HTTPS page that runs mixed content.
  ASSERT_TRUE(GetFilePathWithHostAndPortReplacement(
      "files/ssl/page_runs_insecure_content.html",
      test_server()->host_port_pair(), &replacement_path));
  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL(replacement_path));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::RAN_AND_DISPLAYED_MIXED_CONTENT,
                         false /* expect cert status error */);
}

// Same as the test above but with a long-lived SHA1 cert.
IN_PROC_BROWSER_TEST_F(SecurityStateModelTest, MixedContentWithBrokenSHA1) {
  ASSERT_TRUE(test_server()->Start());
  ASSERT_TRUE(https_server_.Start());
  // The test server uses a long-lived cert by default, so a SHA1
  // signature in it will register as a "broken" condition rather than
  // "warning".
  SetUpMockCertVerifierForHttpsServer(net::CERT_STATUS_SHA1_SIGNATURE_PRESENT,
                                      net::OK);

  // Navigate to an HTTPS page that displays mixed content.
  std::string replacement_path;
  ASSERT_TRUE(GetFilePathWithHostAndPortReplacement(
      "files/ssl/page_displays_insecure_content.html",
      test_server()->host_port_pair(), &replacement_path));
  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL(replacement_path));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::DEPRECATED_SHA1_BROKEN,
                         SecurityStateModel::DISPLAYED_MIXED_CONTENT,
                         false /* expect cert status error */);

  // Navigate to an HTTPS page that displays mixed content dynamically.
  ASSERT_TRUE(GetFilePathWithHostAndPortReplacement(
      "files/ssl/page_with_dynamic_insecure_content.html",
      test_server()->host_port_pair(), &replacement_path));
  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL(replacement_path));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::DEPRECATED_SHA1_BROKEN,
                         SecurityStateModel::NO_MIXED_CONTENT,
                         false /* expect cert status error */);
  // Load the insecure image.
  bool js_result = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(), "loadBadImage();",
      &js_result));
  EXPECT_TRUE(js_result);
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::DEPRECATED_SHA1_BROKEN,
                         SecurityStateModel::DISPLAYED_MIXED_CONTENT,
                         false /* expect cert status error */);

  // Navigate to an HTTPS page that runs mixed content.
  ASSERT_TRUE(GetFilePathWithHostAndPortReplacement(
      "files/ssl/page_runs_insecure_content.html",
      test_server()->host_port_pair(), &replacement_path));
  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL(replacement_path));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::DEPRECATED_SHA1_BROKEN,
                         SecurityStateModel::RAN_AND_DISPLAYED_MIXED_CONTENT,
                         false /* expect cert status error */);
}

IN_PROC_BROWSER_TEST_F(SecurityStateModelTest, BrokenHTTPS) {
  ASSERT_TRUE(test_server()->Start());
  ASSERT_TRUE(https_server_.Start());
  SetUpMockCertVerifierForHttpsServer(net::CERT_STATUS_DATE_INVALID,
                                      net::ERR_CERT_DATE_INVALID);

  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL("files/ssl/google.html"));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::NO_MIXED_CONTENT,
                         true /* expect cert status error */);

  ProceedThroughInterstitial(
      browser()->tab_strip_model()->GetActiveWebContents());

  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::NO_MIXED_CONTENT,
                         true /* expect cert status error */);

  // Navigate to a broken HTTPS page that displays mixed content.
  std::string replacement_path;
  ASSERT_TRUE(GetFilePathWithHostAndPortReplacement(
      "files/ssl/page_displays_insecure_content.html",
      test_server()->host_port_pair(), &replacement_path));
  ui_test_utils::NavigateToURL(browser(),
                               https_server_.GetURL(replacement_path));
  CheckHTTPSSecurityInfo(browser()->tab_strip_model()->GetActiveWebContents(),
                         SecurityStateModel::SECURITY_ERROR,
                         SecurityStateModel::NO_DEPRECATED_SHA1,
                         SecurityStateModel::DISPLAYED_MIXED_CONTENT,
                         true /* expect cert status error */);
}

// TODO(estark): test the following cases:
// - warning SHA1 (2016 expiration)
// - active mixed content + warning SHA1
// - broken HTTPS + warning SHA1

}  // namespace
