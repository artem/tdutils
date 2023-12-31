#pragma once

#include "td/utils/buffer.h"
#include "td/utils/common.h"
#include "td/utils/Slice.h"
#include "td/utils/Status.h"
#include "td/utils/UInt.h"

namespace td {

uint64 pq_factorize(uint64 pq);

#if TD_HAVE_OPENSSL
void init_crypto();

int pq_factorize(Slice pq_str, string *p_str, string *q_str);

void aes_ige_encrypt(const UInt256 &aes_key, UInt256 *aes_iv, Slice from, MutableSlice to);
void aes_ige_decrypt(const UInt256 &aes_key, UInt256 *aes_iv, Slice from, MutableSlice to);

void aes_cbc_encrypt(const UInt256 &aes_key, UInt128 *aes_iv, Slice from, MutableSlice to);
void aes_cbc_decrypt(const UInt256 &aes_key, UInt128 *aes_iv, Slice from, MutableSlice to);

class AesCtrState {
 public:
  AesCtrState();
  AesCtrState(const AesCtrState &from) = delete;
  AesCtrState &operator=(const AesCtrState &from) = delete;
  AesCtrState(AesCtrState &&from);
  AesCtrState &operator=(AesCtrState &&from);
  ~AesCtrState();

  void init(const UInt256 &key, const UInt128 &iv);

  void encrypt(Slice from, MutableSlice to);

  void decrypt(Slice from, MutableSlice to);

 private:
  class Impl;
  unique_ptr<Impl> ctx_;
};

class AesCbcState {
 public:
  AesCbcState(const UInt256 &key, const UInt128 &iv);

  void encrypt(Slice from, MutableSlice to);
  void decrypt(Slice from, MutableSlice to);

 private:
  UInt256 key_;
  UInt128 iv_;
};

void sha1(Slice data, unsigned char output[20]);

void sha256(Slice data, MutableSlice output);

void sha512(Slice data, MutableSlice output);

string sha256(Slice data) TD_WARN_UNUSED_RESULT;

string sha512(Slice data) TD_WARN_UNUSED_RESULT;

struct Sha256StateImpl;

struct Sha256State;
void sha256_init(Sha256State *state);
void sha256_update(Slice data, Sha256State *state);
void sha256_final(Sha256State *state, MutableSlice output, bool destroy = true);

struct Sha256State {
  Sha256State();
  Sha256State(Sha256State &&from);
  Sha256State &operator=(Sha256State &&from);
  ~Sha256State();
  void init() {
    sha256_init(this);
  }
  void feed(Slice data) {
    sha256_update(data, this);
  }
  void extract(MutableSlice dest) {
    sha256_final(this, dest, false);
  }
  unique_ptr<Sha256StateImpl> impl;
};

void md5(Slice input, MutableSlice output);

void pbkdf2_sha256(Slice password, Slice salt, int iteration_count, MutableSlice dest);
void pbkdf2_sha512(Slice password, Slice salt, int iteration_count, MutableSlice dest);

void hmac_sha256(Slice key, Slice message, MutableSlice dest);
void hmac_sha512(Slice key, Slice message, MutableSlice dest);

// Interface may be improved
Result<BufferSlice> rsa_encrypt_pkcs1_oaep(Slice public_key, Slice data);
Result<BufferSlice> rsa_decrypt_pkcs1_oaep(Slice private_key, Slice data);

void init_openssl_threads();
#endif

#if TD_HAVE_ZLIB
uint32 crc32(Slice data);
#endif

#if TD_HAVE_CRC32C
uint32 crc32c(Slice data);
uint32 crc32c_extend(uint32 old_crc, Slice data);
uint32 crc32c_extend(uint32 old_crc, uint32 new_crc, size_t data_size);
#endif

uint64 crc64(Slice data);
uint16 crc16(Slice data);

}  // namespace td
