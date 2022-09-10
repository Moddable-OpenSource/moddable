/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import tests from "tests";

let re = /(\w+)\s(\w+)/;
let str = 'John Smith';
let newstr = str.replace(re, '$2, $1');
trace(newstr + "\n");

let url = 'http://xxx.domain.com';
trace(/[^.]+/.exec(url)[0].substr(7) + "\n");

re = /\w+\s/g;
str = 'fee fi fo fum';
let myArray = str.match(re);
trace(myArray + "\n");


function execute({ regexp, string, expected }) {
	trace("### /" + regexp.source + "/" + regexp.flags + ".exec(\"" + string + "\") // " + expected + "\n");
	let results = regexp.exec(string);
	let output = "[";
	if (results) {
		let c = results.length, i = 0;
		while (i < c) {
			if (i)
				output += ", ";
			let result = results[i];
			if (result === undefined)
				output += result;
			else
				output += '"' + results[i] + '"';
			i++;
		}
	}
	output += "]";
	if (output != expected)
		trace("# " + output + "\n");
	if (results) {
		let groups = results.groups;
		if (groups) {
			for (let name in groups)
			trace("# groups." + name + ': "' + groups[name] + '"\n');
		}
  }
}

for (let test of tests)
	execute(test);
