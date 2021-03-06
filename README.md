# Swift SE-0282 Experimental Atomics Library

This repository contains a proof-of-concept implementation of the APIs proposed in the [first version of SE-0282][SE-0282r0]. It demonstrates how the atomic operations provided by the C programming language can be exposed to Swift.

The goal is to enable intrepid developers to start building synchronization constructs directly in Swift. 

[SE-0282r0]: https://github.com/apple/swift-evolution/blob/3a358a07e878a58bec256639d2beb48461fc3177/proposals/0282-atomics.md
[SE-0282]: https://github.com/apple/swift-evolution/blob/master/proposals/0282-atomics.md

## Getting Started

This package serves as a supplement to the revised [SE-0282] proposal currently going through the Swift Evolution process. Accordingly, it is currently at an **experimental stage of development** -- it doesn't include any tagged releases, and currently **we do not recommend using this package in production**.

However, importing this package can be helpful while validating SE-0282's C-based approach. To do this, you need to set up a branch-based dependency:

```swift
// swift-tools-version:5.2
import PackageDescription

let package = Package(
  name: "MyPackage",
  dependencies: [
    .package(
      url: "https://github.com/apple/swift-se-0282-experimental.git",
      .branch("master")
    )
  ],
  targets: [
    .target(
      name: "MyTarget",
      dependencies: [
        .product(name: "SE0282_Experimental", package: "swift-se-0282-experimental")
      ]
    )
  ]
)
```

Note that this dependency will pick up every commit that lands on the master branch of this repository; some of these may include source-breaking changes. 

We expect the package will stabilize if/when SE-0282 is accepted. As part of this, **the name of the package and the exported module may change, requiring you to update your code**.

## Usage

Once you have successfully set up the dependency, you can start using it -- you just need to import the `SE0282_Experimental` module:

``` swift
import SE0282_Experimental
import Dispatch

let counter = UnsafeAtomic<Int>.create(initialValue: 0)

DispatchQueue.concurrentPerform(iterations: 10) { _ in
  for _ in 0 ..< 1_000_000 {
    counter.wrappingIncrement(ordering: .relaxed)
  }
}
print(counter.load(ordering: .relaxed))
counter.destroy()
```

For an introduction to the APIs provided by this package, for now please see the [first version of SE-0282][SE-0282r0].

In addition to the APIs originally proposed, this package also provides a `ManagedAtomic` generic class. This is a memory-safe reference type whose instances hold a single atomic value. This class provides an easy-to-use, safe interface that exactly matches the API of an eventual move-only atomic generic type. However, it requires allocating a separate class instance for every atomic value, which can add significant overhead compared to the lightweight, unsafe pointer-based `UnsafeAtomic`.

Ultimately, we expect both `ManagedAtomic` and `UnsafeAtomic` will be replaced by a single move-only atomic struct. Once that becomes possible, this module may get adopted as a core standard library component through the Swift Evolution process. (And rewritten to use native Swift builtins rather than C facilities.)

## Contributing

This repository is part of the Swift.org open source project. Its contents are licensed under the [Swift License]. For more information, see the Swift.org [Community Guidelines], [Contribution Guidelines], as well as the files [LICENSE.txt](./LICENSE.txt), [CONTRIBUTING.md](./CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md) at the root of this repository.

[Swift License]: https://swift.org/LICENSE.txt
[Community Guidelines]: https://swift.org/community/
[Contribution Guidelines]: https://swift.org/contributing/

## Development

This package defines a large number of similar-but-not-quite-the-same operations. To make it easier to maintain these, we use code generation to produce these.

A number of [source files](./Sources/Atomics) have a `.swift.gyb` extension. These are using a Python-based [code generation utility](./Utilities/gyb.py) which we also use within the Swift Standard Library. To make sure the package remains buildable by SPM, the autogenerated output files are committed into this repository. You must never edit the contents of `autogenerated` subdirectories, or your changes will get overwritten the next time the code is regenerated.

To regenerate sources (and to update the registry of XCTest tests), you need to manually run the script [`generate-sources`](./generate-sources) in the root of this repository. This needs to be done every time one of the templates is modified. If you rename or remove a `.gyb` file, you'll need to manually remove the corresponding generated file, or it will continue to build alongside the rest of the package. The script will warn you if it finds such stray files, but it won't remove them on its own.

The same script also runs `swift test --generate-linuxmain` to register any newly added unit tests.

In addition to gyb, the [`_AtomicsShims.h`](./Sources/_AtomicsShims/include/_AtomicsShims.h) header file uses the C preprocessor to define trivial wrapper functions for every supported atomic operation -- memory ordering pairing.

