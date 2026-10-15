/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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

/*
	WebDAV route for the HTTP Server (RFC 4918)

	Serves an ECMA-419 Files directory over WebDAV. Compose with other routes
	through the HTTP Server's onRoute:

		const dav = new WebDAV({directory, prefix: "/dav"});
		onRoute(request) {
			return dav.onRoute(request) ?? otherRoutes(request);
		}

	Notes:
	 - LOCK and UNLOCK are advertised (DAV class 2) so read-write clients like
	   macOS Finder and Windows Explorer can mount, but the locks are not
	   enforced. This server is not safe for concurrent writers.
	 - COPY of a collection is not implemented (501). MOVE, MKCOL, and DELETE
	   support collections.
	 - PROPPATCH accepts and ignores property changes, as the file system
	   stores no properties. creationdate and getlastmodified are unavailable.
	 - PROPFIND supports Depth 0 and 1; Depth infinity is refused (403).
	 - GET of a file supports a single byte range (206, 416, Accept-Ranges).
	   macOS WebDAVFS reads files in ranged windows when the server allows.
*/

import {XML} from "xml";

const ALLOW = "OPTIONS, GET, HEAD, PUT, DELETE, COPY, MOVE, MKCOL, PROPFIND, PROPPATCH, LOCK, UNLOCK";
const COPY_BUFFER = 1024;
const TEMP_PREFIX = ".davtmp";		// in-flight uploads, at the share root; hidden from listings

const CONTENT_TYPES = `
bin application/octet-stream
js application/js
json application/json
pdf application/pdf
xml application/xml
zip application/zip
bmp image/bmp
gif image/gif
jpeg image/jpeg
jpg image/jpeg
png image/png
css text/css; charset=utf-8
html text/html; charset=utf-8
md text/markdown; charset=utf-8
txt text/plain; charset=utf-8
`;

function getContentType(path) {
	const dot = path.lastIndexOf(".");
	if (dot >= 0) {
		const extension = path.slice(dot + 1).toLowerCase();
		const index = CONTENT_TYPES.indexOf(`\n${extension} `);
		if (index >= 0)
			return CONTENT_TYPES.slice(index + extension.length + 2, CONTENT_TYPES.indexOf("\n", index + 1));
	}
	return "application/octet-stream";
}

// item is undefined (absent), or {size, isDirectory} ("" is the share root)
function statusOf(dav, path) {
	if ("" === path)
		return {isDirectory: true};
	const status = dav.directory.status(path);
	if (status.isFile())
		return {size: status.size};
	if (status.isDirectory())
		return {isDirectory: true};
}

function parentOf(path) {
	const slash = path.lastIndexOf("/");
	return (slash < 0) ? "" : path.slice(0, slash);
}

function nameOf(path) {
	return path.slice(path.lastIndexOf("/") + 1);
}

function encodePath(dav, path, isDirectory) {
	let href = dav.prefix + "/";
	if (path)
		href += path.split("/").map(encodeURIComponent).join("/");
	if (isDirectory && !href.endsWith("/"))
		href += "/";
	return href;
}

// tolerate any namespace prefix on DAV: names
function searchDAV(element, name) {
	return element?.elements?.find(item => (item.name === name) || item.name.endsWith(`:${name}`));
}

function deleteTree(directory, path) {
	for (const name of directory.scan(path)) {
		const child = `${path}/${name}`;
		if (directory.status(child).isDirectory())
			deleteTree(directory, child);
		else
			directory.delete(child);
	}
	directory.delete(path);
}

// request body accumulation
function accumulate(route, buffer) {
	(route.body ??= []).push(buffer);
}

function collectString(route) {
	const body = route.body;
	delete route.body;
	if (!body)
		return;
	let total = 0;
	for (const buffer of body)
		total += buffer.byteLength;
	if (!total)
		return;
	const result = new Uint8Array(total);
	let offset = 0;
	for (const buffer of body) {
		result.set(new Uint8Array(buffer), offset);
		offset += buffer.byteLength;
	}
	return result;
}

