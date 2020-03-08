//===- FuzzerValueBitMap.h - INTERNAL - Bit map -----------------*- C++ -* ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// ValueBitMap.
//===----------------------------------------------------------------------===//

#pragma once

#include <string.h>

// A bit map containing kMapSizeInWords bits.
struct ValueBitMap {
  static const size_t kMapSizeInBits = 1 << 16;
  static const size_t kMapPrimeMod = 65371;  // Largest Prime < kMapSizeInBits;
  static const size_t kBitsInWord = (sizeof(uintptr_t) * 8);
  static const size_t kMapSizeInWords = kMapSizeInBits / kBitsInWord;
 public:

  // Clears all bits.
  void Reset() { memset(Map, 0, sizeof(Map)); }

  // Computes a hash function of Value and sets the corresponding bit.
  // Returns true if the bit was changed from 0 to 1.
  inline bool AddValue(uintptr_t Value) {
    uintptr_t Idx = Value % kMapSizeInBits;
    uintptr_t WordIdx = Idx / kBitsInWord;
    uintptr_t BitIdx = Idx % kBitsInWord;
    uintptr_t Old = Map[WordIdx];
    uintptr_t New = Old | (1UL << BitIdx);
    Map[WordIdx] = New;
    return New != Old;
  }

  inline bool AddValueModPrime(uintptr_t Value) {
    return AddValue(Value % kMapPrimeMod);
  }

  inline bool Get(uintptr_t Idx) {
    uintptr_t WordIdx = Idx / kBitsInWord;
    uintptr_t BitIdx = Idx % kBitsInWord;
    return Map[WordIdx] & (1UL << BitIdx);
  }

  size_t GetNumBitsSinceLastMerge() const { return NumBits; }

  // Merges 'Other' into 'this', clears 'Other', updates NumBits,
  // returns true if new bits were added.
  bool MergeFrom(ValueBitMap &Other) {
    uintptr_t Res = 0;
    size_t OldNumBits = NumBits;
    for (size_t i = 0; i < kMapSizeInWords; i++) {
      auto O = Other.Map[i];
      auto M = Map[i];
      if (O) {
        Map[i] = (M |= O);
        Other.Map[i] = 0;
      }
      if (M)
        Res += __builtin_popcountll(M);
    }
    NumBits = Res;
    return OldNumBits < NumBits;
  }

  template <class Callback>
  void ForEach(Callback CB) const {
    for (size_t i = 0; i < kMapSizeInWords; i++)
      if (uintptr_t M = Map[i])
        for (size_t j = 0; j < sizeof(M) * 8; j++)
          if (M & ((uintptr_t)1 << j))
            CB(i * sizeof(M) * 8 + j);
  }

 private:
  size_t NumBits = 0;
  uintptr_t Map[kMapSizeInWords] __attribute__((aligned(512)));
};
