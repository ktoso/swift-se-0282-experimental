//===----------------------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2020 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

// Swift-importable shims for C atomics.
//
// This file defines an atomic storage representation and 56 atomic operations
// for each of the 10 standard integer types in the Standard Library. To make
// this manageable, we use the C preprocessor to stamp these out.

#include <stdbool.h>
#include <stdatomic.h>
#include <assert.h>

#define SWIFTATOMIC_INLINE static inline __attribute__((__always_inline__))
#define SWIFTATOMIC_SWIFT_NAME(name) __attribute__((swift_name(#name)))

// Atomic fences
#define SWIFTATOMIC_THREAD_FENCE_FN(order)                              \
  SWIFTATOMIC_INLINE void _sa_thread_fence_ ## order(void)              \
  {                                                                     \
    atomic_thread_fence(memory_order_ ## order);                        \
  }

SWIFTATOMIC_THREAD_FENCE_FN(acquire)
SWIFTATOMIC_THREAD_FENCE_FN(release)
SWIFTATOMIC_THREAD_FENCE_FN(acq_rel)
SWIFTATOMIC_THREAD_FENCE_FN(seq_cst)

// Definition of atomic storage type
#define SWIFTATOMIC_STORAGE_TYPE(swiftType, cType)                      \
  typedef struct {                                                      \
    _Atomic(cType) value;                                               \
  } _sa_ ## swiftType                                                   \
  SWIFTATOMIC_SWIFT_NAME(_Atomic ## swiftType ## Storage);

// Storage value initializer
#define SWIFTATOMIC_PREPARE_FN(swiftType, cType)                        \
  SWIFTATOMIC_INLINE                                                    \
  _sa_ ## swiftType _sa_prepare_ ## swiftType(cType value)              \
  {                                                                     \
    _sa_ ## swiftType storage = { value };                              \
    assert(atomic_is_lock_free(&storage.value));                        \
    return storage;                                                     \
  }

// Storage value disposal function
#define SWIFTATOMIC_DISPOSE_FN(swiftType, cType)                        \
  SWIFTATOMIC_INLINE                                                    \
  cType _sa_dispose_ ## swiftType(_sa_ ## swiftType storage)            \
  {                                                                     \
    return storage.value;                                               \
  }

// Atomic load
#define SWIFTATOMIC_LOAD_FN(swiftType, cType, order)                    \
  SWIFTATOMIC_INLINE                                                    \
  cType _sa_load_ ## order ## _ ## swiftType(                           \
    _sa_ ## swiftType *ptr)                                             \
  {                                                                     \
    return atomic_load_explicit(&ptr->value, memory_order_ ## order);   \
  }

// Atomic store
#define SWIFTATOMIC_STORE_FN(swiftType, cType, order)                   \
  SWIFTATOMIC_INLINE                                                    \
  void _sa_store_ ## order ## _ ## swiftType(                           \
    _sa_ ## swiftType *ptr,                                             \
    cType desired)                                                      \
  {                                                                     \
    atomic_store_explicit(&ptr->value,                                  \
                          desired,                                      \
                          memory_order_ ## order);                      \
  }

// Atomic exchange
#define SWIFTATOMIC_EXCHANGE_FN(swiftType, cType, order)                \
  SWIFTATOMIC_INLINE                                                    \
  cType _sa_exchange_ ## order ## _ ## swiftType(                       \
    _sa_ ## swiftType *ptr,                                             \
    cType desired)                                                      \
  {                                                                     \
    return atomic_exchange_explicit(&ptr->value,                        \
                                    desired,                            \
                                    memory_order_ ## order);            \
  }

// Atomic compare/exchange
#define SWIFTATOMIC_CMPXCHG_FN(_kind, swiftType, cType, succ, fail)     \
  SWIFTATOMIC_INLINE                                                    \
  bool                                                                  \
  _sa_cmpxchg_ ## _kind ## _ ## succ ## _ ## fail ## _ ## swiftType(    \
    _sa_ ## swiftType *ptr,                                             \
    cType *expected,                                                    \
    cType desired)                                                      \
  {                                                                     \
    return atomic_compare_exchange_ ## _kind ## _explicit(              \
      &ptr->value,                                                      \
      expected,                                                         \
      desired,                                                          \
      memory_order_ ## succ,                                            \
      memory_order_ ## fail);                                           \
  }

// Atomic integer operations
#define SWIFTATOMIC_INTEGER_FN(op, swiftType, cType, order)             \
  SWIFTATOMIC_INLINE                                                    \
  cType _sa_fetch_ ## op ## _ ## order ## _ ## swiftType(               \
    _sa_ ## swiftType *ptr,                                             \
    cType operand)                                                      \
  {                                                                     \
    return atomic_fetch_ ## op ## _explicit(&ptr->value,                \
                                            operand,                    \
                                            memory_order_ ## order);    \
  }

// Functions for each supported operation + memory ordering combination
#define SWIFTATOMIC_STORE_FNS(swiftType, cType)                         \
  SWIFTATOMIC_STORE_FN(swiftType, cType, relaxed)                       \
  SWIFTATOMIC_STORE_FN(swiftType, cType, release)                       \
  SWIFTATOMIC_STORE_FN(swiftType, cType, seq_cst)

#define SWIFTATOMIC_LOAD_FNS(swiftType, cType)                          \
  SWIFTATOMIC_LOAD_FN(swiftType, cType, relaxed)                        \
  SWIFTATOMIC_LOAD_FN(swiftType, cType, acquire)                        \
  SWIFTATOMIC_LOAD_FN(swiftType, cType, seq_cst)

#define SWIFTATOMIC_EXCHANGE_FNS(swiftType, cType)                      \
  SWIFTATOMIC_EXCHANGE_FN(swiftType, cType, relaxed)                    \
  SWIFTATOMIC_EXCHANGE_FN(swiftType, cType, acquire)                    \
  SWIFTATOMIC_EXCHANGE_FN(swiftType, cType, release)                    \
  SWIFTATOMIC_EXCHANGE_FN(swiftType, cType, acq_rel)                    \
  SWIFTATOMIC_EXCHANGE_FN(swiftType, cType, seq_cst)

#define SWIFTATOMIC_CMPXCHG_FNS(kind, swiftType, cType)                 \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, relaxed, relaxed)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, acquire, relaxed)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, release, relaxed)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, acq_rel, relaxed)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, seq_cst, relaxed)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, acquire, acquire)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, acq_rel, acquire)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, seq_cst, acquire)      \
  SWIFTATOMIC_CMPXCHG_FN(kind, swiftType, cType, seq_cst, seq_cst)

#define SWIFTATOMIC_INTEGER_FNS(op, swiftType, cType)                   \
  SWIFTATOMIC_INTEGER_FN(op, swiftType, cType, relaxed)                 \
  SWIFTATOMIC_INTEGER_FN(op, swiftType, cType, acquire)                 \
  SWIFTATOMIC_INTEGER_FN(op, swiftType, cType, release)                 \
  SWIFTATOMIC_INTEGER_FN(op, swiftType, cType, acq_rel)                 \
  SWIFTATOMIC_INTEGER_FN(op, swiftType, cType, seq_cst)

#define SWIFTATOMIC_DEFINE_TYPE(swiftType, cType)                       \
  SWIFTATOMIC_STORAGE_TYPE(swiftType, cType)                            \
  SWIFTATOMIC_PREPARE_FN(swiftType, cType)                              \
  SWIFTATOMIC_DISPOSE_FN(swiftType, cType)                              \
  SWIFTATOMIC_LOAD_FNS(swiftType, cType)                                \
  SWIFTATOMIC_STORE_FNS(swiftType, cType)                               \
  SWIFTATOMIC_EXCHANGE_FNS(swiftType, cType)                            \
  SWIFTATOMIC_CMPXCHG_FNS(strong, swiftType, cType)                     \
  SWIFTATOMIC_CMPXCHG_FNS(weak, swiftType, cType)                       \
  SWIFTATOMIC_INTEGER_FNS(add, swiftType, cType)                        \
  SWIFTATOMIC_INTEGER_FNS(sub, swiftType, cType)                        \
  SWIFTATOMIC_INTEGER_FNS(or, swiftType, cType)                         \
  SWIFTATOMIC_INTEGER_FNS(xor, swiftType, cType)                        \
  SWIFTATOMIC_INTEGER_FNS(and, swiftType, cType)

// All known integer types
SWIFTATOMIC_DEFINE_TYPE(Int, intptr_t)
SWIFTATOMIC_DEFINE_TYPE(Int8, int8_t)
SWIFTATOMIC_DEFINE_TYPE(Int16, int16_t)
SWIFTATOMIC_DEFINE_TYPE(Int32, int32_t)
SWIFTATOMIC_DEFINE_TYPE(Int64, int64_t)

SWIFTATOMIC_DEFINE_TYPE(UInt, uintptr_t)
SWIFTATOMIC_DEFINE_TYPE(UInt8, uint8_t)
SWIFTATOMIC_DEFINE_TYPE(UInt16, uint16_t)
SWIFTATOMIC_DEFINE_TYPE(UInt32, uint32_t)
SWIFTATOMIC_DEFINE_TYPE(UInt64, uint64_t)