// request log, for understanding chatty clients. correlate with a packet
// trace by timestamp: epoch seconds, so accurate only when the clock is set.
const LOG_HEADERS = Object.freeze(["depth", "destination", "overwrite", "timeout", "if", "lock-token", "content-length", "content-type", "expect", "range"]);

function logRequest(dav, request) {
	if (!dav.log)
		return;
	let line = `@${(Date.now() / 1000).toFixed(3)} ${request.method} ${request.path}`;
	for (const name of LOG_HEADERS) {
		const value = request.headers.get(name);
		if (undefined !== value)
			line += ` ${name}="${value}"`;
	}
	trace(line, "\n");
}

function logResponse(route, status) {
	if (route.dav.log)
		trace(`@${(Date.now() / 1000).toFixed(3)} ${route.method} ${route.href} -> ${status}\n`);
}

// responses
function respondEmpty(connection, response, status) {
	logResponse(connection.route, status);
	response.status = status;
	response.headers.set("content-length", 0);
	if (405 === status)
		response.headers.set("allow", ALLOW);
	connection.respond(response);
}

function respondChunked(connection, response, status, contentType, parts) {
	logResponse(connection.route, status);
	response.status = status;
	response.headers.set("content-type", contentType);
	response.headers.set("transfer-encoding", "chunked");
	connection.route.parts = parts;
	connection.respond(response);
}

function onWritableChunked(count) {
	const route = this.route;
	try {
		while (count > 0) {
			if (!route.pending) {
				let next;
				do {
					next = route.parts.next();
				} while (!next.done && !next.value);		// a zero-length chunk would end the response early
				if (next.done) {
					this.write();
					return;
				}
				if (next.value instanceof ArrayBuffer)
					route.pending = new Uint8Array(next.value);
				else
					route.pending = new Uint8Array(ArrayBuffer.fromString(next.value));
				route.position = 0;
			}
			const use = Math.min(count, route.pending.length - route.position);
			count = this.write(route.pending.subarray(route.position, route.position + use));
			route.position += use;
			if (route.position === route.pending.length)
				route.pending = undefined;
		}
	}
	catch {		// mid-response failure: too late for a status, close the connection
		logResponse(route, "failed");
		this.close();
	}
}

// PROPFIND / PROPPATCH serialization
const SUPPORTED_LOCK = Object.freeze({
	name: "D:supportedlock",
	elements: [
		{
			name: "D:lockentry",
			elements: [
				{name: "D:lockscope", elements: [{name: "D:exclusive"}]},
				{name: "D:locktype", elements: [{name: "D:write"}]}
			]
		},
		{
			name: "D:lockentry",
			elements: [
				{name: "D:lockscope", elements: [{name: "D:shared"}]},
				{name: "D:locktype", elements: [{name: "D:write"}]}
			]
		}
	]
}, true);

function lockToken() {
	// UUID-shaped so the opaquelocktoken: scheme is well formed (RFC 4918 §6.5);
	// the time is folded in so tokens differ across reboots even where Math.random is poorly seeded
	const bytes = new Uint8Array(16);
	let time = Date.now();
	for (let i = 0; i < 16; i++) {
		bytes[i] = Math.irandom(256) ^ time;
		time = Math.floor(time / 256);
	}
	bytes[6] = (bytes[6] & 0x0F) | 0x40;	// version 4
	bytes[8] = (bytes[8] & 0x3F) | 0x80;	// variant
	const hex = bytes.toHex();
	return "opaquelocktoken:" + hex.slice(0, 8) + "-" + hex.slice(8, 12) + "-" + hex.slice(12, 16) + "-" + hex.slice(16, 20) + "-" + hex.slice(20);
}

function activeLockElement(dav, path, lock) {
	const ownerElement = {name: "D:owner", text: lock.owner};
	if (lock.ownerHREF)
		ownerElement.elements = [{name: "D:href", text: lock.ownerHREF}];
	return {
		name: "D:activelock",
		elements: [
			{name: "D:locktype", elements: [{name: "D:write"}]},
			{name: "D:lockscope", elements: [{name: `D:${lock.lockscope}`}]},
			{name: "D:depth", text: lock.depth ?? "0"},
			ownerElement,
			{name: "D:timeout", text: lock.timeout ?? "Infinite"},
			{name: "D:locktoken", elements: [{name: "D:href", text: lock.token}]},
			{name: "D:lockroot", elements: [{name: "D:href", text: encodePath(dav, path)}]}
		]
	};
}

