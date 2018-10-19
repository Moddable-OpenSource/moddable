/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

const ThingHeaders = [	"content-type", "application/json",
						"access-control-allow-origin", "*",
						"access-control-allow-methods", "PUT, GET"];

class WebThing {
	constructor(host) {
		this.host = host;
		this.controllers = [];
	}
	changed() {
		const thing = this.host.things.find(thing => this === thing.instance);
		if (this.host.updateTXT(thing))
			this.host.mdns.update(thing.service);
	}
    set controller(data) {
		const thing = this.host.things.find(thing => this === thing.instance);
    	for (let i=0; i<data.length; i++) {
    		let item = data[i];
    		if (item.property === undefined || item.remote === undefined || item.txt === undefined || thing.description.properties[item.property] === undefined) {
    			data.splice(i, 1);
    			i--;
    		}
    	}
    	this.controllers = data;
    }
    get controller() {
    	return this.controllers;
    }
}
Object.freeze(WebThing.prototype);


class WebThings {
	constructor(mdns) {
		this.mdns = mdns;
		this.things = [];
		this.server = new Server({port: 80});
		this.server.callback = this.callback.bind(this);
		mdns.monitor("_webthing._tcp", (service, instance) => {
			let txt = instance.txt;
			let name = instance.name;
			this.things.forEach(thing => {
				let properties = Object.keys(thing.description.properties);
				let bools = properties.filter(prop => {
					let isControlled = false;
					for (let item of thing.instance.controller) {
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
					let controllers = thing.instance.controller;
					controllers.forEach(item => {
						if ((name === item.remote) && (key === item.txt)){
							let property = item.property;
							let type = thing.description.properties[property];
							if (type) {
								type = type.type;
								switch(type) {
									case "boolean":
										thing.instance[property] = true;
										bools.splice(bools.indexOf(property), 1);
										break;
									case "number":
									case "integer":
										thing.instance[property] = parseInt(value);
										break;
									case "object":
									case "array":
										thing.instance[property] = JSON.parse(value);
										break;
									default:
										thing.instance[property] = value;
										break;
								}
							}
						}
					});
				}
				for (let prop of bools) {
					thing.instance[prop] = false;
				}
			});
		});
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
		this.mdns.add(thing.service);

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
				else if ("" == prev)
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
	callback(message, value, etc) {
		switch (message) {
			case 2:
				this.path = value;
				this.method = etc;
				break;

			case 4:
				return ("PUT" === this.method) ? String : undefined;

			case 6:
				this.JSON = JSON.parse(value);
				break;

			case 8: {
				let body;

				switch (this.method) {
					case "GET":
						if (this.path.startsWith("/thng/desc/")) {
							this.things.forEach(thing => {
								if (this.path.slice("/thng/desc/".length) === thing.description.name)
									body = thing.description;
							});
						}
						else
						if (this.path.startsWith("/thng/prop/")) {
							const parts = this.path.split("/");
							const name = parts[3], property = parts[4];
							this.things.forEach(thing => {
								if (name === thing.description.name)
									body = {[property]: thing.instance[property]};
							});
						}
						else
								body = {error: "invalid path"};
						break;
					case "PUT":
						if (this.path.startsWith("/thng/prop/")) {
							const parts = this.path.split("/");
							const name = parts[3], property = parts[4];
							this.things.forEach(thing => {
								if (name === thing.description.name) {
									thing.instance[property] = this.JSON[property];
									body = {[property]: thing.instance[property]};
								}
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
}
Object.freeze(WebThings.prototype);

export {WebThings, WebThing};
