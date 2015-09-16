// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/platform_keys/verify_trust_api.h"

#include <algorithm>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/stl_util.h"
#include "chrome/browser/extensions/api/platform_keys/platform_keys_api.h"
#include "chrome/common/extensions/api/platform_keys_internal.h"
#include "extensions/browser/extension_registry_factory.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/x509_certificate.h"
#include "net/log/net_log.h"
#include "net/ssl/ssl_config_service.h"

namespace extensions {

namespace {

base::LazyInstance<BrowserContextKeyedAPIFactory<VerifyTrustAPI>>::Leaky
    g_factory = LAZY_INSTANCE_INITIALIZER;

const char kErrorEmptyCertificateChain[] =
    "Server certificate chain must not be empty.";

}  // namespace

// This class bundles IO data and functions of the VerifyTrustAPI that are to be
// used on the IO thread only.
// It is created on the UI thread and afterwards lives on the IO thread.
class VerifyTrustAPI::IOPart {
 public:
  ~IOPart();

  // Verifies the certificate as stated by |params| and calls back |callback|
  // with the result (see the declaration of VerifyCallback).
  // Will not call back after this object is destructed or the verifier for this
  // extension is deleted (see OnExtensionUnloaded).
  void Verify(scoped_ptr<Params> params,
              const std::string& extension_id,
              const VerifyCallback& callback);

  // Must be called when the extension with id |extension_id| is unloaded.
  // Deletes the verifier for |extension_id| and cancels all pending
  // verifications of this verifier.
  void OnExtensionUnloaded(const std::string& extension_id);

 private:
  struct RequestState {
    RequestState() {}

    scoped_ptr<net::CertVerifier::Request> request;

   private:
    DISALLOW_COPY_AND_ASSIGN(RequestState);
  };

  // Calls back |callback| with the result and no error.
  void CallBackWithResult(const VerifyCallback& callback,
                          scoped_ptr<net::CertVerifyResult> verify_result,
                          RequestState* request_state,
                          int return_value);

  // One CertVerifier per extension to verify trust. Each verifier is created on
  // first usage and deleted when this IOPart is destructed or the respective
  // extension is unloaded.
  std::map<std::string, linked_ptr<net::CertVerifier>> extension_to_verifier_;
};

// static
BrowserContextKeyedAPIFactory<VerifyTrustAPI>*
VerifyTrustAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

template <>
void BrowserContextKeyedAPIFactory<
    VerifyTrustAPI>::DeclareFactoryDependencies() {
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
  DependsOn(ExtensionRegistryFactory::GetInstance());
}

VerifyTrustAPI::VerifyTrustAPI(content::BrowserContext* context)
    : io_part_(new IOPart), registry_observer_(this), weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  registry_observer_.Add(ExtensionRegistry::Get(context));
}

VerifyTrustAPI::~VerifyTrustAPI() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void VerifyTrustAPI::Verify(scoped_ptr<Params> params,
                            const std::string& extension_id,
                            const VerifyCallback& ui_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Call back through the VerifyTrustAPI object on the UIThread. Because of the
  // WeakPtr usage, this will ensure that |ui_callback| is not called after the
  // API is destroyed.
  VerifyCallback finish_callback(base::Bind(
      &CallBackOnUI, base::Bind(&VerifyTrustAPI::FinishedVerificationOnUI,
                                weak_factory_.GetWeakPtr(), ui_callback)));

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&IOPart::Verify, base::Unretained(io_part_.get()),
                 base::Passed(&params), extension_id, finish_callback));
}

void VerifyTrustAPI::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionInfo::Reason reason) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&IOPart::OnExtensionUnloaded, base::Unretained(io_part_.get()),
                 extension->id()));
}

void VerifyTrustAPI::FinishedVerificationOnUI(const VerifyCallback& ui_callback,
                                              const std::string& error,
                                              int return_value,
                                              int cert_status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  ui_callback.Run(error, return_value, cert_status);
}

// static
void VerifyTrustAPI::CallBackOnUI(const VerifyCallback& ui_callback,
                                  const std::string& error,
                                  int return_value,
                                  int cert_status) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(ui_callback, error, return_value, cert_status));
}

VerifyTrustAPI::IOPart::~IOPart() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

void VerifyTrustAPI::IOPart::Verify(scoped_ptr<Params> params,
                                    const std::string& extension_id,
                                    const VerifyCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  const api::platform_keys::VerificationDetails& details = params->details;

  if (details.server_certificate_chain.empty()) {
    callback.Run(kErrorEmptyCertificateChain, 0, 0);
    return;
  }

  std::vector<base::StringPiece> der_cert_chain;
  for (const std::vector<char>& cert_der : details.server_certificate_chain) {
    if (cert_der.empty()) {
      callback.Run(platform_keys::kErrorInvalidX509Cert, 0, 0);
      return;
    }
    der_cert_chain.push_back(base::StringPiece(
        reinterpret_cast<const char*>(vector_as_array(&cert_der)),
        cert_der.size()));
  }
  scoped_refptr<net::X509Certificate> cert_chain(
      net::X509Certificate::CreateFromDERCertChain(der_cert_chain));
  if (!cert_chain) {
    callback.Run(platform_keys::kErrorInvalidX509Cert, 0, 0);
    return;
  }

  if (!ContainsKey(extension_to_verifier_, extension_id)) {
    extension_to_verifier_[extension_id] =
        make_linked_ptr(net::CertVerifier::CreateDefault().release());
  }
  net::CertVerifier* verifier = extension_to_verifier_[extension_id].get();

  scoped_ptr<net::CertVerifyResult> verify_result(new net::CertVerifyResult);
  scoped_ptr<net::BoundNetLog> net_log(new net::BoundNetLog);
  const int flags = 0;

  std::string ocsp_response;
  net::CertVerifyResult* const verify_result_ptr = verify_result.get();

  RequestState* request_state = new RequestState();
  base::Callback<void(int)> bound_callback(
      base::Bind(&IOPart::CallBackWithResult, base::Unretained(this), callback,
                 base::Passed(&verify_result), base::Owned(request_state)));

  const int return_value = verifier->Verify(
      cert_chain.get(), details.hostname, ocsp_response, flags,
      net::SSLConfigService::GetCRLSet().get(), verify_result_ptr,
      bound_callback, &request_state->request, *net_log);

  if (return_value != net::ERR_IO_PENDING) {
    bound_callback.Run(return_value);
    return;
  }
}

void VerifyTrustAPI::IOPart::OnExtensionUnloaded(
    const std::string& extension_id) {
  extension_to_verifier_.erase(extension_id);
}

void VerifyTrustAPI::IOPart::CallBackWithResult(
    const VerifyCallback& callback,
    scoped_ptr<net::CertVerifyResult> verify_result,
    RequestState* request_state,
    int return_value) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  callback.Run(std::string() /* no error message */, return_value,
               verify_result->cert_status);
}

}  // namespace extensions