// The file systems here store no timestamps (littlefs has no clock), so date
// properties report a stable constant. That is better for clients than no
// answer: macOS WebDAVFS re-validates constantly when its caching properties
// (getetag, getlastmodified) go unanswered.
const LAST_MODIFIED = "Fri, 01 Jan 2016 00:00:00 GMT";
const CREATION_DATE = "2016-01-01T00:00:00Z";

const SUPPORTED_PROPERTIES = Object.freeze(["getcontentlength", "resourcetype", "supportedlock", "lockdiscovery", "getetag", "getlastmodified", "creationdate"]);

function propertyElement(dav, local, path, item) {
	switch (local) {
		case "getcontentlength":
			if (item.isDirectory) return;
			return {name: "D:getcontentlength", text: item.size.toString()};
		case "resourcetype":
			return item.isDirectory ? {name: "D:resourcetype", elements: [{name: "D:collection"}]} : {name: "D:resourcetype"};
		case "supportedlock":
			return SUPPORTED_LOCK;
		case "lockdiscovery": {
			const lock = dav.locks.get(path);
			return lock ? {name: "D:lockdiscovery", elements: [activeLockElement(dav, path, lock)]} : {name: "D:lockdiscovery"};
		}
		case "getetag":
			if (item.isDirectory) return;
			return {name: "D:getetag", text: `"${item.size}"`};
		case "getlastmodified":
			return {name: "D:getlastmodified", text: LAST_MODIFIED};
		case "creationdate":
			return {name: "D:creationdate", text: CREATION_DATE};
	}
}

function localName(name) {
	const colon = name.indexOf(":");
	return (colon >= 0) ? name.slice(colon + 1) : name;
}

// resolve an element name's namespace from xmlns declarations, nearest scope first
function xmlnsOf(name, ...scopes) {
	const colon = name.indexOf(":");
	const attribute = (colon >= 0) ? `xmlns:${name.slice(0, colon)}` : "xmlns";
	for (const scope of scopes) {
		const declaration = scope?.attributes?.find(item => item.name === attribute);
		if (declaration)
			return declaration.value;
	}
}

// request is undefined for allprop, "propname", or an array of {local, xmlns}
function serializeResponseItem(dav, path, item, request) {
	const propElements = [], missingElements = [];
	if ("propname" === request) {
		for (const local of SUPPORTED_PROPERTIES) {
			if (propertyElement(dav, local, path, item))
				propElements.push({name: `D:${local}`});
		}
	}
	else if (undefined === request) { // allprop
		for (const local of SUPPORTED_PROPERTIES) {
			const element = propertyElement(dav, local, path, item);
			if (element)
				propElements.push(element);
		}
	}
	else {
		for (const requested of request) {
			const element = ("DAV:" === requested.xmlns) ? propertyElement(dav, requested.local, path, item) : undefined;
			if (element)
				propElements.push(element);
			else if (requested.xmlns)
				missingElements.push({name: `U:${requested.local}`, attributes: [{name: "xmlns:U", value: requested.xmlns}]});
			else
				missingElements.push({name: requested.local, attributes: [{name: "xmlns", value: ""}]});
		}
	}
	const propstats = [];
	if (propElements.length || !missingElements.length) {
		propstats.push({
			name: "D:propstat",
			elements: [
				{name: "D:prop", elements: propElements},
				{name: "D:status", text: "HTTP/1.1 200 OK"}
			]
		});
	}
	if (missingElements.length) {
		propstats.push({
			name: "D:propstat",
			elements: [
				{name: "D:prop", elements: missingElements},
				{name: "D:status", text: "HTTP/1.1 404 Not Found"}
			]
		});
	}
	return XML.serialize({
		name: "D:response",
		elements: [
			{name: "D:href", text: encodePath(dav, path, item.isDirectory)},
			...propstats
		]
	}, {declaration: false, format: "buffer"});
}

