/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

let url;

url = new URL("hello:world");
assert.sameValue(url.search, "");
url.search = "?query=0";
assert.sameValue(url.href, "hello:world?query=0");
assert.throws(TypeError, () => new URL("?query=1", url));

url = new URL("file:///");
assert.sameValue(url.search, "");
url.search = "?query=2";
assert.sameValue(url.href, "file:///?query=2");
url = new URL("?query=3", url);
assert.sameValue(url.search, "?query=3");

url = new URL("http://moddable.com");
assert.sameValue(url.search, "");
url.search = "?query=4";
assert.sameValue(url.href, "http://moddable.com/?query=4");
url = new URL("?query=5", "http://moddable.com");
assert.sameValue(url.search, "?query=5");

url = new URL("", "file:///?query=6");
assert.sameValue(url.href, "file:///?query=6");
url = new URL("", "http://moddable.com/?query=7");
assert.sameValue(url.href, "http://moddable.com/?query=7");

url = new URL("?query=6", "file:///");
assert.sameValue(url.href, "file:///?query=6");
url = new URL("?query=7", "http://moddable.com/");
assert.sameValue(url.href, "http://moddable.com/?query=7");
