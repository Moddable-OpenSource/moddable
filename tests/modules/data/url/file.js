/*---
description:
flags: [module]
---*/

import URL from "url";

function test(url, base, success, expected) {
	try {
		let it = base ? new URL(url, new URL(base)) : new URL(url);
		let actual = it.toString();
		if (!success)
  			throw new Test262Error("Expected failure but got «" + actual + "»");
  		success = false;
		assert.sameValue(actual, expected);
  }
  catch(e) {
    if (success)
  		throw new Test262Error("Expected «" + expected + "» but got ", e.name);
  }
}

test("file:foo/bar", "", true, "file:///foo/bar");
test("file:/foo/bar", "", true, "file:///foo/bar");
test("file://foo/bar", "", true, "file://foo/bar");
test("file:///foo/bar", "", true, "file:///foo/bar");

test("file:C:/bar1", "", true, "file:///C:/bar1");
test("file:/C:/bar2", "", true, "file:///C:/bar2");
test("file://C:/bar3", "", true, "file:///C:/bar3");
test("file:///C:/bar4", "", true, "file:///C:/bar4");
test("file:c:/bar1", "", true, "file:///c:/bar1");
test("file:/c:/bar2", "", true, "file:///c:/bar2");
test("file://c:/bar3", "", true, "file:///c:/bar3");
test("file:///c:/bar4", "", true, "file:///c:/bar4");

test("file:///foo/bar", "", true, "file:///foo/bar");
test("..", "file:///C:/demo", true, "file:///C:/");
test("file://loc%61lhost/", "", true, "file:///");
test("file:///C|/demo", "", true, "file:///C:/demo");

test("foo1", "file:///", true, "file:///foo1");
test("/foo2", "file:///", true, "file:///foo2");
test("//foo3", "file:///", true, "file://foo3/");
test("///foo4", "file:///", true, "file:///foo4");

test("foo1", "file://example.com/", true, "file://example.com/foo1");
test("/foo2", "file://example.com/", true, "file://example.com/foo2");
test("//foo", "file://example.com/", true, "file://foo/");
test("///foo", "file://example.com/", true, "file:///foo");
test("////foo", "file://example.com/", true, "file:////foo");

test("file:foo", "file://example.com/", true, "file://example.com/foo");
test("file:/foo", "file://example.com/", true, "file://example.com/foo");
test("file://foo", "file://example.com/", true, "file://foo/");
test("file:///foo", "file://example.com/", true, "file:///foo");
test("file:\\foo", "file://example.com/", true, "file://example.com/foo");
test("file:\\\\foo", "file://example.com/", true, "file://foo/");
test("file:\\\\\\foo", "file://example.com/", true, "file:///foo");
test("file:\\\\\\\\foo", "file://example.com/", true, "file:////foo");