function *propfindParts(dav, path, item, depth, request) {
	yield `<?xml version="1.0" encoding="utf-8"?><D:multistatus xmlns:D="DAV:">`;
	yield serializeResponseItem(dav, path, item, request);
	if (item.isDirectory && ("1" === depth)) {
		for (const name of (path ? dav.directory.scan(path) : dav.directory.scan())) {
			if (!path && name.startsWith(TEMP_PREFIX))
				continue;
			const child = path ? `${path}/${name}` : name;
			const childItem = statusOf(dav, child);
			if (childItem)
				yield serializeResponseItem(dav, child, childItem, request);
		}
	}
	yield `</D:multistatus>`;
}

function *htmlParts(dav, path) {
	yield `<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"/><title>${XML.escape(path ? nameOf(path) : "WebDAV")}</title></head><body><ul>`;
	for (const name of (path ? dav.directory.scan(path) : dav.directory.scan())) {
		if (!path && name.startsWith(TEMP_PREFIX))
			continue;
		const child = path ? `${path}/${name}` : name;
		const item = statusOf(dav, child);
		if (!item)
			continue;
		// absolute, so links resolve when the collection URL lacks its trailing slash
		const href = encodePath(dav, child, item.isDirectory);
		yield `<li><a href="${XML.escape(href, "attribute")}">${XML.escape(name)}</a></li>`;
	}
	yield `</ul></body></html>`;
}

// MOVE / COPY destination
function parseDestination(dav, request) {
	let destination = request.headers.get("destination");
	if (!destination)
		return;
	const scheme = destination.indexOf("://");
	if (scheme >= 0) {
		const slash = destination.indexOf("/", scheme + 3);
		if (slash < 0)
			return;
		destination = destination.slice(slash);
	}
	const prefix = dav.prefix;
	if (prefix) {
		if (!destination.startsWith(prefix))
			return;
		destination = destination.slice(prefix.length);
	}
	return normalizePath(destination);
}

function normalizePath(path) {
	return path.split("/").map(decodeURIComponent).filter(part => part && ("." !== part) && (".." !== part)).join("/");
}

// route handlers
const OptionsRoute = Object.freeze({
	onResponse(response) {
		response.headers.set("allow", ALLOW);
		response.headers.set("dav", "1, 2");
		response.headers.set("ms-author-via", "DAV");
		respondEmpty(this, response, 200);
	}
});

// a single byte range: "bytes=start-end", "bytes=start-", or "bytes=-suffix".
// returns {start, end} (inclusive), undefined to serve the entire file (no or
// unsupported range), or null when unsatisfiable (416)
function parseRange(range, size) {
	if (!range?.startsWith("bytes=") || range.includes(","))
		return;		// multiple ranges are not supported; serving the entire file is allowed
	const parts = range.slice(6).split("-");
	if (2 !== parts.length)
		return;
	let [start, end] = parts;
	if ("" === start) {		// suffix: the last N bytes
		const suffix = parseInt(end);
		if (!(suffix > 0))
			return (0 === suffix) ? null : undefined;
		start = Math.max(0, size - suffix);
		end = size - 1;
	}
	else {
		start = parseInt(start);
		end = ("" === end) ? (size - 1) : parseInt(end);
		if (!(start >= 0) || !(end >= 0))
			return;
		if (end > size - 1)
			end = size - 1;
		if ((start >= size) || (start > end))
			return null;
	}
	return {start, end};
}

