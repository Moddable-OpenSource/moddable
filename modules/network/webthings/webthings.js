/*
 * Copyright (c) 2018-19  Moddable Tech, Inc.
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
import Timer from "timer";
import {Server} from "http"

const ThingHeaders = Object.freeze([	"content-type", "application/json",
						"access-control-allow-origin", "*",
						"access-control-allow-methods", "PUT, GET"], true);

class WebThing {
	constructor(host) {
		this.host = host;
		this.controllers = [];
		trace("Web Thing API support deprecated.\n");
	}
	changed() {
		if (!this.host.mdns)
			return;
		const thing = this.host.things.find(thing => this === thing.instance);
		if (this.host.updateTXT(thing))
			this.host.mdns.update(thing.service);
	}
    set controller(data) {
		const description = this.constructor.description;
    	for (let i=0; i<data.length; i++) {
    		let item = data[i];
    		if (!item.property || !item.remote || !item.txt ||
				!description.properties[item.property] ||
				description.properties[item.property].readOnly) {
    			data.splice(i, 1);
    			i--;
    		}
    	}
    	this.controllers = data;
		this.host.monitor();
    }
    get controller() {
    	return this.controllers;
    }
}
Object.freeze(WebThing.prototype);

class WebThings {
	constructor(mdns, dictionary = {}) {
		if (mdns)
			this.mdns = mdns;
		this.things = [];
		this.server = new Server(dictionary.server);
		this.server.callback = this.http;
		this.server.webThings = this;
	}
	close () {
		if (this.server)
			this.server.close();
		delete this.server;
		delete this.things;
	}
	add(WebThing, ...args) {
		const description = WebThing.description;
		description.properties.controller = {
			type: "object",
			description: "property inputs",
		};
		for (let property in description.properties)
			description.properties[property].href = `/thng/prop/${description.name}/${property}`;
		const thing = {
			description,
			instance: new WebThing(this, ...args),
			service: {
				name: "webthing",
				protocol: "tcp",
				port: 80,
				txt: {
					path: `/thng/desc/${description.name}`,
				}
			}
		}
		this.things.push(thing);
		this.updateTXT(thing);
		if (this.mdns) {
			this.monitor();
			this.mdns.add(thing.service);
		}

		return thing.instance;
	}
	updateTXT(thing) {
		let changed;
		for (let name in thing.description.properties) {
			const property = thing.description.properties[name];
			if (!property.txt) continue;
			const value = thing.instance[name];
			let prev = thing.service.txt[property.txt];
			if ("boolean" === property.type) {
				if (undefined === prev)
					prev = false;
				else if ("" === prev)
					prev = true;
				if (value !== prev) {
					if (value)
						thing.service.txt[property.txt] = "";
					else
						delete thing.service.txt[property.txt];
					changed = true;
				}
			}
			else if (value !== prev) {
				thing.service.txt[property.txt] = thing.instance[name];
				changed = true;
			}
		}
		return changed;
	}
	http(message, value, etc) {
		const webThings = this.server.webThings;

		if (this.delegate)
			return webThings.callback.call(this, message, value, etc);

		switch (message) {
			case 2:
				this.delegate = webThings.callback && !value.startsWith("/thng/");
				if (this.delegate)
					return webThings.callback.call(this, message, value, etc);
				this.path = value;
				this.method = etc;
				break;

			case 4:
				return ("PUT" === this.method) ? String : undefined;

			case 6:
				this.JSON = JSON.parse(value);
				break;

			case 8: {
				const things = webThings.things;
				let body;

				switch (this.method) {
					case "GET":
						if (this.path.startsWith("/thng/desc/")) {
							things.some(thing => {
								if (this.path.slice("/thng/desc/".length) !== thing.description.name)
									return;

								body = thing.description;
								return true;
							});
						}
						else
						if (this.path.startsWith("/thng/prop/")) {
							const parts = this.path.split("/");
							const name = parts[3], property = parts[4];
							things.some(thing => {
								if (name !== thing.description.name)
									return;

								body = {[property]: thing.instance[property]};
								return true;
							});
						}
						else
							body = {error: "invalid path"};
						break;
					case "PUT":
						if (this.path.startsWith("/thng/prop/")) {
							const parts = this.path.split("/");
							const name = parts[3], property = parts[4];
							things.some(thing => {
								if (name !== thing.description.name)
									return;

								if (thing.description.properties[property].readOnly)
									body = {[property]: thing.instance[property], error: "readOnly"};
								else {
									thing.instance[property] = this.JSON[property];
									body = {[property]: thing.instance[property]};
								}
								return true;
							});
						}
						else 
							body = {error: "invalid path"};
						break;
					default:
						body = {error: "invalid method"};
						break;
				}

				return {headers: ThingHeaders, body: JSON.stringify(body)};
				}
				break;
		}
	}
	monitor() {
		const enable = (this.mdns ? true : false) && this.things.some(thing => 0 !== thing.instance.controllers.length);

		if (enable === (undefined !== this._watch))
			return;

		if (enable) {
			this._watch = this.watch.bind(this);
			this.mdns.monitor("_webthing._tcp", this._watch);
		}
		else {
			this.mdns.remove("_webthing._tcp", this._watch);
			delete this._watch;
		}
	}
	watch(service, instance) {
		let txt = instance.txt;
		let name = instance.name;
		for (let j = 0; j < this.things.length; j++) {
			const thing = this.things[j];
			let properties = Object.keys(thing.description.properties);
			let bools = properties.filter(prop => {
				let isControlled = false;
				for (let item of thing.instance.controllers) {
					if ((name === item.remote) && (prop === item.property)) isControlled = true;
				}
				return (thing.description.properties[prop].type === "boolean") && isControlled;
			});
			let item, equal, key, value;
			for (let i=1; i<txt.length; i++) {	// first item is description url
				item = instance.txt[i];
				equal = item.indexOf("=");
				key = item.substring(0, equal);
				value = item.substring(equal+1);
				for (let k = 0; k < thing.instance.controllers.length; k++) {
					const item = thing.instance.controllers[k];
					if ((name === item.remote) && (key === item.txt)){
						let property = item.property;
						let type = thing.description.properties[property];
						if (type) {
							switch(type.type) {
								case "boolean":
									value = true;
									bools.splice(bools.indexOf(property), 1);
									break;
								case "number":
								case "integer":
									value = ("number" === type.type) ? parseFloat(value) : parseInt(value);
									if ((undefined !== type.minimum) && (value < type.minimum))
										value = type.minimum;
									if ((undefined !== type.maximum) && (value > type.maximum))
										value = type.maximum;
									break;
								case "object":
								case "array":
									value = JSON.parse(value);
									break;
							}
							thing.instance[property] = value;
						}
					}
				}
			}
			for (let prop of bools)
				thing.instance[prop] = false;
		}
	}
}
Object.freeze(WebThings.prototype);

export {WebThings, WebThing};
