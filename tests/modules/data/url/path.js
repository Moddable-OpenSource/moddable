/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

let url;

url = new URL("hello:world/0");
assert.sameValue(url.pathname, "world/0");
url.pathname = "path/0";
assert.sameValue(url.pathname, "world/0");

url = new URL("hello:/world/1");
assert.sameValue(url.pathname, "/world/1");
url.pathname = "path/1";
assert.sameValue(url.pathname, "/path/1");

url = new URL("hello://world/2");
assert.sameValue(url.pathname, "/2");
url.pathname = "path/2";
assert.sameValue(url.pathname, "/path/2");

url = new URL("hello:///world/3");
assert.sameValue(url.pathname, "/world/3");
url.pathname = "path/3";
assert.sameValue(url.pathname, "/path/3");

url = new URL("hello:////world/4");
assert.sameValue(url.pathname, "//world/4");
url.pathname = "path/4";
assert.sameValue(url.pathname, "/path/4");

url = new URL("http:moddable.com/0");
assert.sameValue(url.pathname, "/0");
url.pathname = "path/0";
assert.sameValue(url.pathname, "/path/0");

url = new URL("http:/moddable.com/1");
assert.sameValue(url.pathname, "/1");
url.pathname = "path/1";
assert.sameValue(url.pathname, "/path/1");

url = new URL("http://moddable.com/2");
assert.sameValue(url.pathname, "/2");
url.pathname = "path/2";
assert.sameValue(url.pathname, "/path/2");

url = new URL("http:///moddable.com/3");
assert.sameValue(url.pathname, "/3");
url.pathname = "path/3";
assert.sameValue(url.pathname, "/path/3");

url = new URL("http:////moddable.com/4");
assert.sameValue(url.pathname, "/4");
url.pathname = "path/4";
assert.sameValue(url.pathname, "/path/4");


url = new URL("http:////moddable.com/");
url.pathname = "./path/5";
assert.sameValue(url.pathname, "/path/5");

url = new URL("http:////moddable.com/");
url.pathname = "../path/6";
assert.sameValue(url.pathname, "/path/6");

url = new URL("http:////moddable.com/");
url.pathname = "../../../path/7";
assert.sameValue(url.pathname, "/path/7");
