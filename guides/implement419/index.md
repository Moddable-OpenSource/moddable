---
name: Implementing ECMA-419 Modules
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-26
---

ECMA-419, [ECMAScript® Embedded Systems API Specification](https://419.ecma-international.org) defines standard APIs for Embedded JavaScript. This guide describes best practices for implementing these APIs. The focus is conformance: ensuring the implementations match the standard. That's important because developers depend on ECMA-419's common patterns. Fortunately, the implementation details are easy to get right, once you understand what the standard requires.

## Base Class Pattern

These guides cover topics that apply to most ECMA-419 implementations. Many of these requirements are based on ECMA-419's [Base Class Pattern](https://419.ecma-international.org/#base-class-pattern).

- [Constructor Sets `target`](./target.md)
- [Constructor Clean-up on Failure](./cleanup.md)
- [`close()` and `[Symbol.dispose]`](./close.md)
- [Calling Callbacks](./callbacks.md)
- [Keep Instance Surface Clean](./private.md)

## Peripheral Class Pattern

- [Setting Options with `configure()`](./configure.md)

## IO Class Pattern

- [Constructor IO](./io.md)

## Sensors

ECMA-419 Sensors are peripherals that obtain sample data from one or more inputs. Samples are retrieved by polling or notification that a new sample is available.

- [Sensor Constructor](./sensor-constructor.md)
- [Sensor Sample Polling](./sensor-poll.md)
- [Sensor Sample Notifications](./sensor-notify.md)
- [Sensor Sample Time](./sensor-time.md)
- [Sensor Configuration Report](./sensor-configuration.md)

The `configure()` method is the primary API for configuring how a sensor obtains and reports samples data. The guide [Setting Options with `configure()`](./configure.md) in the Peripheral Class Pattern describes how this works.

## Additional Guidance

These guides focus on modules implemented in JavaScript. ECMA-419 modules may also be implemented in C using the Moddable SDK's [XS in C](../../documentation/xs/XS%20in%20C.md) API. Whatever language is used, the conformance requirements are the same.

When implementing shared code, there is a strong tendency to try to help the user, for example by tolerating incorrectly formed inputs or resolving an ambiguity by guessing. This kind of help is problematic for portability, as it means that two different implementations of, for example, a temperature sensor may behave differently with the same inputs. As a rule, ECMA-419 methods should throw an exception so the caller fixes their code. ECMA-419 is fleixble on the error thrown, so you can provide as helpful a message as you feel is appropriate.

If possible, your module should be compatible with the Moddable SDK's preload feature. This allows your implementation to reside fully in flash memory, saving limited RAM space and reducing start-up time. The recommendations in these guides are compatible with preload. Check out the [preload documentation](../../documentation/xs/preload.md) for details.
