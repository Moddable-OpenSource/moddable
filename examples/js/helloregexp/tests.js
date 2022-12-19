
const tests = [];

function prepare(pattern, flags, string, expected) {
	tests.push({
		regexp: new RegExp(pattern, flags),
		string,
		expected
	});
}

prepare("a|ab", "", "abc", '["a"]');
prepare("((a)|(ab))((c)|(bc))", "", "abc", '["abc", "a", "a", undefined, "bc", undefined, "bc"]');
prepare("a[a-z]{2,4}", "", "abcdefghi", '["abcde"]');
prepare("a[a-z]{2,4}?", "", "abcdefghi", '["abc"]');
prepare("(aa|aabaac|ba|b|c)*", "", "aabaac", '["aaba", "ba"]');
prepare("(z)((a+)?(b+)?(c))*", "", "zaacbbbcac", '["zaacbbbcac", "z", "ac", "a", undefined, "c"]');
prepare("(a*)*", "", "b", '["", ""]');
prepare("(a*)b\\1+", "", "baaaac", '["b", ""]');
prepare("(?=(a+))", "", "baaabac", '["", "aaa"]');
prepare("(?=(a+))a*b\\1", "", "baaabac", '["aba", "a"]');
prepare("(.*?)a(?!(a+)b\\2c)\\2(.*)", "", "baaabaac", '["baaabaac", "ba", undefined, "abaac"]');
prepare("(?<=\\$)\\d+(\\.\\d*)?", "", "$10.53", '["10.53", ".53"]');
prepare("(?<!\\$)\\d+(?:\\.\\d*)", "", "x10.53", '["10.53"]');
prepare("(?<=\\$\\d+\\.)\\d+", "", "$10.53", '["53"]');
prepare("(?<=(\\d+)(\\d+))$", "", "1053", '["", "1", "053"]');
prepare("^(\\d+)(\\d+)", "", "1053", '["1053", "105", "3"]');
prepare("(?<=\\1(.))bcd", "", "aabcd", '["bcd", "a"]');
prepare("(?<year>\\d{4})-(?<month>\\d{2})-(?<day>\\d{2})", "u", "2015-01-02", '["2015-01-02", "2015", "01", "02"]');
prepare("^(?<half>.*).\\k<half>$", "u", "a*a", '["a*a", "a"]');
prepare("^(?<half>.*).\\k<half>$", "u", "a*b", '[]');
prepare("^(?<part>.*).\\k<part>.\\1$", "", "a*a*a", '["a*a*a", "a"]');
prepare("^(?<part>.*).\\k<part>.\\1$", "", "a*a*b", '[]');
prepare("\\k<part>", "", "k<part>", '["k<part>"]');

Object.freeze(tests, true);
export default tests;