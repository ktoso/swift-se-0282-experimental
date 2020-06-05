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

/// An atomic optional strong reference that can be set (initialized) exactly
/// once, but read many times.
@frozen
public struct UnsafeAtomicLazyReference<Instance: AnyObject> {
  public typealias Value = Instance?

  @usableFromInline
  internal let _ptr: UnsafeMutablePointer<_Rep>

  @_transparent // Debug performance
  public init(@_nonEphemeral at pointer: UnsafeMutablePointer<Storage>) {
    // `Storage` is layout-compatible with its only stored property, `_Rep`.
    _ptr = UnsafeMutableRawPointer(pointer).assumingMemoryBound(to: _Rep.self)
  }

  @_transparent @_alwaysEmitIntoClient
  @usableFromInline
  var _address: UnsafeMutablePointer<Storage> {
    // `Storage` is layout-compatible with its only stored property, `_Rep`.
    UnsafeMutableRawPointer(_ptr).assumingMemoryBound(to: Storage.self)
  }
}

extension UnsafeAtomicLazyReference {
  @usableFromInline
  internal typealias _Rep = Unmanaged<Instance>.AtomicOptionalRepresentation

  @frozen
  public struct Storage {
    @usableFromInline
    internal var _storage: _Rep

    @inlinable @inline(__always)
    public init() {
      _storage = _Rep(nil)
    }

    @inlinable @inline(__always)
    @discardableResult
    public mutating func dispose() -> Value {
      defer { _storage = _Rep(nil) }
      return _storage.dispose()?.takeRetainedValue()
    }
  }
}

extension UnsafeAtomicLazyReference {
  @inlinable
  public static func create() -> Self {
    let ptr = UnsafeMutablePointer<Storage>.allocate(capacity: 1)
    ptr.initialize(to: Storage())
    return Self(at: ptr)
  }

  @discardableResult
  @inlinable
  public func destroy() -> Value {
    let address = _address
    defer { address.deallocate() }
    return address.pointee.dispose()
  }
}

extension UnsafeAtomicLazyReference {
  /// Atomically initializes this reference if its current value is nil, then
  /// returns the initialized value. If this reference is already initialized,
  /// then `initialize(to:)` discards its supplied argument and returns the
  /// current value without updating it.
  ///
  /// The following example demonstrates how this can be used to implement a
  /// thread-safe lazily initialized reference:
  ///
  /// ```
  /// class Image {
  ///   var _histogram: UnsafeAtomicLazyReference<Histogram> = ...
  ///
  ///   // This is safe to call concurrently from multiple threads.
  ///   var atomicLazyHistogram: Histogram {
  ///     if let histogram = _histogram.load() { return foo }
  ///     // Note that code here may run concurrently on
  ///     // multiple threads, but only one of them will get to
  ///     // succeed setting the reference.
  ///     let histogram = ...
  ///     return _histogram.storeIfNilThenLoad(foo)
  /// }
  /// ```
  ///
  /// This operation uses acquiring-and-releasing memory ordering.
  public func storeIfNilThenLoad(_ desired: __owned Instance) -> Instance {
    let desiredUnmanaged = Unmanaged.passRetained(desired)
    let (exchanged, current) = _Rep.atomicCompareExchange(
            expected: nil,
            desired: desiredUnmanaged,
            at: _ptr,
            ordering: .acquiringAndReleasing)
    if !exchanged {
      // The reference has already been initialized. Balance the retain that
      // we performed on `desired`.
      desiredUnmanaged.release()
      return current!.takeUnretainedValue()
    }
    return desiredUnmanaged.takeUnretainedValue()
  }
}

extension UnsafeAtomicLazyReference {
  /// Atomically loads and returns the current value of this reference.
  ///
  /// The load operation is performed with the memory ordering
  /// `AtomicLoadOrdering.acquiring`.
  public func load() -> Instance? {
    let value = _Rep.atomicLoad(at: _ptr, ordering: .acquiring)
    return value?.takeUnretainedValue()
  }
}
