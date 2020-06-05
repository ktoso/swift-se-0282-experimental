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

import XCTest
import SE0282_Experimental

enum State: Int, AtomicValue {
  case starting
  case running
  case stopped
}

class AtomicRawRepresentableTests: XCTestCase {
    func test_load() {
        let v = UnsafeAtomic<State>.create(initialValue: .starting)
        defer { v.destroy() }
        XCTAssertEqual(v.load(ordering: .relaxed), State.starting)
    }

    func test_store() {
        let v = UnsafeAtomic<State>.create(initialValue: .starting)
        defer { v.destroy() }
        XCTAssertEqual(State.starting, v.load(ordering: .relaxed))
        v.store(.running, ordering: .relaxed)
        XCTAssertEqual(State.running, v.load(ordering: .relaxed))
    }

    func test_exchange() {
        let v = UnsafeAtomic<State>.create(initialValue: .starting)
        defer { v.destroy() }
        XCTAssertEqual(State.starting, v.load(ordering: .relaxed))
        XCTAssertEqual(State.starting, v.exchange(.running, ordering: .relaxed))
        XCTAssertEqual(State.running, v.load(ordering: .relaxed))
        XCTAssertEqual(State.running, v.exchange(.stopped, ordering: .relaxed))
        XCTAssertEqual(State.stopped, v.load(ordering: .relaxed))
    }

    func test_compareExchange() {
        let v = UnsafeAtomic<State>.create(initialValue: .starting)
        defer { v.destroy() }
        XCTAssertEqual(State.starting, v.load(ordering: .relaxed))

        var (success, old) = v.compareExchange(
                expected: .starting,
                desired: .running,
                ordering: .relaxed)
        XCTAssertTrue(success)
        XCTAssertEqual(State.starting, old)
        XCTAssertEqual(State.running, v.load(ordering: .relaxed))

        (success, old) = v.compareExchange(
            expected: .starting,
            desired: .stopped,
            ordering: .relaxed)
        XCTAssertFalse(success)
        XCTAssertEqual(.running, old)
        XCTAssertEqual(State.running, v.load(ordering: .relaxed))

        (success, old) = v.compareExchange(
            expected: .running,
            desired: .stopped,
            ordering: .relaxed)
        XCTAssertTrue(success)
        XCTAssertEqual(State.running, old)
        XCTAssertEqual(State.stopped, v.load(ordering: .relaxed))
    }
}
