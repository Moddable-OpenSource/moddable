---
name: Logging
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

The Web platform Console APIs (e.g. [`console.log`](https://developer.mozilla.org/en-US/docs/Web/API/console/log_static)) and the Moddable SDK's `trace()` are both available to Embedded JavaScript developers using the Moddable SDK. In fact, the Web platform's Console API is implemented using the Moddable SDK's `trace()` function.

## Using console

- [Logging with Console](./logging-console.md)

## Using `trace()`

- [Logging with `trace()`](./logging-trace.md)

## How to Choose

The two APIs are equivalent for most purposes. Use whichever is most comfortable. The Moddable SDK's built-in `trace()` is a little lighter than `console.log()` but requires the developer to include linefeeds. If you are strongly concerned about minimizing code size and runtime overhead, use `trace()`.

## Building with mcconfig

`trace()` is built into the XS JavaScript engine, so no manifest changes are required to use it.

To use the Web platform's Console APIs, include their manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/web/console

The `console` global is initialized by the module.

## Building with mcpack

When building with `mcpack`, the `console` global is automatically installed if your project uses it.

## Learn More

- Documentation
	- [`console`](https://developer.mozilla.org/en-US/docs/Web/API/console) on MDN
	- [`trace()`](../../documentation/xs/xsbug.md#colorize) in Moddable SDK
- Standard
	- [Console](https://console.spec.whatwg.org/) in the WHATWG Console Standard
- Implementation
	- [Console](../../modules/web/console/console.js) in Moddable SDK
- TypeScript Declarations
	- [`console`](../../typings/web/console.d.ts) in Moddable SDK
	- [`xs.d.ts`](../../typings/xs.d.ts) in Moddable SDK — declares `trace()`
