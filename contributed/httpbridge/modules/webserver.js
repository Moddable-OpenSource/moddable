/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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

import {Server as HTTPServer} from "http"

class Bridge {
	#next;
	constructor() {
		this.#next = null
	}
	get next() {
		return this.#next;
	}
	set next(n) {
		this.#next = n;
	}
}

class WebServer extends HTTPServer {
	#last;
	#first;

	constructor(options) {
		super(options);

		this.#first = null;
		this.#last = null;
	}

	use( handler ) {
		// chaining here, join new to previous
		if( this.#first === null ) {
			this.#first=this.#last=handler;
		} else {
			this.#last.next=handler;
			this.#last=handler;
		}
		// Advise Bridge of http parent - needed by websocket for connection management
		if ( 'parent' in handler ) 
			handler.parent=this;
		return handler;
	}
	
	callback(message, value, etc) {
		switch (message) {
			case HTTPServer.status: 
				this.path=value;
				this.method = etc;
			break;
			case HTTPServer.header: 
				value=value.toLowerCase(); // Header field names are case-insensitive - force lower for easy compare
		}
		return this.server.#first?.handler(this, message, value, etc);
	}

	close() {
		if ( this.connections )
			super.close(); // Close any http connections
	}
}

Object.freeze(WebServer.prototype);

export { WebServer, HTTPServer, Bridge };
