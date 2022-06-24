# Testing the Moddable SDK
Copyright 2020-2022 Moddable Tech, Inc.<BR>
Revised: June 24, 2022

## Table of Contents

* [Introduction](#intro)
* [Tests](#tests)
	* [JavaScript Language Tests (TEST262)](#javascript-tests)
	* [Moddable SDK Tests](#moddable-tests)
* [test262 Test App](#test262-app)
* [testmc Test App](#testmc-app)
* [Running Tests on the Simulator](#testing-on-simulator)
* [Writing Moddable SDK Tests](#writing-tests)

<a id="intro"></a>
## Introduction
This document explains how to run unit tests implemented in JavaScript in the Moddable SDK. There are three parts to running the tests:

- **Tests**. The unit tests perform tests for success and failure conditions. They are implemented in JavaScript and their source code is stored in `.js` files.
- **Test app**. The test app is responsible for running the unit tests. It executes on the device under test.
- **Test runner**. The test runner is responsible for selecting the tests to run, coordinating with the test app to execute each test, and collecting the test results. The xsbug debugger is the test runner for the Moddable SDK.

The separation of the tests, test app, and test runner allows large test suites to run on resource constrained embedded devices.

> **Note**: The test runner uses the xsbug communication channel to coordinate test execution with the test app. A consequence of this is that release builds cannot be used to run tests.

<a id="tests"></a>
## Tests

The Moddable SDK uses two separate tests suites, one for the JavaScript language and another for the Moddable SDK runtime. The following two section introduce these test suites and how to install them.

<a id="javascript-tests"></a>
### JavaScript Language Tests (TEST262)
TEST262 is a the official suite of conformance tests from TC39, the ECMAScript (JavaScript) language committee. It is defined in the [Ecma-414](https://www.ecma-international.org/publications-and-standards/standards/ecma-414/) standard. These tests are run by all JavaScript engines to ensure compatibility, interoperability, and conformance with the specification.

The TEST262 tests are located in a repository on GitHub.

```
git clone https://github.com/tc39/test262
```

> Note: The `xst` tool may be used to [run test262 tests](../xs/xst.md#test262) on the local computer from the command line. 

<a id="moddable-tests"></a>
### Moddable SDK Tests
The Moddable SDK tests are a suite of tests created by Moddable to validate the correctness and consistency the implementations across different device targets. The tests cover a wide range of functionality including hardware I/O, networking, graphics, data, and more. Because of differences in hardware capabilities and features supported in each port, not all tests will pass in all environments. 

The Moddable SDK tests are located in the Moddable SDK repository at `$MODDABLE/tests`.

The Moddable SDK tests must be copied into test262 `test` directory.

```
cp -r $MODDABLE/tests <path-to-test262-repository>/test/moddable
```

> **Note**: To run the Moddable SDK tests you must install the TEST262 tests, even if you do not intend to run the TEST262 tests.

<a id="test262-app"></a>
## test262 Test App

The test262 test app is a Moddable app to run the tests in test262. It includes an implementation of the TEST262 test harness required by its tests. The test262 app is located in `$MODDABLE/tools/test262`.

> **Note**: The test262 test app requires a total of about 128 KB of free memory to run. It cannot run on devices with less free memory, such as the ESP8266.

### Build `test262` Test App
Build the `test262` app with the `-d` debug option as you would any other Moddable app.

```
cd $MODDABLE/tools/test262
mcconfig -d -m -p esp32
```

> Note: These instructions assume that the environment is set up to build for your target device.

> Note: The examples in this document are for the `esp32` platform. To build for other platforms, use the corresponding platform identifier.

### Configure xsbug for Tests

In `xsbug`, select the `Preferences` menu.

Turn off the `On Exceptions` slider and turn on the `Test` tab. See the circled items in the image below:

<img src="../assets/tools/testing/test-xsbug-prefs.png" width=794>

The TEST tab will appear at the top of the window.

> Note: Disabling "Break: On exception" is necessary because many tests intentionally trigger exceptions.

### Locate Tests

The first time TEST262 is run, you need to configure xsbug with the location of the test repository.

<img src="../assets/tools/testing/test-xsbug-locate.png" width=340>

Select the `REPOSITORY...` item and a file selection dialog appears. Select the directory `test262` of the repository that you cloned from GitHub.

The `REPOSITORY...` item is replaced with a list of the test categories.

<img src="../assets/tools/testing/test-categories.png" width=335>

### Select Tests

Clicking to the left of a category or individual test selects that test or category to run. The bullet indicates the tests selected to run.

<img src="../assets/tools/testing/test-select.png" width=344>

You may also use the `SELECT` text field to type in the name of the test(s) that you would like to run. 

### Run Tests

Click the "Play" button to start the tests:

<img src="../assets/tools/testing/test-run.png" width=344>

The "Play" button turns into a "Stop" button and the number of tests run and results update as the tests run.

<img src="../assets/tools/testing/test-running.png" width=340>

Press the "Stop" button to stop the tests.

While the tests run, the `REPORT` item displays the currently running test.

> **Note**: Most test262 tests run very quickly but some take a very long time.

### View Results

When the run of tests is complete, the "Stop" button changes back to a "Play" button.

<img src="../assets/tools/testing/test-results.png" width=1179>

Next to the "Play" button is a status line with the number of failed, passed, and skipped tests.

The `REPORT` item can be opened to view the failures. The `LOG` pane contains output including errors and exceptions.

> **Note**: The tests intentionally create failure conditions which generate exceptions that appear in the log. An exception does not necessarily indicate a test failure. The tests operate with valid and invalid inputs to verify both success conditions and failure conditions.

If you hover the mouse pointer over the `REPORT` item, an icon appears. Clicking on this icon puts the results into a text file that you can save.

<img src="../assets/tools/testing/test-report.png" width=372>

<a id="testmc-app"></a>
## testmc Test App

The `testmc` test app is a Moddable app to run tests specific to the Moddable SDK. It is similar to the `test262` app and operates in the same way. However, `testmc` is configured for validation of Moddable SDK modules rather than the JavaScript language.

The `testmc` app is located in `$MODDABLE/tools/testmc`.

> **Important**: You must use the `test262` app to run the TEST262 tests. You must use the `testmc` app to run the Moddable SDK tests.

### Build `testmc` Test App

Similar to other Moddable apps, build the `testmc` app.

```
cd $MODDABLE/tools/testmc
mcconfig -d -m -p esp32
```

To run network tests, you need to include the `ssid` and `password` on the `mcconfig` line:

```
cd $MODDABLE/tools/testmc
mcconfig -d -m -p esp32 ssid=<ssid> password=<password>
```
> Note: To run tests that require a screen, use include the subplatform identifier when building (e.g. `esp32/moddable_two`, not just `esp32`) so the screen driver is included.

### Run Tests

The tests appear as a `moddable` category in the Test list.

<img src="../assets/tools/testing/test-moddable-category.png" width=365>

Once the testmc test app is running and the tests are in place, selecting tests, running tests, and viewing results is done in the same way as test262 tests.

<a id="testing-on-simulator"></a>
## Running Tests on the Simulator
The test262 and testmc test apps may also be run on the mcsim, the Moddable SDK's simulator. This is useful for verifying that the simulator is providing consistent results with the target device and for developing tests. Of course, tests which depend on features the simulator does not support, such as hardware I/O, will fail.

The process of running on the simulator is the same in xsbug. When the simulator launches, there is one change that must be made to the simulator options.  Set the "Reload on Abort" switch to "on". If this is not set, testing will stall after the first test is run.

<img src="../assets/tools/testing/test-simulator.png" width=300>

<a id="writing-tests"></a>
## Writing Moddable SDK Tests

The Moddable SDK tests are written in the same way as TEST262 tests. They require the same front-matter, for example, and use the same basic `assert` facility. Because the Moddable SDK is entirely module based, Moddable SDK tests only run in strict mode. Many of the tests run as modules, to be able to import other modules.

The testmc test app adds features to the runtime environment to make it easier to write test scripts. This is similar in concept to `$TEST262` global variable added by the TEST262 test harness. The following sections introduce the runtime features of testmc for tests.

### `$TESTMC` Global

The `$TESTMC` global is an object with properties for many different purposes.

#### `timeout(ms [, message])`
The `timeout` function is used to cause a test to fail if it does not complete within the specified period of time. The `timeout` function only works with asynchronous tests (those marked with `async` in their front-matter).

```js
$TESTMC.timeout(5000, "dns lookup timeout");
```

#### `HostObject`, `HostObjectChunk`, `HostBuffer`
These constructors are used to create host objects to pass as arguments to functions being tested. 

```js
new $TESTMC.HostObject // host object with pointer (-1) for storage
new $TESTMC.HostObjectChunk // host object with 16 byte chunk for storage
new $TESTMC.HostBuffer(count) // host buffer of count bytes
```
See the [XS in C](../xs/XS%20in%20C.md) document for additional information on host objects.

#### `config`
The `config` property is a shortcut to the `mc/config` module's default export. It contains configuration properties for some kinds of tests (details below). This is also convenient for test scripts running as a program rather than a module.

```js
let ssid = $TESTMC.config.ssid;
```

#### `wifiInvalidConnectionTimeout`, `wifiConnectionTimeout`, `wifiScanTimeout`
These constants are used to set timeouts for Wi-Fi tests. They are provided as values on `$TESTMC` because the optimal value varies by target device and network conditions such as signal strength.

```js
$TESTMC.timeout($TESTMC.wifiScanTimeout, "wi-fi scan timeout");
```

#### `Behavior`
The `$TESTMC.Behavior` constructor extends the Piu `Behavior` class for testing. It wraps each handler in a `try`/`catch` block. If the wrapper catches an exception, it terminates the test as failed. This is convenient for writing tests for Piu events.

```js
class SampleBehavior extends $TESTMC.Behavior {
	onTouchEnded(content, id, x, y, ticks) {
		assert.sameValue(x, 10);
		assert.sameValue(y, 20);
	}
}
```

### `$NETWORK` Global
The `$NETWORK` global provides functions and values useful for testing operations that use the network.

#### `connected`
The `connected` property returns a Promise that resolves when a network connection is available. If the network is already connected (such as when running tests in the simulator), the promise resolves immediately.

Because the resolution of the promise is asynchronous, the `connected` property should only be used in tests marked `async`.

```js
await $NETWORK.connected;
```

#### `invalidDomain`
The `invalidDomain` property is a string containing a DNS hostname guaranteed to fail to resolve. This is useful for testing error handling of network code.

#### `wifi()`
The `wifi` function returns a promise which resolves to an object with the options object to establish a Wi-Fi connection for testing.

```js
let options = await $NETWORK.wifi();
new WiFi(options);
```

Note that most network tests can use `$NETWORK.connected` to establish a connection. The `$NETWORK.wifi` function is provided to test the APIs used to establish a Wi-Fi connection.

### `screen` Global
The `screen` global is a standard part of the Moddable SDK runtime environment on devices with a display. It is used by both Poco and Piu to access the display driver. The `testmc` test app adds wraps the device's `screen` global to provide additional capabilities.

##### `checksum`
The `checksum` property is a hash of the last update drawn to the screen. The value is a 32 character hexadecimal string.

```js
render.begin();
	render.fillRectangle(black, 0, 0, render.width, render.height);
	render.blendRectangle(white, 32, 10, 10, 20, 20);
render.end();

assert.sameValue("e5386bfa56b0ab7128c75199547c2178", screen.checksum, "mismatch");
```

The checksum is for the pixels sent to the display driver by the last update. It is only a checksum of the entire screen if all pixels were drawn in a single update. The checksum changes if the pixel format or software rotation changes.

#### `checkImage(checksum)`
The `checkImage` method uses `screen.checksum` to verify the correctness of rendering with Piu. If the checksum does not match, the test is terminated as failed.

```js
one.height = 50;
one.width = 50;
screen.checkImage("f1dd85eb20df5f900845e1ae8b79aa25");

one.height = 150;
one.width = 150;
screen.checkImage("4e008cd76aa8c80e480d966a8aa91228");
```

#### `doTouchBegan()`, `doTouchMoved()`, `doTouchEnded()`
These functions send touch input events as if they were generated by the touch screen driver. The arguments to each function are `id, x, y, ticks`. These `async` functions resolve once the touch event has been delivered.

```js
await screen.doTouchBegan(0, 100, 100, Time.ticks);
```
#### Test Configuration in `mc/config`
The testmc manifest contains configuration values for on-device testing, such as hardware pin numbers and I/O ports. These values are used by tests, so that they may written to be independent of the device being tested. When running testmc on a new device, the configuration values for the device must be added to the manifest. 

- `config.digital[]`

An array of ECMA-419 digital pin specifiers. 

- `config.i2c`

An object with properties for the `hz` and `address` of a device connected to the default I²C  bus to use for tests. Also has `unusedAddress` for an address that is guaranteed not to have a device connected.

- `config.pwm`

An object with properties for a PWM testing. `from` is `true` if the host implementation supports the `from` property in the constructor options object. `pins` is an array of ECMA-419 pin specifiers that may be used with PWM. `ports` is an array of ECMA-419 port specifiers that may be used with the PWM pins. `invalidPorts` is an array of invalid ECMA-419 port specifiers. `resolutions` is an array of supported PWM resolutions. `hzs` is an array of supported PWM frequencies.

- `config.spi`

An object with properties for SPI testing. `select` is the ECMA-419 pin specifier for the select pin. `ports` is an array of ECMA-419 port specifiers for SPI. `invalidPorts` is an array of invalid SPI port specifiers. `hzs` is an array of support SPI frequencies.

- `config.invalidPins`

An array of ECMA-419 pin specifiers that are invalid for the target device. 

- `config.flashParition`

The name of the flash partition to use for tests. The content of this partition will be destroyed by the tests.
