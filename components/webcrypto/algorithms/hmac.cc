// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <openssl/hmac.h>

#include "base/logging.h"
#include "base/stl_util.h"
#include "components/webcrypto/algorithm_implementation.h"
#include "components/webcrypto/algorithms/util_openssl.h"
#include "components/webcrypto/crypto_data.h"
#include "components/webcrypto/jwk.h"
#include "components/webcrypto/key.h"
#include "components/webcrypto/status.h"
#include "components/webcrypto/webcrypto_util.h"
#include "crypto/openssl_util.h"
#include "crypto/secure_util.h"
#include "third_party/WebKit/public/platform/WebCryptoAlgorithmParams.h"
#include "third_party/WebKit/public/platform/WebCryptoKeyAlgorithm.h"

namespace webcrypto {

namespace {

const char* GetJwkHmacAlgorithmName(blink::WebCryptoAlgorithmId hash) {
  switch (hash) {
    case blink::WebCryptoAlgorithmIdSha1:
      return "HS1";
    case blink::WebCryptoAlgorithmIdSha256:
      return "HS256";
    case blink::WebCryptoAlgorithmIdSha384:
      return "HS384";
    case blink::WebCryptoAlgorithmIdSha512:
      return "HS512";
    default:
      return NULL;
  }
}

const blink::WebCryptoKeyUsageMask kAllKeyUsages =
    blink::WebCryptoKeyUsageSign | blink::WebCryptoKeyUsageVerify;

Status SignHmac(const std::vector<uint8_t>& raw_key,
                const blink::WebCryptoAlgorithm& hash,
                const CryptoData& data,
                std::vector<uint8_t>* buffer) {
  crypto::OpenSSLErrStackTracer err_tracer(FROM_HERE);

  const EVP_MD* digest_algorithm = GetDigest(hash.id());
  if (!digest_algorithm)
    return Status::ErrorUnsupported();
  size_t hmac_expected_length = EVP_MD_size(digest_algorithm);

  buffer->resize(hmac_expected_length);
  crypto::ScopedOpenSSLSafeSizeBuffer<EVP_MAX_MD_SIZE> hmac_result(
      vector_as_array(buffer), hmac_expected_length);

  unsigned int hmac_actual_length;
  unsigned char* const success = HMAC(
      digest_algorithm, vector_as_array(&raw_key), raw_key.size(), data.bytes(),
      data.byte_length(), hmac_result.safe_buffer(), &hmac_actual_length);
  if (!success || hmac_actual_length != hmac_expected_length)
    return Status::OperationError();

  return Status::Success();
}

class HmacImplementation : public AlgorithmImplementation {
 public:
  HmacImplementation() {}

  Status GenerateKey(const blink::WebCryptoAlgorithm& algorithm,
                     bool extractable,
                     blink::WebCryptoKeyUsageMask usages,
                     GenerateKeyResult* result) const override {
    Status status = CheckKeyCreationUsages(kAllKeyUsages, usages, false);
    if (status.IsError())
      return status;

    const blink::WebCryptoHmacKeyGenParams* params =
        algorithm.hmacKeyGenParams();

    unsigned int keylen_bits = 0;
    status = GetHmacKeyGenLengthInBits(params, &keylen_bits);
    if (status.IsError())
      return status;

    return GenerateWebCryptoSecretKey(blink::WebCryptoKeyAlgorithm::createHmac(
                                          params->hash().id(), keylen_bits),
                                      extractable, usages, keylen_bits, result);
  }

