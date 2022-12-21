/*

if (globalThis.URL == undefined) {
	// for xst
	const SCHEME = 1;
	const USERNAME = 2;
	const PASSWORD = 3;
	const HOST = 4;
	const HOSTNAME = 5;
	const PORT = 6;
	const PATH = 7;
	const QUERY = 8;
	const FRAGMENT = 9;
	const ORIGIN = 10;

	class URL {
		#parts;
		#searchParams;
		constructor(href, base) {
			if (base !== undefined)
				this.#parts = parseURL(href, parseURL(base));
			else
				this.#parts = parseURL(href);
			this.#searchParams = new URLSearchParams(this.#parts.query, this.#parts);
		}
		get hash() {
			return serializeURL(this.#parts, FRAGMENT);
		}
		set hash(it) {
			parseURL(it, this.#parts, FRAGMENT);
		}
		get host() {
			return serializeURL(this.#parts, HOST);
		}
		set host(it) {
			parseURL(it, this.#parts, HOST);
		}
		get hostname() {
			return serializeURL(this.#parts, HOSTNAME);
		}
		set hostname(it) {
			parseURL(it, this.#parts, HOSTNAME);
		}
		get href() {
			return serializeURL(this.#parts);
		}
		set href(it) {
			this.#parts = parseURL(it);
			this.#searchParams.updatePairs();
		}
		get origin() {
			return serializeURL(this.#parts, ORIGIN);
		}
		get password() {
			return serializeURL(this.#parts, PASSWORD);
		}
		set password(it) {
			parseURL(it, this.#parts, PASSWORD);
		}
		get pathname() {
			return serializeURL(this.#parts, PATH);
		}
		set pathname(it) {
			parseURL(it, this.#parts, PATH);
		}
		get port() {
			return serializeURL(this.#parts, PORT);
		}
		set port(it) {
			parseURL(it, this.#parts, PORT);
		}
		get protocol() {
			return serializeURL(this.#parts, SCHEME);
		}
		set protocol(it) {
			parseURL(it, this.#parts, SCHEME);
		}
		get search() {
			return serializeURL(this.#parts, QUERY);
		}
		set search(it) {
			parseURL(it, this.#parts, QUERY);
			this.#searchParams.updatePairs();
		}
		get searchParams() {
			return this.#searchParams;
		}
		get username() {
			return serializeURL(this.#parts, USERNAME);
		}
		set username(it) {
			parseURL(it, this.#parts, USERNAME);
		}
		toJSON() {
			return serializeURL(this.#parts);
		}
		toString() {
			return serializeURL(this.#parts);
		}
	}

	// https://url.spec.whatwg.org/#interface-urlsearchparams

	class URLSearchParams {
		#pairs = null;
		#parts = null;
		constructor(it = "", parts = null) {
			if (typeof(it) == "object") {
				if (Array.isArray(it)) {
					for (let item of it) {
						if (item.length != 2)
							throw new TypeError("invalid parameter");
						this.append(item[0], item[1]);
					}
				}
				else {
					for (let name in it)
						this.append(name, it[name]);
				}
			}
			else {
				this.#pairs = parseQuery(it);
				this.#parts = parts;
			}
		}
		get length() {
			return this.#pairs.length;
		}
		append(name, value) {
			this.#pairs.push({ name, value });
			this.updateParts();
		}
		delete(name) {
			this.#pairs = this.#pairs.filter(pair => pair.name != name);
			this.updateParts();
		}
		get(name) {
			const pair = this.#pairs.find(pair => pair.name == name);
			return pair ? pair.value : null;
		}
		getAll(name) {
			return this.#pairs.filter(pair => pair.name == name).map(pair => pair.value);
		}
		has(name) {
			const pair = this.#pairs.find(pair => pair.name == name);
			return pair ? true : false;
		}
		set(name, value) {
			const pair = this.#pairs.find(pair => pair.name == name);
			if (pair)
				pair.value = value;
			else
				this.#pairs.push({ name, value });
			this.updateParts();
		}
		toString() {
			return serializeQuery(this.#pairs);
		}
		updatePairs() {
			if (this.#parts)
				this.#pairs = parseQuery(this.#parts.query);
		}
		updateParts() {
			if (this.#parts)
				this.#parts.query = serializeQuery(this.#pairs);
		}
		[Symbol.iterator]() {
			const pairs = this.#pairs
			let index = 0;
			const iteraror = {
				next() {
					if (index < pairs.length) {
						const pair = pairs[index];
						index++;
						return { value: [pair.name, pair.value], done: false };
					}
					return { value: undefined, done: true };
				}
			}
			return iteraror;
		}
	}

	globalThis.URL = URL;
	globalThis.URLSearchParams = URLSearchParams;
}

if (globalThis.Test262Error === undefined) {
	// for node
	globalThis.Test262Error = class extends Error {};
	globalThis.assert = {
		sameValue(actual, expected) {
			if (actual != expected)
				throw new Test262Error("Expected «" + expected + "» but got «" + actual + "»");
		},
		throws(constructor, f) {
			const expected = constructor.prototype.name;
			try {
				const actual = f();
  				throw new Test262Error("Expected «" + expected + "» but got «" + actual + "»");
			}
			catch (e) {
				const actual = e.name;
				if (actual != expected) 
  					throw new Test262Error("Expected «" + expected + "» but got «" + actual + "»");
			}
		}
	}
	globalThis.print = function(...args) {
		console.log(...args);
	}
}

*/
const keys = [
    "href",
    "protocol",
    "username",
    "password",
    "host",
    "hostname",
    "port",
    "pathname",
    "search",
    "hash",
    "origin",
];

function runTest(test) {
	try {
		const url = new URL(test.input, test.base);
		if (test.failure) {
  			throw new Test262Error("Expected failure but got «" + url.href + "»");
		}
		for (let key of keys) {
			if (key in test) {
				let actual = url[key];
				let expected = test[key];
				assert.sameValue(actual, expected);
			}
		}
	}
	catch(e) {
		if (e.constructor == URIError)
			print(test.input, e.message);
		if (e.constructor == Test262Error)
			throw e;
		if (!test.failure)
			throw new Test262Error("Expected «" + test.href + "» but got «" + e.name + "»");
	}
}

function runTests(tests) {
	for (let test of tests) {
		if (typeof(test) == "object")
			runTest(test);
	}
}

export { runTests };

function test(url, base, success, expected) {
	try {
		let it = base ? new URL(url, base) : new URL(url);
		let actual = it.href;
		if (!success)
  			throw new Test262Error("Expected failure but got «" + actual + "»");
		assert.sameValue(actual, expected);
  }
  catch(e) {
    if (e.constructor == Test262Error)
    	throw e;
    if (success)
  		throw new Test262Error("Expected «" + expected + "» but got ", e.name);
  }
}

export default test;

