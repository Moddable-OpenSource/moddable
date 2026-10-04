# AGENTS.md
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: September 30, 2026

> **Note**: This document was written by an LLM for LLMs.

Guidance for AI coding agents working in the Moddable SDK repository. It covers what you may no be able to easily infer from the source tree, and links to the documentation for everything else.

Most of this file describes mistakes that are easy to make here and hard to diagnose. The sections most worth reading before you change anything are [The JavaScript language here is current, not a subset](#the-javascript-language-here-is-current-not-a-subset), [Console output and debugging](#console-output-and-debugging), [Choosing an API](#choosing-an-api), [Documentation that describes legacy APIs](#documentation-that-describes-legacy-apis), and [Manifests](#manifests).

Three assumptions carried over from Node.js will waste your time: that the language is a limited subset (it is not), that printed output reaches your terminal (it does not), and that a program exits when it finishes (it never finishes).

## What this is

The Moddable SDK builds JavaScript applications that run on microcontrollers. It contains the XS JavaScript engine, a runtime of JavaScript modules (many implemented entirely or partly in C), and the command line tools that compile, link, and deploy applications to devices and to a desktop simulator.

## The JavaScript language here is current, not a subset

**Do not write ES5-era JavaScript.** Embedded JavaScript engines are often severely limited, and that assumption is badly wrong here. XS tracks the current JavaScript language standard closely — of the test262 cases for features it implements, it passes 99.95% of the language tests and 99.85% of the built-ins tests.

Write modern JavaScript. All of the following are present, verified by running them:

- Classes with private fields and static initialization blocks, modules, `async`/`await`, generators and async generators, destructuring, template literals, optional chaining and nullish coalescing, `Proxy`, `Reflect`, `WeakRef`, `Map`/`Set`/`WeakMap`/`WeakSet`, typed arrays, `BigInt`, `SharedArrayBuffer`, `Atomics`
- ES2024/ES2025: `Object.groupBy`, iterator helpers (`.take`, `.drop`, …), `Set.prototype.union` and the other new set methods, `RegExp.escape`, `Promise.try`, `Float16Array`, the RegExp `d` flag, duplicate named capture groups
- ES2026: `Error.isError`, `Array.fromAsync`, `Math.sumPrecise`, `Uint8Array.fromBase64`, and explicit resource management — `using` declarations and `Symbol.dispose` both work

If you are unsure whether a recent feature is available, assume it is and check with `xst` rather than working around it.

What is genuinely absent or conditional, none of it about language level:

- **No ECMA-402** — the internationalization API (`Intl`) is not implemented.
- **`Temporal` is not implemented.** It is the largest single source of skipped conformance tests.
- **One realm per virtual machine.** `$262.createRealm` and multi-realm patterns do not work.
- **Script evaluation is optional per platform** — `eval` and `new Function` may be unavailable on a given target. `JSON.parse` is always available.
- **Built-ins can be dead-stripped.** The manifest `strip` property removes built-ins the application does not use; reaching one that was stripped throws `dead strip!` at build time. See the `strip` section of [documentation/tools/manifest.md](./documentation/tools/manifest.md).
- **This is not a browser.** There is no DOM, and no `window`. The web APIs that exist are the ones in [modules/web](./modules/web) and the globals listed in [eslint.config.mjs](./eslint.config.mjs).

The real constraint on a microcontroller is memory, not language features — see [`creation`](#creation--configuring-ram). What limits an application is how many features it uses *in combination*, not which ones are available.

To check a feature yourself, `xst` runs plain scripts on the host — `xst script.js`, using `print()` for output.

Background: [documentation/xs/XS Conformance.md](./documentation/xs/XS%20Conformance.md) for the per-feature results, and [documentation/xs/XS Differences.md](./documentation/xs/XS%20Differences.md) for how the compile-time/run-time split and object freezing shape the runtime model.

## Setup

Two environment variables and a `PATH` entry are required by every tool:

```shell
export MODDABLE="/path/to/this/repo"
export PATH="${MODDABLE}/build/bin/mac/release:$PATH"    # or lin, win
```

**Check `$MODDABLE` before building anything.** It is commonly already set in the shell, and pointing at a *different* checkout than the one you are editing. Every tool resolves `$(MODDABLE)` in manifests, so a stale value silently builds and tests the wrong tree while your edits appear to have no effect. Confirm it matches your working directory, and override it per command if it does not.

Tools are not checked in. Build them once before doing anything else:

```shell
cd $MODDABLE/build/makefiles/mac && make                 # or lin, win, wasm
```

The tools are built per checkout, into `$MODDABLE/build/bin/<platform>/release`. A second checkout needs its own `make` run; borrowing another checkout's binaries risks mismatching the XS version in the tree you are building.

ESP32 work additionally needs `IDF_PATH` pointing at an ESP-IDF installation, plus `source $IDF_PATH/export.sh` in each shell. See [documentation/Moddable SDK - Getting Started.md](./documentation/Moddable%20SDK%20-%20Getting%20Started.md) and [documentation/devices/esp32.md](./documentation/devices/esp32.md). Similarly, building for other microcontroller families requires installation and configuration of their SDKs (see Getting Started).

## Build and run

Two front ends, depending on how the project is described.

**`mcconfig`** builds projects that have a `manifest.json`:

```shell
cd $MODDABLE/examples/piu/balls
mcconfig -dl -m -p sim
```

- `-d` debug build, `-i` release-instrumented, neither means release
- `-dl` debug build using xsdb, `-dn` debug build with no debugger launched (see below)
- `-m` run `make`. **Required with every `-t` target, not just the default.** Without it `mcconfig` writes the makefile, prints nothing, and exits 0 — which is indistinguishable from a successful build or a program that produced no output. If a command you expected to do work returns instantly and silently, this is why.
- `-p <platform>` target platform; defaults to the host
- `-t build|deploy|xsbug|clean|all` target, default `all`
- trailing `key="value"` pairs merge into the manifest `config` section, readable at runtime as `import config from "mc/config"`

Two `-t` targets are worth knowing, because using them instead of the default is the difference between a fast loop and a slow one:

- **`-t build` is how you check that code compiles.** It runs the real build — `xsc` and `xsl` over every module, TypeScript type-checked where present — and stops before launching anything. This is the syntax and type check for a project; there is no separate lint or check step to reach for, and nothing here corresponds to `node --check`.
- **`-t xsbug` starts a debug session against the build that already exists**, skipping compile and deploy entirely. On a device this avoids re-flashing the binary, which dominates the edit-run cycle. Despite the name it works with `-dl` and xsdb, not just the xsbug GUI. **It does nothing on `-p sim`**, where it exits successfully having printed nothing at all — which reads as a hang or a crash rather than an unsupported target. Use the default target for the simulator.

Additionally, **`-t clean` removes all build artifacts** and is preferred to manually deleting build output directories.

Splitting the two — `-t build` to compile, then `-t xsbug` to run — is usually the right shape for automated or scripted work: only the build is slow, and it needs no timing guesswork, while launch and connect take well under a second.

```shell
cd <project>
mcconfig -d -m -p sim -t build                           # compile; note the -m
mcconfig -dl -m -p sim -t xsbug                          # run the build just made, under xsdb
```

The debug flag has to agree across the two: `-t xsbug` launches the build matching the flag you pass it, so a `-d`/`-dl` build is what `-dl -t xsbug` will find. A release build is not debuggable at all.

**A project is identified by the name of its directory, not by its path.** `mcconfig` sets `NAME` from the last component of the project directory and uses it alone to place output, in `build/tmp/<platform>/…/<NAME>` and `build/bin/<platform>/…/<NAME>`. Two projects whose directories happen to share a name therefore build over each other, no matter where they live in the file system. Nothing warns you: the build succeeds, and the project runs the other one's code. If a project behaves as though your edits had no effect — or traces output belonging to something else entirely — check for a name collision before you debug anything else. This bites hardest when creating scratch projects, where `test`, `app`, and `client` are tempting names; give each one a distinctive directory name instead. Alternatively, override the default name with a unique `NAME` in the manifest:

```json
"build": {
	"NAME": "scratch-test-irandom-12"
}
```

**`mcpack`** builds projects that have a `package.json`. It generates a manifest and chains to `mcconfig` (for applications) or `mcrun` (for mods):

```shell
cd $MODDABLE/examples/packages/hello
npm install                                              # first time only
mcpack mcconfig -dl -m -p sim
```

`mcpack` has no page under `documentation/tools/`. Its documentation is [examples/packages/readme.md](./examples/packages/readme.md), and each guide topic's `index.md` has a "Building with mcpack" section.

**Platform names** are the host by default (`mac`, `win`, `lin`), the simulator (`sim`, or `sim/<device>` for a device skin), or a device (`esp32/<target>`, `esp/<target>` (for ESP8266), `pico`, `nrf52`, `zephyr`, `wasm`). The valid targets for a platform are the directory names under `build/devices/<platform>/targets/`; simulator skins are under `build/simulators/`.

**A `-d` build launches and waits for the xsbug GUI debugger.** For unattended or headless work use `-dn` to build debug without launching a debugger, or `-dl` to use xsdb.

**On a device target, `-i` shows the serial console directly in the terminal.** `mcconfig -i -m -p esp32/<target>` builds instrumented, flashes, and then prints the device's serial output as it arrives — no debugger, no serial plumbing of your own. This is the way to see a native crash: a panic (core dump, `Guru Meditation`) never reaches xsdb — from the debugger's side it looks like the device silently going away — but it appears in full on the serial console. When a device under `-dl` hangs or disconnects for no visible reason, rerun with `-i` before theorizing.

**Without a TTY, `-i` on ESP32 flashes and then shows nothing.** The serial console comes from ESP-IDF's monitor, which exits immediately with `Monitor requires standard input to be attached to TTY` — after the flash has succeeded, so the build looks fine and the crash output is simply lost. Commands run by an agent usually have no terminal attached. Instead, flash with `mcconfig -i -m -p esp32/<target> -t deploy`, then read the serial port yourself for a fixed time — for example, a short pyserial loop at 115200 baud that reopens the port if it disappears, since USB-JTAG boards re-enumerate on reset. When scanning that output for a crash, look only at what follows the flash's `Hard resetting` line: the ESP-IDF build compiles `abort.c` and `panic.c`, so a naive search for `abort` or `panic` matches the build log itself.

### The simulator is not a device

`-p sim` runs your JavaScript on the host computer. It is the fastest way to check application logic, and it is weak evidence for anything that touches hardware. Pins, BLE, Wi-Fi, and displays are absent or stubbed, memory behaves nothing like a microcontroller (see [`static`](#creation--configuring-ram)), and timing is unrepresentative. "It works in the simulator" does not mean it works on the target — build for the real device before concluding a hardware-facing change is correct.

## Console output and debugging

**Nothing your code prints reaches your terminal.** This is not Node.js. `trace()` and `console.log()` write to the debug channel, not to stdout, so a program that runs without a debugger attached produces no visible output at all — including when it runs in the simulator. Building with `-dn` gets you a running program you cannot see.

**To see output, run under xsdb.** Traces appear at the `(xsdb)` prompt, prefixed with the thread they came from:

```
(xsdb) Continuing.
[Thread 1] Hello, world - sample
```

If you are looking for a log line and finding nothing, the usual cause is that no debugger is attached — not that the code did not run.

**The program never exits.** Also unlike Node.js: an application here runs forever. There is no "script finished, process exits" — the event loop keeps going, on real hardware and in the simulator alike. Do not run a build-and-run command and wait for it to return; it will block until whatever timeout you are under kills it, and you will read that as a hang or a failure.

Instead, drive the session explicitly: run under xsdb and issue `quit` when you have what you need, or start the program in the background and terminate it yourself. When scripting xsdb, allow time for the target to connect and reach your code before sending commands — commands sent too early are answered with `The program is not stopped.`

Piping commands to xsdb on stdin is enough for an unattended run — let it run for a while, then `quit`:

```shell
{ sleep 25; echo quit; } | mcconfig -dl -m -p sim -t xsbug
```

Traces are interleaved with the `(xsdb)` prompt and carry terminal escape sequences, so filter them with `grep -a` — plain `grep` may treat the stream as binary and print nothing but `Binary file (standard input) matches`.

### xsdb

xsdb is a command line JavaScript debugger modeled on gdb, working against both devices and the simulator. Its documentation names three intended audiences, the third being "code generators, such as LLMs" — so it is the tool to reach for here rather than adding `trace()` calls and rebuilding.

```shell
cd $MODDABLE/tools/xsbug-log && npm install               # first time only; xsdb is implemented in Node
cd $MODDABLE/examples/piu/balls && mcconfig -dl -m -p sim
```

- **There is no `xsdb` executable.** xsdb *is* [tools/xsbug-log](./tools/xsbug-log), and `mcconfig -dl` launches it for you — you never invoke it by name. `which xsdb` finding nothing means nothing is wrong; do not conclude the debugger is missing from the checkout and fall back to `trace()` and rebuilds.
- **`help` at the `(xsdb)` prompt lists every command**, grouped into control flow, breakpoints, inspection, navigation, settings, and control. The full command surface is discoverable at runtime, so it is not reproduced here.
- **`set output json`** switches to structured JSON output intended to be parsed rather than scraped. `set output text` gives gdb-style output. Prefer JSON when consuming output programmatically.
- **`info instruments` prints the runtime's vital signs** — network sockets, timers, files, chunk and slot memory in use, system bytes free, garbage collections, CPU load. This is a resource canary, and using it is one of the motivations for instrumentation existing: functional tests check answers; instruments check costs. Snapshot it at baseline, after a workload, and again after the system quiesces — any value that does not return to baseline is a finding (a leak or a runaway), even when every assertion passed. A years-latent socket leak was found exactly this way. Instrumented device builds (`-i`) stream the same columns to the serial console once per second as `instruments:` lines; the `instruments key:` line printed at boot names the fields.
- Settings persist per project in a hidden `.xsdb.json` file in the project directory — breakpoints, output mode, break-on-exception, break-at-startup. Behavior can differ between projects for this reason. The file is gitignored.
- `mcrun -dl` works for mods, but not yet on all targets.

Reference: the `xsdb` section of [documentation/tools/tools.md](./documentation/tools/tools.md). The GUI debugger is [documentation/xs/xsbug.md](./documentation/xs/xsbug.md).

## Repository layout

- [**build**](./build): Files required for specific microcontroller targets, the simulator, and make files for build tools in the `tools` directory.
- [**contributed**](./contributed): Unofficial projects and modules. See the caution below.
- [**documentation**](./documentation): All the documentation for the Moddable SDK, in markdown.
- [**examples**](./examples): Example applications. [examples/readme.md](./examples/readme.md) is a guide to building them.
- [**guides**](./guides): Task-oriented how-to pages, newer than most of `documentation`. Each topic directory has an `index.md` giving the manifest include path and module specifier for the topic.
- [**licenses**](./licenses): License agreements, including the Contributor License Agreements.
- [**modules**](./modules): The software modules that make up the runtime — networking, graphics, user interface, hardware access, cryptographic primitives, and device drivers. All have a JavaScript API; many are implemented in part using C.
- [**tests**](./tests): Unit tests, in test262 format. See [Tests](#tests).
- [**tools**](./tools): Tools to build applications, including `mcconfig`, `mcpack`, `mcrun`, and the xsbug debugger.
- [**typings**](./typings): TypeScript declarations for the SDK, published as `@moddable/typings`.
- [**xs**](./xs): The XS JavaScript engine including its compiler and linker, and the test262 execution shell.

## Choosing an API

Several parts of the SDK have been superseded. The older code still works and is still shipped, so it is not obviously stale — but it should not be used as a model for new code, and its documentation is extensive enough to be actively misleading.

**The rule: if an ECMA-419 (`embedded:*`) version of a module exists, it is the preferred one.**

There are roughly three tiers, newest first:

1. **Web standard APIs** — [modules/web](./modules/web) and related: `fetch`, `WebSocket`, streams, `console`, timers, `localStorage`. Prefer these where they cover the task.
2. **ECMA-419** — [modules/io](./modules/io), imported as `embedded:io/*`, `embedded:network/*`, `embedded:storage/*`, and `embedded:provider/*`. These are **not documented in this repository**; the reference is the standard itself, <https://419.ecma-international.org>, plus the relevant [guides](./guides) topic.
3. **Legacy** — the following, which should not be used for new work:

	- [**modules/pins**](./modules/pins) — superseded by `embedded:io/*` (`digital`, `analog`, `i2c`, `smbus`, `spi`, `pwm`, `pulsecount`, `pulsewidth`, `serial`). Treat the whole directory as deprecated.<BR>
	**Exception: audio.** AudioOut is current. It already lives at [modules/io/audioout](./modules/io/audioout) as `embedded:io/audio/out`, with a `pins/` compatibility shim inside it, and [documentation/pins/audioout.md](./documentation/pins/audioout.md) remains the reference.
	- [**modules/files**](./modules/files) — the original `file`, `flash`, and `preference` APIs. Prefer `embedded:storage/files` and `embedded:storage/flash`.
	- [**modules/network**](./modules/network) — the original networking stack. Prefer `embedded:io/socket/*` and `embedded:network/*`, or the web standard tier above.

Existing examples and tests still use the legacy modules, so you will encounter them. The guidance above is about what to write, not a claim that the old modules are broken.

### Preferences is replaced by Key-Value storage

This one is missed constantly, because the two APIs share no vocabulary — nothing in the name "Key-Value store" suggests it supersedes "Preference", so there is no way to make the connection by pattern matching on names.

**The `Preference` module is replaced by the ECMA-419 Key-Value store**, or by Web Storage / `localStorage` if you prefer that wrapper. Web Storage is implemented on top of the Key-Value store.

```js
const kvp = device.keyValue.open({path: "local"});
globalThis.localStorage = new WebStorage(kvp);
```

[guides/key-value/index.md](./guides/key-value/index.md) explains both APIs and how to choose between them. The Web Storage implementation is [examples/io/storage/webstorage](./examples/io/storage/webstorage) with its `manifest_webstorage.json`; typings are in `typings/web/webstorage.d.ts`.

## Documentation that describes legacy APIs

These documents are thorough, well written, and carry recent revision dates. They describe the superseded APIs above. Read [Choosing an API](#choosing-an-api) before following them.

- [documentation/pins/pins.md](./documentation/pins/pins.md) — superseded by `embedded:io/*`. Note that its sibling [documentation/pins/audioout.md](./documentation/pins/audioout.md) is **not** obsolete.
- [documentation/files/files.md](./documentation/files/files.md) — the File, Flash, and Preference sections are legacy. For Preference, see Key-Value storage above.
- [documentation/network/network.md](./documentation/network/network.md) — the original Socket, HTTP, WebSocket, MQTT, DNS, and Wi-Fi stack.

## `contributed` is not a reference

[contributed](./contributed) holds unofficial projects, mostly unmaintained. Do not treat it as a source of current practice. These are the ones worth consulting:

`conversationalAI`, `first-run-moddable-four`, `moddable_six`, `somafm`, `window-display-moddable-one`, `window-display-moddable-three`

Ignore the rest.

## Manifests

**If you are adding or changing anything in a `manifest.json`, read [documentation/tools/manifest.md](./documentation/tools/manifest.md) first.** Manifests look like ordinary configuration files and invite editing by pattern matching on whatever JSON is nearby. They are not: the build's module list, memory configuration, per-platform behavior, and dead-stripping all live here, and mistakes surface later as failures that do not point back at the manifest.

A manifest describes the modules and resources of an application. Its properties are `build`, `include`, `creation`, `defines`, `config`, `strip`, `modules`, `preload`, `resources`, `data`, `platforms`, and `bundle`. Two rules that are not obvious:

- **Every module in the build must be listed in `modules`.** A file that is present on disk but unlisted is simply not in the build.
- **Same-named properties concatenate across includes rather than replacing.** You cannot override a value inherited from an included manifest by redeclaring it.

**To see what a build actually resolved to, read the generated `manifest_flat.json`** in the project's `build/tmp/<platform>/<target>/<build>/<project>/` directory, rather than reasoning about the merge by hand. It is the fully merged manifest — `modules`, `include`, `preload`, `creation`, and the rest — after every include and platform section has been applied, which is the only practical way to answer "is this module actually in the build?" or "where did this value come from?". `mcconfig` writes it even without `-m`, so it is a fast check on device targets where a real build is slow.

Reach for it whenever you change `modules` or `include`, because the build will not tell you: **an `import` of a module that no manifest supplies is not a build error.** It compiles and links cleanly, and fails only at runtime when the import is evaluated. A clean `-t build` is therefore no evidence that a manifest edit was correct — grep the `modules` of `manifest_flat.json` for the paths you expect instead.

See also [documentation/tools/defines.md](./documentation/tools/defines.md) for how `defines` becomes `MODDEF_`-prefixed C `#define`s.

### `creation` — configuring RAM

`creation` sets the allocation parameters of the XS virtual machine: `static`, `chunk`, `heap`, `stack`, `keys`, `parser`, and `main` (the module specifier of the entry module). The inherited defaults work for most projects, including nearly all of the examples. Change them only with reason — bigger values are not always better on a constrained device.

**Know where the value you are looking at comes from.** `creation` is assembled from several manifests, and the memory sizing is usually not in the project's own manifest:

- [examples/manifest_base.json](./examples/manifest_base.json) supplies `keys`, `parser`, and `main`. It does **not** set the memory sizes.
- The memory sizes — `static`, `chunk`, `heap`, `stack` — come from the platform or target manifest under `build/devices/`. Compare [build/devices/pico/manifest.json](./build/devices/pico/manifest.json) (`static: 131072`) with [build/devices/esp32/targets/moddable_six/manifest.json](./build/devices/esp32/targets/moddable_six/manifest.json) (`static: 0` with explicit `chunk` and `heap` pools). Different targets are configured very differently.
- The simulator manifests set no `creation` at all.

The memory sizing a build actually resolved to is in its `manifest_flat.json`, along with everything else the merge produced — see [Manifests](#manifests) above.

- **`static` is the total byte budget for the JavaScript runtime** on a microcontroller — a hard ceiling covering the stack, objects, byte code, and strings. It exists so that a script cannot exhaust the memory the host OS needs.
- **`static` is ignored by the simulator**, which allocates on demand. A project can therefore run cleanly under `-p sim` and fail for memory on a device, with nothing in the symptom pointing back at the manifest. If you only ever test in the simulator, you will not see this.
- Setting `static` to `0` disables the static allocator and switches to split slot and chunk heaps, needed when RAM is discontiguous (ESP32) or when pools have been tuned by hand. It also requires setting `chunk.initial`, `heap.initial`, and both `incremental` values. **Deleting the `static` property is not sufficient**, because an included manifest may still define it.
- `keys.initial` should be small, since most keys are allocated at build time. `keys.incremental: 0` prevents any runtime key allocation.

Full detail is in the `creation` section of [documentation/tools/manifest.md](./documentation/tools/manifest.md), which refers to [Machine allocation](./documentation/xs/XS%20in%20C.md#machine-allocation) in XS in C.

### `preload` — code that runs at build time

`preload` is the other manifest property with consequences that are easy to get wrong, and it has no analogue in Node.js or the browser.

**Modules named in `preload` are executed on your build machine, not on the device.** The XS linker runs the module body at build time, then writes the resulting classes, functions, and objects into ROM, where they are used in place rather than copied to RAM. This is why applications here boot instantly and start with so little RAM used.

Consequences:

- **A preloaded module body cannot call a native function.** Native functions expect the target's environment, possibly a different instruction set, so calling one during preload is a build error. *Defining* a native function during preload is fine — only calling it fails. This is the most common preload failure.
- **A preloaded module body cannot touch hardware**, for the same reason. Typically exactly one module — the one that starts your application — cannot be preloaded for this reason.
- **Preloaded objects are frozen.** The linker automatically freezes classes, functions, prototypes, and built-ins like `Math` and `JSON`. Code that mutates them at runtime still works, via an alias table that clones the object into RAM, but it costs memory and time. Writing code that expects to patch a preloaded object is a design smell here.
- `preload` takes **module names, not paths**, unlike `modules`.

Read [documentation/xs/preload.md](./documentation/xs/preload.md) before adding a module to `preload` — it has a "What Cannot be Preloaded" section listing the restrictions and which built-ins are safe to construct at build time.

## Tests

Tests live in [tests](./tests) — `modules/` mirrors `modules/`, `xs/` covers the engine, and `contributed/`. They are written in test262 format with YAML frontmatter. Reference: [documentation/tools/testing.md](./documentation/tools/testing.md).

**The `flags` line differs by subtree**, so copy it from a neighboring test rather than assuming one. `tests/xs` is almost entirely `flags: [onlyStrict]`, because those are language tests that run as scripts. `tests/modules` is mixed: `[module]` for the SDK module tests, `[onlyStrict]` for the Piu and web platform tests.

Two things to know before trying to run them:

- **Tests require a debug build.** The test runner communicates over the xsbug channel, so release builds cannot run tests.
- **Tests do not run on their own.** They execute inside a test app, which a debugger drives: [tools/test262](./tools/test262) for the language tests in `tests/xs` and for the TEST262 suite, [tools/testmc](./tools/testmc) for the SDK module tests in `tests/modules`. The two are not interchangeable — see below.

### Running tests headlessly

**xsdb is a test runner, so the suite does not need the xsbug GUI.** Build a test app with `-dl` and use `test`. Paths are relative to `$MODDABLE/tests`, so nothing has to be copied into a test262 checkout:

```shell
cd $MODDABLE/tools/test262
mcconfig -dl -m -p sim
(xsdb) test xs/built-ins/Math
xs/built-ins/Math: 6 tests, FAIL 0, PASS 6, SKIP 0 (1.2s)
```

The same commands run the tests on a device — build the app with `-p esp32/<target>` instead. `info tests` lists the failures of the last run, and every run also writes a report `.txt` into the project directory; those are untracked, so delete the ones you create. `set test262 <path>` points at a test262 checkout, whose `harness` then also supplies the `includes:` of the Moddable tests; without it the fallback copies in [tools/test262/harness](./tools/test262/harness) are used. Only the xsbug GUI requires a test262 checkout with `$MODDABLE/tests` copied into it.

Also available headless:

- `xst` runs test262 cases directly on the host, and is by far the fastest way to iterate while writing one. It looks for the harness at `../harness` relative to the working directory, so run it from a `test` directory that has a sibling `harness` — a scratch tree holding a copy of the harness and the cases works, and leaves the checkout alone. See [documentation/xs/xst.md](./documentation/xs/xst.md).
- [tools/test-examples](./tools/test-examples) builds, installs, and launches every project in a tree, passing if nothing throws. It drives execution through xsdb.

### The two test apps are not interchangeable

**[tools/test262](./tools/test262) preloads nothing, while [tools/testmc](./tools/testmc) preloads modules — and preloading freezes the built-ins** (see [`preload`](#preload--code-that-runs-at-build-time)). Under `testmc`, `Object.isFrozen(Math)` is therefore `true` and the properties of `Math` are neither writable nor configurable; under the test262 app they are. A language test that checks property attributes passes in one app and fails in the other with `descriptor should be writable`, which reads as an engine bug rather than as the wrong app. Run `tests/xs` under the test262 app.

## Native code

Much of the runtime is C beneath a JavaScript API — around 250 `.c` files under [modules](./modules) use the `xsmc` macros. **If you are editing C in `modules/` or `xs/`, read [XS in C](./documentation/xs/XS%20in%20C.md) first.**

It is not a conventional C API, and the ways it goes wrong are memory corruption rather than clean errors. In particular the garbage collector may move data, so pointers obtained from the VM must not be held across an allocation, and native data that references JavaScript objects needs the correct mark hooks. Follow the patterns in a neighboring module rather than inventing an approach — [modules/io](./modules/io) is the most current native code in the tree.

## Conventions

- **Indent with tabs**, including in JSON manifests. `.editorconfig` only covers markdown, so this is not enforced by tooling.
- **Every source file carries a license header, and it differs by tree**: LGPL "Moddable SDK Runtime" for `modules/**` and `xs/**`, GPL "Moddable SDK Tools" for `tools/**` and `typings/**`, Creative Commons BY 4.0 for `examples/**`. Copy the header from a neighboring file in the same tree rather than reconstructing it. See [licenses/readme.md](./licenses/readme.md).
- Markdown in `documentation/` opens with an H1, then `Copyright <years> Moddable Tech, Inc.<BR>` and `Revised: <date>`. Pages in `guides/` instead use YAML frontmatter with `name`, `SPDX-FileCopyrightText`, and `updated`.
- [eslint.config.mjs](./eslint.config.mjs) declares the SDK's implicit globals (`trace`, `device`, `screen`, the Piu surface, and others). It marks `System` and `Host` as **deprecated** — do not use them in new code.
- Never commit build output or Node artifacts: `build/bin/`, `build/tmp/`, `node_modules/`, `report.json`. See [.gitignore](./.gitignore).
- A signed Contributor License Agreement is required before a pull request can be accepted. See [contributing.md](./contributing.md).

## Where to look things up

**Start with [guides](./guides).** It is the freshest documentation in the repository — the topic pages carry 2026 dates, while much of `documentation/` is older and some of it predates the APIs it describes.

How the guides are structured, which is not obvious from any one page:

- Each guide is a short, task-focused page with runnable JavaScript examples.
- **A horizontal rule (`---`) separates independent sections**, and each section is a short explanation followed by the code block immediately below it. The prose and its example are meant to be read as a unit — a code block lifted out of its paragraph loses the context that explains it. Note the double duty: the first two `---` in every page delimit the YAML frontmatter, not a section.
- **Most topics document two APIs** — a web standard one (`fetch`, `WebSocket`, `localStorage`) and a lower-level ECMA-419 one (the `Client` and `device.*` variants). Both are current; they are tiers, not old and new. The topic index explains how to choose.
- **Start at the topic's `index.md`.** Beyond listing the guides, it explains how to choose between the available APIs, gives the build setup needed to use them — manifest include paths for `mcconfig`/`mcpack`, and module specifiers where relevant — and ends with a "Learn More" list linking the documentation, the relevant standard, runnable examples, and reference implementations.

**ECMA-419 is not yet documented in this repository.** For the `embedded:*` APIs, use the standard itself: <https://419.ecma-international.org> — a single HTML page covering the whole specification. Read it rather than guessing at an API shape. The guides deep-link into it with anchors (`#http-client-class-pattern`, `#storage-keyvalue`, `#byte-buffer`, and so on), so the relevant guide topic is usually the fastest way to the right part of the spec.

Note that [documentation/io/io.md](./documentation/io/io.md) is *not* a reference for ECMA-419: it is a 2021 introduction written while the IO Class Pattern was still a proposal, and it targets only ESP8266.

- [documentation/readme.md](./documentation/readme.md) — API reference, organized per module. **Incomplete**: it omits `documentation/io/` entirely, along with the board guides and the driver documentation. List the directory rather than trusting this index alone.
- [examples/readme.md](./examples/readme.md) — building and running examples, screen formats, Wi-Fi configuration.
- [documentation/tools/tools.md](./documentation/tools/tools.md) — `mcconfig`, `mcrun`, `mcrez`, `png2bmp`, `xsc`, `xsl`, the simulator, xsdb, and `test-examples`.
- [typings](./typings) — TypeScript declarations. Projects including `examples/manifest_base.json` get them automatically; `mcconfig` compiles TypeScript itself. `mcpack` relies on `npm run build` compiling TypeScript **before** `mcpack` run (see [ts-hello](./examples/packages/ts-hello) for an example).
