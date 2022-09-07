/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	{
		"input": "file://C|/",
		"base": "about:blank",
		"href": "file:///C:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/C:/",
		"search": "",
		"hash": ""
	},
	"# file URLs without base URL by Rimas Misevičius",
	{
		"input": "file:",
		"base": "about:blank",
		"href": "file:///",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:?q=v",
		"base": "about:blank",
		"href": "file:///?q=v",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/",
		"search": "?q=v",
		"hash": ""
	},
	{
		"input": "file:#frag",
		"base": "about:blank",
		"href": "file:///#frag",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": "#frag"
	},
	"# file: drive letter cases from https://crbug.com/1078698",
	{
		"input": "file:///Y:",
		"base": "about:blank",
		"href": "file:///Y:",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/Y:",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///Y:/",
		"base": "about:blank",
		"href": "file:///Y:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/Y:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///./Y",
		"base": "about:blank",
		"href": "file:///Y",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/Y",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///./Y:",
		"base": "about:blank",
		"href": "file:///Y:",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/Y:",
		"search": "",
		"hash": ""
	},
	{
		"input": "\\\\\\.\\Y:",
		"base": "about:blank",
		"failure": true,
		"inputCanBeRelative": true
	},
	"# file: drive letter cases from https://crbug.com/1078698 but lowercased",
	{
		"input": "file:///y:",
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
		"input": "file:///y:/",
		"base": "about:blank",
		"href": "file:///y:/",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/y:/",
		"search": "",
		"hash": ""
	},
	{
		"input": "file:///./y",
		"base": "about:blank",
		"href": "file:///y",
		"protocol": "file:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/y",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
