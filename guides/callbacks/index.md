---
name: Time Callbacks
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

The web standard Timers API (e.g. [`setTimeout`](https://developer.mozilla.org/en-US/docs/Web/API/Window/setTimeout)) and the Moddable SDK's [`Timer`](../../documentation/base/base.md#timer) module are both available to Embedded JavaScript developers using the Moddable SDK. In fact, the Web platform's Timers API is implemented using the Moddable SDK's `Timer` module.

## Using Timer module

- [One-Time Callback](./callback-one-time.md)
- [Repeating Callback](./callback-repeating.md)
- [Repeating Callback with Initial Delay](./callback-repeating-initial-delay.md)
- [Immediate Callback](./callback-immediate.md)
- [Reschedule Callback](./callback-reschedule.md)
- [Cancel Callback](./callback-cancel.md)
- [Suspend Callback](./callback-suspend.md)

## How to Choose

The Moddable SDK prefers `Timer` for callbacks because it provides greater control and efficiency for Embedded JavaScript than the Web platform's `setTimeout()` family. `Timer` supports one-time, repeating, and immediate callbacks, and callbacks can be rescheduled, suspended, and canceled.

There is only benefit to using the Web platform's Timers API: code portability.

## Building with mcconfig

The `Timer` module is included in every Moddable SDK build, so no manifest changes are required. Import it in your JavaScript source code:

```js
import Timer from "timer";
```

To use the Web platform's Timers API, include its manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/web/timers

The `setTimeout()` family of functions is installed as globals by the module, so no import is required.

## Building with mcpack

When building with `mcpack`, the `setTimeout()`, `setInterval()`, and `setImmediate()` functions, along with the corresponding `clearTimeout()`,` clearInterval()`, and `clearImmediate()` functions, are automatically installed if your project uses them.

## Learn More

- Documentation
	- [Timer](../../documentation/base/base.md#timer) in Moddable SDK
	- Web Timers API — [`setTimeout`](https://developer.mozilla.org/en-US/docs/Web/API/Window/setTimeout), [`clearTimeout`](https://developer.mozilla.org/en-US/docs/Web/API/Window/clearTimeout), [`setInterval`](https://developer.mozilla.org/en-US/docs/Web/API/Window/setInterval), [`clearInterval`](https://developer.mozilla.org/en-US/docs/Web/API/Window/clearInterval), [`setImmediate`](https://developer.mozilla.org/en-US/docs/Web/API/Window/setImmediate), and [`clearImmediate`](https://developer.mozilla.org/en-US/docs/Web/API/Window/clearImmediate) on MDN
- Example
	- [Timers example](../../examples/base/timers/main.js) in Moddable SDK
- Implementations
	- [Timer module](../../modules/base/timer/) in Moddable SDK
	- [Web Timers](../../modules/web/timers/) in Moddable SDK
- Standard
	- [Web Timers API](https://html.spec.whatwg.org/multipage/timers-and-user-prompts.html#dom-settimeout-dev) in WHATWG HTML Standard
- TypeScript Declarations
	- [Timer module](../../typings/timer.d.ts) in Moddable SDK
	- [Web Timers](../../typings/web/timers.d.ts) in Moddable SDK