const GetRoute = Object.freeze({
	onRequest(request) {
		this.route.range = request.headers.get("range");
	},
	onResponse(response) {
		const route = this.route, dav = route.dav;
		try {
			const item = statusOf(dav, route.path);
			if (!item)
				return respondEmpty(this, response, 404);
			if (item.isDirectory)
				return respondChunked(this, response, 200, "text/html; charset=utf-8", htmlParts(dav, route.path));
			response.headers.set("accept-ranges", "bytes");
			const range = parseRange(route.range, item.size);
			if (null === range) {
				response.headers.set("content-range", `bytes */${item.size}`);
				return respondEmpty(this, response, 416);
			}
			route.file = dav.directory.openFile({path: route.path, mode: "r"});
			route.position = 0;
			route.end = item.size;		// exclusive
			let status = 200;
			if (range) {
				status = 206;
				route.position = range.start;
				route.end = range.end + 1;
				response.headers.set("content-range", `bytes ${range.start}-${range.end}/${item.size}`);
			}
			response.status = status;
			response.headers.set("content-length", route.end - route.position);
			response.headers.set("content-type", getContentType(route.path));
			logResponse(route, status);
			this.respond(response);
		}
		catch {
			respondEmpty(this, response, 500);
		}
	},
	onWritable(count) {
		const route = this.route;
		if (route.parts)
			return onWritableChunked.call(this, count);
		try {
			const buffer = route.file.read(Math.min(count, route.end - route.position), route.position);
			route.position += buffer.byteLength;
			this.write(buffer);
		}
		catch {		// mid-response failure: too late for a status, close the connection
			logResponse(route, "failed");
			route.file?.close();
			route.file = undefined;
			this.close();
		}
	},
	onDone() {
		this.route.file?.close();
		this.route.file = undefined;
	},
	onError() {
		logResponse(this.route, "aborted");
		this.route.file?.close();
		this.route.file = undefined;
	}
}, true);

const HeadRoute = Object.freeze({
	onResponse(response) {
		const route = this.route;
		try {
			const item = statusOf(route.dav, route.path);
			if (!item)
				return respondEmpty(this, response, 404);
			if (!item.isDirectory) {
				response.headers.set("content-type", getContentType(route.path));
				response.headers.set("accept-ranges", "bytes");
			}
			respondEmpty(this, response, 200);
		}
		catch {
			respondEmpty(this, response, 500);
		}
	}
});

// close and remove an in-flight upload's temporary file, tolerating a storage
// device in any state — every path here can throw on a full or failing volume
function discardUpload(route) {
	try {
		route.file?.close();
	}
	catch {
		/* this space intentionally left blank */
	}
	route.file = undefined;
	if (route.temp) {
		try {
			route.dav.directory.delete(route.temp);
		}
		catch {
			/* this space intentionally left blank */
		}
		route.temp = undefined;
	}
}

const PutRoute = Object.freeze({
	onRequest(request) {
		const route = this.route, dav = route.dav;
		try {
			if (!route.path)
				return void (route.error = 405);
			const item = statusOf(dav, route.path);
			if (item?.isDirectory)
				return void (route.error = 405);
			route.existed = undefined !== item;
			if (!route.existed) {		// an existing target implies an existing parent
				const parent = parentOf(route.path);
				if (parent && !statusOf(dav, parent)?.isDirectory)
					return void (route.error = 409); // parent collection does not exist
			}
			// upload into a temporary file, renamed over the target on success:
			// a failed or abandoned upload never leaves a partial file behind,
			// and the file being replaced survives intact until the atomic rename
			route.temp = dav.tempPath();
			route.file = dav.directory.openFile({path: route.temp, mode: "w"});
			route.position = 0;
		}
		catch {
			route.error = 507; // Insufficient Storage
		}
	},
	onReadable(count) {
		const route = this.route;
		const buffer = this.read(count);
		if (route.file) {
			try {
				route.file.write(buffer, route.position);
				route.position += buffer.byteLength;
			}
			catch {		// write failed - out of space
				discardUpload(route);
				route.error = 507;
			}
		}
	},
	onResponse(response) {
		const route = this.route, dav = route.dav;
		try {
			route.file?.close();
			route.file = undefined;
			if (!route.error) {
				dav.directory.move(route.temp, route.path);
				route.temp = undefined;
			}
		}
		catch {
			route.error = 507;
		}
		discardUpload(route);
		if (route.error)
			return respondEmpty(this, response, route.error);
		respondEmpty(this, response, route.existed ? 204 : 201);
	},
	onDone() {
		discardUpload(this.route);
	},
	onError() {
		logResponse(this.route, "aborted");
		discardUpload(this.route);
	}
}, true);

