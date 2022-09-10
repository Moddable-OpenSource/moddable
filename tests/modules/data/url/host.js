/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

let url;

url = new URL("http://moddable");
assert.sameValue(url.hostname, "moddable");
url = new URL("file://moddable");
assert.sameValue(url.hostname, "moddable");
url = new URL("hello://moddable");
assert.sameValue(url.hostname, "moddable");

// no punycode yet!
// url = new URL("http://Moddàble");
// assert.sameValue(url.hostname, "xn--moddble-cwa");
// url = new URL("file://Moddàble");
// assert.sameValue(url.hostname, "xn--moddble-cwa");
url = new URL("hello://Moddàble");
assert.sameValue(url.hostname, "Modd%C3%A0ble");

url = new URL("http://%4Dodd%61ble");
assert.sameValue(url.hostname, "moddable");
url = new URL("file://%4Dodd%61ble");
assert.sameValue(url.hostname, "moddable");
url = new URL("hello://%4Dodd%61ble");
assert.sameValue(url.hostname, "%4Dodd%61ble");

url = new URL("http://moddable");
url.hostname = "wow";
assert.sameValue(url.hostname, "wow");
url = new URL("file://moddable");
url.hostname = "wow";
assert.sameValue(url.hostname, "wow");
url = new URL("hello://moddable");
url.hostname = "wow";
assert.sameValue(url.hostname, "wow");

url = new URL("http://moddable");
url.hostname = "@@@";
assert.sameValue(url.hostname, "moddable");
url = new URL("file://moddable");
url.hostname = "@@@";
assert.sameValue(url.hostname, "moddable");
url = new URL("hello://moddable");
url.hostname = "@@@";
assert.sameValue(url.hostname, "moddable");

url = new URL("http://moddable");
url.hostname = "oops:123";
assert.sameValue(url.hostname, "moddable");
url = new URL("file://moddable");
url.hostname = "oops:123";
assert.sameValue(url.hostname, "moddable");
url = new URL("hello://moddable");
url.hostname = "oops:123";
assert.sameValue(url.hostname, "moddable");

url = new URL("http://moddable");
url.host = "wow:123";
assert.sameValue(url.hostname, "wow");
url = new URL("file://moddable");
url.host = "wow:123";
assert.sameValue(url.hostname, "moddable");
url = new URL("hello://moddable");
url.host = "wow:123";
assert.sameValue(url.hostname, "wow");