  Status VerifyKeyUsagesBeforeImportKey(
      blink::WebCryptoKeyFormat format,
      blink::WebCryptoKeyUsageMask usages) const override {
    switch (format) {
      case blink::WebCryptoKeyFormatRaw:
      case blink::WebCryptoKeyFormatJwk:
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
    const blink::WebCryptoHmacImportParams* params =
        algorithm.hmacImportParams();

    unsigned int keylen_bits = 0;
    Status status = GetHmacImportKeyLengthBits(params, key_data.byte_length(),
                                               &keylen_bits);
    if (status.IsError())
      return status;

    const blink::WebCryptoKeyAlgorithm key_algorithm =
        blink::WebCryptoKeyAlgorithm::createHmac(params->hash().id(),
                                                 keylen_bits);

    // If no bit truncation was requested, then done!
    if ((keylen_bits % 8) == 0) {
      return CreateWebCryptoSecretKey(key_data, key_algorithm, extractable,
                                      usages, key);
    }

    // Otherwise zero out the unused bits in the key data before importing.
    std::vector<uint8_t> modified_key_data(
        key_data.bytes(), key_data.bytes() + key_data.byte_length());
    TruncateToBitLength(keylen_bits, &modified_key_data);
    return CreateWebCryptoSecretKey(CryptoData(modified_key_data),
                                    key_algorithm, extractable, usages, key);
  }

  Status ImportKeyJwk(const CryptoData& key_data,
                      const blink::WebCryptoAlgorithm& algorithm,
                      bool extractable,
                      blink::WebCryptoKeyUsageMask usages,
                      blink::WebCryptoKey* key) const override {
    const char* algorithm_name =
        GetJwkHmacAlgorithmName(algorithm.hmacImportParams()->hash().id());
    if (!algorithm_name)
      return Status::ErrorUnexpected();

    std::vector<uint8_t> raw_data;
    JwkReader jwk;
    Status status = ReadSecretKeyNoExpectedAlg(key_data, extractable, usages,
                                               &raw_data, &jwk);
    if (status.IsError())
      return status;
    status = jwk.VerifyAlg(algorithm_name);
    if (status.IsError())
      return status;

    return ImportKeyRaw(CryptoData(raw_data), algorithm, extractable, usages,
                        key);
  }

  Status ExportKeyRaw(const blink::WebCryptoKey& key,
                      std::vector<uint8_t>* buffer) const override {
    *buffer = GetSymmetricKeyData(key);
    return Status::Success();
  }

  Status ExportKeyJwk(const blink::WebCryptoKey& key,
                      std::vector<uint8_t>* buffer) const override {
    const std::vector<uint8_t>& raw_data = GetSymmetricKeyData(key);

    const char* algorithm_name =
        GetJwkHmacAlgorithmName(key.algorithm().hmacParams()->hash().id());
    if (!algorithm_name)
      return Status::ErrorUnexpected();

    WriteSecretKeyJwk(CryptoData(raw_data), algorithm_name, key.extractable(),
                      key.usages(), buffer);

    return Status::Success();
  }

  Status Sign(const blink::WebCryptoAlgorithm& algorithm,
              const blink::WebCryptoKey& key,
              const CryptoData& data,
              std::vector<uint8_t>* buffer) const override {
    const blink::WebCryptoAlgorithm& hash =
        key.algorithm().hmacParams()->hash();

    return SignHmac(GetSymmetricKeyData(key), hash, data, buffer);
  }

  Status Verify(const blink::WebCryptoAlgorithm& algorithm,
                const blink::WebCryptoKey& key,
                const CryptoData& signature,
                const CryptoData& data,
                bool* signature_match) const override {
    std::vector<uint8_t> result;
    Status status = Sign(algorithm, key, data, &result);

    if (status.IsError())
      return status;

    // Do not allow verification of truncated MACs.
    *signature_match =
        result.size() == signature.byte_length() &&
        crypto::SecureMemEqual(vector_as_array(&result), signature.bytes(),
                               signature.byte_length());

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
    return GetHmacKeyLength(key_length_algorithm, has_length_bits, length_bits);
  }
};

}  // namespace

scoped_ptr<AlgorithmImplementation> CreateHmacImplementation() {
  return make_scoped_ptr(new HmacImplementation);
}

}  // namespace webcrypto