const DeleteRoute = Object.freeze({
	onResponse(response) {
		const route = this.route, dav = route.dav;
		try {
			if (!route.path)
				return respondEmpty(this, response, 403);
			const item = statusOf(dav, route.path);
			if (!item)
				return respondEmpty(this, response, 404);
			if (item.isDirectory)
				deleteTree(dav.directory, route.path);
			else
				dav.directory.delete(route.path);
			respondEmpty(this, response, 204);
		}
		catch {
			respondEmpty(this, response, 500);
		}
	}
});

const MkcolRoute = Object.freeze({
	onRequest(request) {
		if ((parseInt(request.headers.get("content-length") ?? "0") > 0) || request.headers.has("transfer-encoding"))
			this.route.error = 415; // MKCOL request bodies are not supported
	},
	onResponse(response) {
		const route = this.route, dav = route.dav;
		if (route.error)
			return respondEmpty(this, response, route.error);
		try {
			if (!route.path)
				return respondEmpty(this, response, 405);
			if (statusOf(dav, route.path))
				return respondEmpty(this, response, 405);
			const parent = parentOf(route.path);
			if (parent && !statusOf(dav, parent)?.isDirectory)
				return respondEmpty(this, response, 409);
			dav.directory.createDirectory(route.path);
			respondEmpty(this, response, 201);
		}
		catch {
			// the missing-parent and already-exists cases are vetted above, so a
			// failure here is the storage itself (macOS maps 409 to "invalid name")
			respondEmpty(this, response, 507);
		}
	}
});

const MoveRoute = Object.freeze({
	onRequest(request) {
		this.route.destination = parseDestination(this.route.dav, request);
		this.route.overwrite = request.headers.get("overwrite") ?? "T";
	},
	onResponse(response) {
		const route = this.route, dav = route.dav;
		try {
			const destination = route.destination;
			if (!route.path || (undefined === destination))
				return respondEmpty(this, response, 400);
			if (!statusOf(dav, route.path))
				return respondEmpty(this, response, 404);
			if (destination === route.path)
				return respondEmpty(this, response, 403);
			const existing = statusOf(dav, destination);
			if (existing) {
				if ("F" === route.overwrite)
					return respondEmpty(this, response, 412);
				if (existing.isDirectory)
					deleteTree(dav.directory, destination);
				else
					dav.directory.delete(destination);
			}
			dav.directory.move(route.path, destination);
			respondEmpty(this, response, existing ? 204 : 201);
		}
		catch {
			respondEmpty(this, response, 409);
		}
	}
});

const CopyRoute = Object.freeze({
	onRequest(request) {
		this.route.destination = parseDestination(this.route.dav, request);
		this.route.overwrite = request.headers.get("overwrite") ?? "T";
	},
	onResponse(response) {
		const route = this.route, dav = route.dav;
		let from, to, temp;
		try {
			const destination = route.destination;
			if (!route.path || (undefined === destination))
				return respondEmpty(this, response, 400);
			const item = statusOf(dav, route.path);
			if (!item)
				return respondEmpty(this, response, 404);
			if (item.isDirectory)
				return respondEmpty(this, response, 501); // collection COPY is not implemented
			if (destination === route.path)
				return respondEmpty(this, response, 403);
			const existing = statusOf(dav, destination);
			if (existing) {
				if ("F" === route.overwrite)
					return respondEmpty(this, response, 412);
				if (existing.isDirectory)
					deleteTree(dav.directory, destination);
			}
			from = dav.directory.openFile({path: route.path, mode: "r"});
			// copy into a temporary file, renamed over the destination on
			// success, so a failed copy never leaves a partial destination
			temp = dav.tempPath();
			to = dav.directory.openFile({path: temp, mode: "w"});
			let position = 0;
			while (position < item.size) {
				const buffer = from.read(Math.min(COPY_BUFFER, item.size - position), position);
				to.write(buffer, position);
				position += buffer.byteLength;
			}
			to.close();
			to = undefined;
			dav.directory.move(temp, destination);
			temp = undefined;
			respondEmpty(this, response, existing ? 204 : 201);
		}
		catch {
			// preconditions are vetted above, so a failure here is storage
			respondEmpty(this, response, 507);
		}
		finally {
			try {
				from?.close();
			}
			catch {
				/* this space intentionally left blank */
			}
			try {
				to?.close();
			}
			catch {
				/* this space intentionally left blank */
			}
			try {
				if (temp)
					dav.directory.delete(temp);
			}
			catch {
				/* this space intentionally left blank */
			}
		}
	}
});

