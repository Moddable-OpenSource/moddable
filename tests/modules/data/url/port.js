/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

const url = new URL("http://www.moddable.com:80");

assert.sameValue(url.port, "");
assert.sameValue(url.href, "http://www.moddable.com/");

url.port = 443;
assert.sameValue(url.port, "443");
assert.sameValue(url.href, "http://www.moddable.com:443/");

url.protocol = "file";
assert.sameValue(url.port, "443");
assert.sameValue(url.href, "http://www.moddable.com:443/");

url.protocol = "https";
assert.sameValue(url.port, "");
assert.sameValue(url.href, "https://www.moddable.com/");

url.protocol = "file";
assert.sameValue(url.port, "");
assert.sameValue(url.href, "file://www.moddable.com/");

url.port = 443;
assert.sameValue(url.port, "");
assert.sameValue(url.href, "file://www.moddable.com/");
