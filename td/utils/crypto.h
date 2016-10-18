#pragma once

#include "td/utils/common.h"
#include "td/utils/Slice.h"

#include <cstring>  // for memcpy

#include <algorithm>
#include <string>

namespace td {
struct UInt128;
}
namespace td {
struct UInt256;
}

namespace td {
uint64 pq_factorize(uint64 pq);

int pq_factorize(const Slice &pq_str, string *p_str, string *q_str);

template <class FromT>
string as_string(const FromT &from) {
  size_t size = sizeof(from);
  string res(size, '\0');

  auto ptr = reinterpret_cast<const int8 *>(&from);
  memcpy(&res[0], ptr, size);

  size_t i = size;
  while (i && res[i - 1] == 0) {
    i--;
  }

  res.resize(i);
  std::reverse(res.begin(), res.end());
  return res;
}

/*** AES ***/
ssize_t aes_ige_xcrypt(const UInt256 &aes_key, UInt256 *aes_iv, Slice from, MutableSlice to, bool encrypt_flag);
ssize_t aes_ige_encrypt(const UInt256 &aes_key, UInt256 *aes_iv, Slice from, MutableSlice to);
ssize_t aes_ige_decrypt(const UInt256 &aes_key, UInt256 *aes_iv, Slice from, MutableSlice to);

struct AesCtrState {
  UInt128 counter;
  UInt128 key;
  uint32 n;
};
void init_aes_ctr_state(const UInt128 &iv, AesCtrState *state);
ssize_t aes_ctr_xcrypt(const UInt256 &aes_key, AesCtrState *state, Slice from, MutableSlice to, bool encrypt_flag);
ssize_t aes_ctr_encrypt(const UInt256 &aes_key, AesCtrState *state, Slice from, MutableSlice to);
ssize_t aes_ctr_decrypt(const UInt256 &aes_key, AesCtrState *state, Slice from, MutableSlice to);

/*** SHA-1 ***/
void sha1(Slice data, unsigned char output[20]);

uint32 crc32(Slice data);
uint64 crc64(Slice data);

void sha256(Slice input, MutableSlice output);

struct Sha256StateImpl;

struct Sha256State {
  Sha256State();
  ~Sha256State();
  std::unique_ptr<Sha256StateImpl> impl;
};

void sha256_init(Sha256State *state);
void sha256_update(Slice data, Sha256State *state);
void sha256_final(Sha256State *state, MutableSlice output);

void md5(Slice input, MutableSlice output);

/*** Random ***/
class Random {
 public:
  static void secure_bytes(MutableSlice dest);
  static void secure_bytes(uint8 *ptr, size_t size);
  static int32 secure_int32();
  static int64 secure_int64();
  static uint32 fast_uint32();
  static uint64 fast_uint64();
};

}  // end namespace td