const PropfindRoute = Object.freeze({
	onRequest(request) {
		this.route.depth = request.headers.get("depth") ?? "infinity";
	},
	onReadable(count) {
		accumulate(this.route, this.read(count));
	},
	onResponse(response) {
		const route = this.route, dav = route.dav;
		try {
			const body = collectString(route);
			if (("0" !== route.depth) && ("1" !== route.depth))
				return respondEmpty(this, response, 403); // propfind-finite-depth
			const item = statusOf(dav, route.path);
			if (!item)
				return respondEmpty(this, response, 404);
			let request; // no body is allprop
			if (body) {
				const root = XML.parse(body);
				if (searchDAV(root, "propname"))
					request = "propname";
				else if (!searchDAV(root, "allprop")) {
					const prop = searchDAV(root, "prop");
					request = (prop?.elements ?? []).map(element => ({
						local: localName(element.name),
						xmlns: xmlnsOf(element.name, element, prop, root)
					}));
				}
			}
			respondChunked(this, response, 207, `application/xml; charset="utf-8"`, propfindParts(dav, route.path, item, route.depth, request));
		}
		catch {
			respondEmpty(this, response, 400); // could not parse request body
		}
	},
	onWritable: onWritableChunked
}, true);

const ProppatchRoute = Object.freeze({
	onResponse(response) {
		const route = this.route, dav = route.dav;
		try {
			if (!statusOf(dav, route.path))
				return respondEmpty(this, response, 404);
			const body = XML.serialize({
				name: "D:multistatus",
				attributes: [{name: "xmlns:D", value: "DAV:"}],
				elements: [
					{
						name: "D:response",
						elements: [
							{name: "D:href", text: encodePath(dav, route.path)},
							{name: "D:propstat", elements: [{name: "D:status", text: "HTTP/1.1 200 OK"}]}
						]
					}
				]
			}, {declaration: true, format: "buffer"});
			respondChunked(this, response, 207, `application/xml; charset="utf-8"`, [body].values());
		}
		catch {
			respondEmpty(this, response, 500);
		}
	},
	onWritable: onWritableChunked
}, true);

const LockRoute = Object.freeze({
	// THE LOCKING MECHANISM IS FAKE, NOT SAFE FOR MULTIPLE USERS!
	onRequest(request) {
		this.route.depth = request.headers.get("depth") ?? "infinity";
		this.route.timeout = request.headers.get("timeout") ?? "Infinite";
	},
	onReadable(count) {
		accumulate(this.route, this.read(count));
	},
	onResponse(response) {
		const route = this.route, dav = route.dav;
		try {
			if (!route.path)
				return respondEmpty(this, response, 403);
			let status = 200;
			if (!statusOf(dav, route.path)) {
				const parent = parentOf(route.path);
				if (parent && !statusOf(dav, parent)?.isDirectory)
					return respondEmpty(this, response, 409);
				dav.directory.openFile({path: route.path, mode: "w"}).close(); // LOCK on an unmapped URL creates an empty resource
				status = 201;
			}
			const body = collectString(route);
			let lock = dav.locks.get(route.path);
			if (body || !lock) {
				// a new lock request carries a body; a refresh does not and
				// must answer with the token issued before
				lock = {
					token: lockToken(),
					lockscope: "exclusive"
				};
				if (body) {
					const root = XML.parse(body);
					let element = searchDAV(root, "lockscope");
					if (element && searchDAV(element, "shared"))
						lock.lockscope = "shared";
					element = searchDAV(root, "owner");
					if (element) {
						lock.owner = element.text;
						lock.ownerHREF = searchDAV(element, "href")?.text;
					}
				}
				dav.locks.set(route.path, lock);
				if (dav.locks.size > 16)		// the locks are advisory; cap the table
					dav.locks.delete(dav.locks.keys().next().value);
			}
			lock.depth = route.depth;
			lock.timeout = route.timeout;
			const result = XML.serialize({
				name: "D:prop",
				attributes: [{name: "xmlns:D", value: "DAV:"}],
				elements: [
					{
						name: "D:lockdiscovery",
						elements: [activeLockElement(dav, route.path, lock)]
					}
				]
			}, {declaration: true, format: "buffer"});
			response.headers.set("lock-token", `<${lock.token}>`);
			respondChunked(this, response, status, `application/xml; charset="utf-8"`, [result].values());
		}
		catch {
			respondEmpty(this, response, 400);
		}
	},
	onWritable: onWritableChunked
}, true);

