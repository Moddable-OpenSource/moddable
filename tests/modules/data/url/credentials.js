/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

let url = new URL("http://hello:world@www.moddable.com:80");

assert.sameValue(url.username, "hello");
assert.sameValue(url.password, "world");
assert.sameValue(url.href, "http://hello:world@www.moddable.com/");

url.password = "";

assert.sameValue(url.username, "hello");
assert.sameValue(url.password, "");
assert.sameValue(url.href, "http://hello@www.moddable.com/");

url.username = "";
url.password = "world";

assert.sameValue(url.username, "");
assert.sameValue(url.password, "world");
assert.sameValue(url.href, "http://:world@www.moddable.com/");

url.password = "";

assert.sameValue(url.username, "");
assert.sameValue(url.password, "");
assert.sameValue(url.href, "http://www.moddable.com/");

url = new URL("http://@à:@:@@www.moddable.com:8080/");
assert.sameValue(url.username, "%40%C3%A0");
assert.sameValue(url.password, "%40%3A%40");
assert.sameValue(url.href, "http://%40%C3%A0:%40%3A%40@www.moddable.com:8080/");

url = new URL("http://www.moddable.com/");
url.username = " #/:;<=>?@[\\]^{\}";
url.password = "%%%à";
assert.sameValue(url.href, "http://%20%23%2F%3A%3B%3C%3D%3E%3F%40%5B%5C%5D%5E%7B%7D:%%%%C3%A0@www.moddable.com/");

assert.throws(TypeError, () => new URL("hello://www.moddable.com@"));
