/*---
description: https://github.com/web-platform-tests/wpt/url/resources/urltestdata.json
flags: [module]
---*/

import { runTests } from "./url_FIXTURE.js";

const tests = [
	"# make sure that relative URL logic works on known typically non-relative schemes too",
	{
		"input": "about:/../",
		"base": "about:blank",
		"href": "about:/",
		"origin": "null",
		"protocol": "about:",
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
		"input": "data:/../",
		"base": "about:blank",
		"href": "data:/",
		"origin": "null",
		"protocol": "data:",
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
		"input": "javascript:/../",
		"base": "about:blank",
		"href": "javascript:/",
		"origin": "null",
		"protocol": "javascript:",
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
		"input": "mailto:/../",
		"base": "about:blank",
		"href": "mailto:/",
		"origin": "null",
		"protocol": "mailto:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	"# unknown schemes and their hosts",
	{
		"input": "sc://ñ.test/",
		"base": "about:blank",
		"href": "sc://%C3%B1.test/",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%C3%B1.test",
		"hostname": "%C3%B1.test",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "sc://%/",
		"base": "about:blank",
		"href": "sc://%/",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%",
		"hostname": "%",
		"port": "",
		"pathname": "/",
		"search": "",
		"hash": ""
	},
	{
		"input": "sc://@/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://te@s:t@/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://:/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "sc://:12/",
		"base": "about:blank",
		"failure": true
	},
	{
		"input": "x",
		"base": "sc://ñ",
		"href": "sc://%C3%B1/x",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "%C3%B1",
		"hostname": "%C3%B1",
		"port": "",
		"pathname": "/x",
		"search": "",
		"hash": ""
	},
	"# unknown schemes and backslashes",
	{
		"input": "sc:\\../",
		"base": "about:blank",
		"href": "sc:\\../",
		"origin": "null",
		"protocol": "sc:",
		"username": "",
		"password": "",
		"host": "",
		"hostname": "",
		"port": "",
		"pathname": "\\../",
		"search": "",
		"hash": ""
	}
];

runTests(tests);
