/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// https://url.spec.whatwg.org/#url-class

function parseURL(url) @ "fx_parseURL"
function serializeURL(url) @ "fx_serializeURL"
function parseQuery(query) @ "fx_parseQuery"
function serializeQuery(pairs) @ "fx_serializeQuery"

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
			this.#pairs = [];
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

export { URL, URLSearchParams }
export default URL;

