// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/stl_util.h"
#include "components/webcrypto/algorithm_implementation.h"
#include "components/webcrypto/algorithms/secret_key_util.h"
#include "components/webcrypto/algorithms/util_openssl.h"
#include "components/webcrypto/crypto_data.h"
#include "components/webcrypto/key.h"
#include "components/webcrypto/status.h"
#include "components/webcrypto/webcrypto_util.h"
#include "crypto/openssl_util.h"
#include "third_party/WebKit/public/platform/WebCryptoAlgorithmParams.h"
#include "third_party/WebKit/public/platform/WebCryptoKeyAlgorithm.h"

namespace webcrypto {

namespace {

const blink::WebCryptoKeyUsageMask kAllKeyUsages =
    blink::WebCryptoKeyUsageDeriveKey | blink::WebCryptoKeyUsageDeriveBits;

class Pbkdf2Implementation : public AlgorithmImplementation {
 public:
  Pbkdf2Implementation() {}

  Status VerifyKeyUsagesBeforeImportKey(
      blink::WebCryptoKeyFormat format,
      blink::WebCryptoKeyUsageMask usages) const override {
    switch (format) {
      case blink::WebCryptoKeyFormatRaw:
        return CheckKeyCreationUsages(kAllKeyUsages, usages, false);
      default:
        return Status::ErrorUnsupportedImportKeyFormat();
    }
  }

  Status ImportKeyRaw(const CryptoData& key_data,
                      const blink::WebCryptoAlgorithm& algorithm,
                      bool extractable,
                      blink::WebCryptoKeyUsageMask usages,
                      blink::WebCryptoKey* key) const override {
    const blink::WebCryptoKeyAlgorithm key_algorithm =
        blink::WebCryptoKeyAlgorithm::createWithoutParams(
            blink::WebCryptoAlgorithmIdPbkdf2);

    return CreateWebCryptoSecretKey(key_data, key_algorithm, extractable,
                                    usages, key);
  }

  Status DeriveBits(const blink::WebCryptoAlgorithm& algorithm,
                    const blink::WebCryptoKey& base_key,
                    bool has_optional_length_bits,
                    unsigned int optional_length_bits,
                    std::vector<uint8_t>* derived_bytes) const override {
    crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);

    if (!has_optional_length_bits)
      return Status::ErrorPbkdf2DeriveBitsLengthNotSpecified();

    if (optional_length_bits % 8)
      return Status::ErrorPbkdf2InvalidLength();

    const blink::WebCryptoPbkdf2Params* params = algorithm.pbkdf2Params();

    const blink::WebCryptoAlgorithm& hash = params->hash();
    const EVP_MD* digest_algorithm = GetDigest(hash.id());
    if (!digest_algorithm)
      return Status::ErrorUnsupported();

    unsigned int keylen_bytes = optional_length_bits / 8;
    derived_bytes->resize(keylen_bytes);

    const std::vector<uint8_t>& password = GetSymmetricKeyData(base_key);

    if (!PKCS5_PBKDF2_HMAC(
            reinterpret_cast<const char*>(vector_as_array(&password)),
            password.size(), params->salt().data(), params->salt().size(),
            params->iterations(), digest_algorithm, keylen_bytes,
            vector_as_array(derived_bytes))) {
      return Status::OperationError();
    }
    return Status::Success();
  }

  Status DeserializeKeyForClone(const blink::WebCryptoKeyAlgorithm& algorithm,
                                blink::WebCryptoKeyType type,
                                bool extractable,
                                blink::WebCryptoKeyUsageMask usages,
                                const CryptoData& key_data,
                                blink::WebCryptoKey* key) const override {
    return CreateWebCryptoSecretKey(key_data, algorithm, extractable, usages,
                                    key);
  }

  Status GetKeyLength(const blink::WebCryptoAlgorithm& key_length_algorithm,
                      bool* has_length_bits,
                      unsigned int* length_bits) const override {
    *has_length_bits = false;
    return Status::Success();
  }
};

}  // namespace

scoped_ptr<AlgorithmImplementation> CreatePbkdf2Implementation() {
  return make_scoped_ptr(new Pbkdf2Implementation);
}

}  // namespace webcrypto