const UnlockRoute = Object.freeze({
	onResponse(response) {
		this.route.dav.locks.delete(this.route.path);
		respondEmpty(this, response, 204);
	}
});

const MethodNotAllowedRoute = Object.freeze({
	onResponse(response) {
		respondEmpty(this, response, 405);
	}
});

const ForbiddenRoute = Object.freeze({
	onResponse(response) {
		respondEmpty(this, response, 403);
	}
});

const handlers = Object.freeze({
	OPTIONS: OptionsRoute,
	GET: GetRoute,
	HEAD: HeadRoute,
	PUT: PutRoute,
	DELETE: DeleteRoute,
	MKCOL: MkcolRoute,
	MOVE: MoveRoute,
	COPY: CopyRoute,
	PROPFIND: PropfindRoute,
	PROPPATCH: ProppatchRoute,
	LOCK: LockRoute,
	UNLOCK: UnlockRoute
});

class WebDAV {
	#directory;
	#prefix;
	#locks = new Map;		// path -> {token, lockscope, owner, ownerHREF, depth, timeout}; advisory only
	#log;
	#temp = 0;
	#appleDouble;

	constructor(options) {
		this.#directory = options.directory;
		if (!this.#directory)
			throw new Error("directory required");
		this.#prefix = options.prefix ?? "";
		this.#log = options.log ?? false;
		this.#appleDouble = options.appleDouble ?? true;

		try {
			// discard temporary upload files orphaned by an interrupted run
			const orphans = [];
			for (const name of this.#directory.scan()) {
				if (name.startsWith(TEMP_PREFIX))
					orphans.push(name);
			}
			for (const name of orphans)
				this.#directory.delete(name);

			// macOS probes for this marker when mounting; its presence stops
			// Spotlight from trying to index (and litter) the volume
			if (!this.#directory.status(".metadata_never_index").isFile())
				this.#directory.openFile({path: ".metadata_never_index", mode: "w"}).close();
		}
		catch {
			/* housekeeping - a full or failing volume should not block start-up */
		}
	}
	tempPath() {
		return TEMP_PREFIX + this.#temp++;
	}
	get directory() {
		return this.#directory;
	}
	get prefix() {
		return this.#prefix;
	}
	get locks() {
		return this.#locks;
	}
	get log() {
		return this.#log;
	}
	onRoute(request) {
		logRequest(this, request);		// every request seen, including ones passed on
		const prefix = this.#prefix;
		let path = request.path;
		if (prefix) {
			if (!path.startsWith(prefix))
				return;
			path = path.slice(prefix.length);
			if (path && !path.startsWith("/"))
				return; // "/davish" is not under "/dav"
		}
		let route = handlers[request.method] ?? MethodNotAllowedRoute;
		path = normalizePath(path);
		// refusing to create "._" AppleDouble files makes macOS drop the
		// metadata gracefully, halving Finder's requests per file copied
		if (!this.#appleDouble && (("PUT" === request.method) || ("LOCK" === request.method) || ("MKCOL" === request.method)) && nameOf(path).startsWith("._"))
			route = ForbiddenRoute;
		return {...route, dav: this, path, method: request.method, href: request.path};
	}
}

export default WebDAV;
