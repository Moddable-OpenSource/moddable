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

let urlRegExp = null;
function parseURL(url) {
	if (!urlRegExp)
		urlRegExp = new RegExp("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
	const matches = url.toString().match(urlRegExp);
	return {
		scheme:matches[2],
		authority:matches[4],
		path:matches[5],
		query:matches[7],
		fragment:matches[9],
	}
}

const specialSchemes = {
	ftp: 21,
	file: null,
	http: 80,
	https: 443,
	ws: 80,
	wss: 443,
}

class URL {
	#scheme = null;
	#username = null;
	#password = null;
	#hostname = null;
	#port = null;
	#path = null;
	#searchParams = null;
	#fragment = null;
	
	#parseAuthority(authority) {
		const at = authority.indexOf("@");
		if (at >= 0) {
			const credentials = authority.slice(0, at);
			authority = authority.slice(at + 1);
			const colon = credentials.indexOf(":");
			if (colon >= 0) {
				this.username = credentials.slice(0, colon);
				this.password = credentials.slice(colon + 1);
			}
		}
		this.host = authority;
	}
	constructor(href, base) {
		const baseParts = base ? parseURL(base.toString()) : {};
		const parts = parseURL(href.toString())
		if (!parts.scheme && baseParts) {
			parts.scheme = baseParts.scheme;
			if (!parts.authority) {
				parts.authority = baseParts.authority;
				if (!parts.path) {
					parts.path = baseParts.path;
					if (!parts.query)
						parts.query = baseParts.query;
				}
				else if (parts.path[0] != '/') {
					let slash = baseParts.path.lastIndexOf('/');
					if (slash >= 0)
						baseParts.path = baseParts.path.slice(0, slash + 1);
					else
						baseParts.path = '/';
					parts.path = baseParts.path + parts.path;
				}
			}
		}
		if (parts.scheme)
			this.#scheme = parts.scheme;
		else
			throw new URIError("invalid protocol");
		if (parts.authority)
			this.#parseAuthority(parts.authority);
		if (parts.path)
			this.pathname = parts.path;
		if (parts.query)
			this.search = parts.query;
		if (parts.fragment)
			this.hash = parts.fragment;
	}
	get hash() {
		const fragment = this.#fragment;
		return (fragment) ? "#" + fragment : "";
	}
	set hash(it) {
		it = it.toString();
		if (it.length == 0)
			this.#fragment = null;
		else {
			if (it[0] == "#")
				it = it.slice(1);
			this.#fragment = encodeURIComponent(decodeURIComponent(it));
		}
	}
	get host() {
		let result = "";
		const hostname = this.#hostname;
		if (hostname) {
			result += hostname;
			const port = this.#port;
			if (port) {
				result += ":" + port;
			}
		}
		return result;
	}
	set host(it) {
		it = it.toString();
		if (it.length == 0)
			this.#hostname = null;
		else {
			const colon = it.indexOf(":");
			if (colon >= 0) {
				this.hostname = it.slice(0, colon);
				this.port = it.slice(colon + 1);
			}
			else
				this.hostname = it;
		}
	}
	get hostname() {
		return this.#hostname ?? "";
	}
	set hostname(it) {
		it = it.toString();
		if (it.length == 0)
			this.#hostname = null;
		else
			this.#hostname = it; //@@ 
	}
	get href() {
		let result = this.#scheme + ":";
		if (this.#hostname) {
			result += "//";
			if (this.#username) {
				result += this.#username;
				if (this.#password)
					result += ":" + this.#password;
				result += "@";
			}
			result += this.host;
		}
		result += this.pathname;
		result += this.search;
		result += this.hash;
		return result;
	}
	set href(it) {
		const parts = parseURL(it);
		this.protocol = parts.scheme;
		if (parts.authority)
			this.#parseAuthority(parts.authority);
		if (parts.path)
			this.pathname = parts.path;
		if (parts.query)
			this.search = parts.query;
		if (parts.fragment)
			this.hash = parts.fragment;
	}
	get password() {
		return this.#password ?? "";
	}
	set password(it) {
		it = it.toString();
		if (it.length == 0)
			this.#password = null;
		else
			this.#password = encodeURIComponent(decodeURIComponent(it));
	}
	get pathname() {
		return this.#path ?? "";
	}
	set pathname(it) {
		it = it.toString();
		if (it.length == 0)
			this.#path = null;
		else {
			if (it[0] == "/")
				it = it.slice(1);
			const names = it.split("/");
			let c = names.length, i = 0;
			while (i < c) {
				const name = names[i];
				if (name == ".") {
					names.splice(i, 1);
					c--;
				}
				else if (name == "..") {
					if (i > 0) {
						i--;
						names.splice(i, 2);
						c -= 2;
					}
					else
						throw new URIError("invalid url");
				}
				else {
					names[i] = encodeURIComponent(decodeURIComponent(name));
					i++;
				}
			}
			this.#path = "/" + names.join("/");
		}
	}
	get port() {
		return this.#port ?? "";
	}
	set port(it) {
		it = it.toString();
		if (it.length == 0)
			this.#port = null;
		else {
			this.#port = (specialSchemes[this.#scheme] == it) ? null : it;
		}
	}
	get protocol() {
		return this.#scheme + ":";
	}
	set protocol(it) {
		it = it.toString();
		if (it.length == 0)
			throw new URIError("invalid protocol");
		this.#scheme = it.toLowerCase();
	}
	get search() {
		let searchParams = this.#searchParams;
		let result = "";
		if (searchParams && (searchParams.size > 0)) {
			result += "?" + searchParams;
		}
		return result;
	}
	set search(it) {
		let searchParams = this.#searchParams;
		it = it.toString();
		if (it.length == 0) {
			if (searchParams)
				searchParams.clear();
		}
		else {
			if (it[0] == "?")
				it = it.slice(1);
			if (searchParams)
				searchParams.clear();
			else
				searchParams = this.#searchParams = new URLSearchParams();
			const items = it.split("&");
			items.forEach(item => {
				item = item.split("=");
				if (item.length == 2)
					searchParams.set(decodeURIComponent(item[0]), decodeURIComponent(item[1]));
				else
					searchParams.set(decodeURIComponent(item), "");
			});
		}
	}
	get searchParams() {
		let searchParams = this.#searchParams;
		if (!searchParams)
			searchParams = this.#searchParams = new URLSearchParams();
		return searchParams;
	}
	get username() {
		return this.#username ?? "";
	}
	set username(it) {
		it = it.toString();
		if (it.length == 0)
			this.#username = null;
		else
			this.#username = encodeURIComponent(decodeURIComponent(it));
	}
	toString() {
		return this.href;
	}
}

// https://url.spec.whatwg.org/#interface-urlsearchparams

class URLSearchParams {
	#pairs = [];
	constructor(it) {
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
			it = it.toString();
			const items = it.split("&");
			items.forEach(item => {
				item = item.split("=");
				if (item.length == 2)
					this.append(decodeURIComponent(item[0]), decodeURIComponent(item[1]));
				else
					this.append(decodeURIComponent(item), "");
			});
		}
	}
	append(name, value) {
		this.#pairs.push({ name, value });
	}
	delete(name) {
		this.#pairs = this.#pairs.filter(pair => pair.name != name);
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
			this.#pairs.push = { name, value };
	}
	toString() {
		return this.#pairs.map(pair => encodeURIComponent(pair.name) + "=" + encodeURIComponent(pair.value)).join("&");
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

