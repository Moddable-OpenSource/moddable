/*---
description:
flags: [module]
---*/

import test from "./url_FIXTURE.js";

test("file:foo/bar0", undefined, true, "file:///foo/bar0");
test("file:/foo/bar1", undefined, true, "file:///foo/bar1");
test("file://foo/bar2", undefined, true, "file://foo/bar2");
test("file:///foo/bar3", undefined, true, "file:///foo/bar3");
test("file:////foo/bar4", undefined, true, "file:////foo/bar4");

test("file:C:/bar0", undefined, true, "file:///C:/bar0");
test("file:/C:/bar1", undefined, true, "file:///C:/bar1");
test("file://C:/bar2", undefined, true, "file:///C:/bar2");
test("file:///C:/bar3", undefined, true, "file:///C:/bar3");
test("file:////C:/bar4", undefined, true, "file:////C:/bar4");

test("file:c:/bar0", undefined, true, "file:///c:/bar0");
test("file:/c:/bar1", undefined, true, "file:///c:/bar1");
test("file://c:/bar2", undefined, true, "file:///c:/bar2");
test("file:///c:/bar3", undefined, true, "file:///c:/bar3");
test("file:////c:/bar4", undefined, true, "file:////c:/bar4");


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

test("..", "file:///C:/demo", true, "file:///C:/");
test("file://loc%61lhost/", undefined, true, "file:///");
test("file:///C|/demo", undefined, true, "file:///C:/demo");
