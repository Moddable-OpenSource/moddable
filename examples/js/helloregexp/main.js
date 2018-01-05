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


function execute(pattern, flags, string, expected) {
	trace("### /" + pattern + "/" + flags + ".exec(\"" + string + "\") // " + expected + "\n");
	let regexp = new RegExp(pattern, flags);
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
}

execute("a|ab", "", "abc", '["a"]');
execute("((a)|(ab))((c)|(bc))", "", "abc", '["abc", "a", "a", undefined, "bc", undefined, "bc"]');
execute("a[a-z]{2,4}", "", "abcdefghi", '["abcde"]');
execute("a[a-z]{2,4}?", "", "abcdefghi", '["abc"]');
execute("(aa|aabaac|ba|b|c)*", "", "aabaac", '["aaba", "ba"]');
execute("(z)((a+)?(b+)?(c))*", "", "zaacbbbcac", '["zaacbbbcac", "z", "ac", "a", undefined, "c"]');
execute("(a*)*", "", "b", '["", ""]');
execute("(a*)b\\1+", "", "baaaac", '["b", ""]');
execute("(?=(a+))", "", "baaabac", '["", "aaa"]');
execute("(?=(a+))a*b\\1", "", "baaabac", '["aba", "a"]');
execute("(.*?)a(?!(a+)b\\2c)\\2(.*)", "", "baaabaac", '["baaabaac", "ba", undefined, "abaac"]');
execute("(?<=\\$)\\d+(\\.\\d*)?", "", "$10.53", '["10.53", ".53"]');
execute("(?<!\\$)\\d+(?:\\.\\d*)", "", "x10.53", '["10.53"]');
execute("(?<=\\$\\d+\\.)\\d+", "", "$10.53", '["53"]');
execute("(?<=(\\d+)(\\d+))$", "", "1053", '["", "1", "053"]');
execute("^(\\d+)(\\d+)", "", "1053", '["1053", "105", "3"]');
execute("(?<=\\1(.))bcd", "", "aabcd", '["bcd", "a"]');
