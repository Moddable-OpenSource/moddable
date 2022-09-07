/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "file:///./y:",
		"base": "about:blank",
		"href": "file:///y:",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/y:",
		"search": "",
		"hash": ""
	},
	{
		"input": "\\\\\\.\\y:",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	"# Additional file URL tests for (https://github.com/whatwg/url/issues/405)",
	{
		"input": "file://localhost//a//../..//foo",
		"base": "about:blank",
		"href": "file://///foo",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "///foo",
		"search": "",
		"hash": ""
	},
	{
		"input": "file://localhost////foo",
		"base": "about:blank",
		"href": "file://////foo",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "////foo",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:////foo",
		"base": "about:blank",
		"href": "file:////foo",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//foo",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///one/two",
		"base": "file:///",
		"href": "file:///one/two",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/one/two",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:////one/two",
		"base": "file:///",
		"href": "file:////one/two",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//one/two",
		"search": "",
		"hash": ""
	},
	{
		"input": "//one/two",
		"base": "file:///",
		"href": "file://one/two",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "one",
		"hostname": "one",
		"port": "",
		"pathname": "/two",
		"search": "",
		"hash": ""
	},
	{
		"input": "///one/two",
		"base": "file:///",
		"href": "file:///one/two",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/one/two",
		"search": "",
		"hash": ""
	},
	{
		"input": "////one/two",
		"base": "file:///",
		"href": "file:////one/two",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//one/two",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///.//",
		"base": "file:////",
		"href": "file:////",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//",
		"search": "",
		"hash": ""
	},
	"File URL tests for https://github.com/whatwg/url/issues/549",
	{
		"input": "file:.//p",
		"base": "about:blank",
		"href": "file:////p",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//p",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:/.//p",
		"base": "about:blank",
		"href": "file:////p",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "//p",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
