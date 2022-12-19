/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

let url;

url = new URL("hello:world");
assert.sameValue(url.hash, "");
url.hash = "#fragment0";
assert.sameValue(url.href, "hello:world#fragment0");
url = new URL("#fragment1", url);
assert.sameValue(url.hash, "#fragment1");

url = new URL("file:///");
assert.sameValue(url.hash, "");
url.hash = "#fragment2";
assert.sameValue(url.href, "file:///#fragment2");
url = new URL("#fragment3", url);
assert.sameValue(url.hash, "#fragment3");

url = new URL("http://moddable.com");
assert.sameValue(url.hash, "");
url.hash = "#fragment4";
assert.sameValue(url.href, "http://moddable.com/#fragment4");
url = new URL("#fragment5", "http://moddable.com");
assert.sameValue(url.hash, "#fragment5");

url = new URL("", "http://moddable.com/#fragment6");
assert.sameValue(url.href, "http://moddable.com/");
