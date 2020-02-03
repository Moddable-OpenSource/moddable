/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	pubnub
*/

import {Request} from "http";
import UUID from "uuid";

export class PubNub {
	constructor(params) {
		this.config = { authority:"pubsub.pubnub.com", pnsdk:"Moddable-XS6/1", uuid: UUID() };

		Object.assign(this.config, {
			pub_key:params.publishKey,
			sub_key:params.subscribeKey
		});
		this.listeners = [];
	}
	addListener(listener) {
		this.listeners.push(listener);
	}
	announceMessage(announce) {
		this.listeners.forEach(listener => {
			if (listener.message) listener.message(announce);
		});
	}
	announceStatus(announce) {
		this.listeners.forEach(listener => {
			if (listener.status) listener.status(announce);
		});
	}
	publish(params, callback, scope) {
		let channel = params.channel;
		let message = params.message;
		REST_API.publish(this.config, { channel, message, store:0 }, (error, data) => {
			if (error) trace("Publish Error: " + error + "\n");
			if (callback) {
				if (scope) callback.bind(scope);
				callback(error, data);
			}
		}, this);
	}
	subscribe(params, callback, scope) {
		if (params.channels && params.channels.length > 1) {
			trace("Note: multiple channels not supported!\n");
			debugger;
		}
		let channel = params.channel || params.channels[0];
		let timetoken = params.timetoken || this.timetoken;
		REST_API.subscribe(this.config, { channel, timetoken }, (error, data) => {
			if (data) data = JSON.parse(data);
			if (error) trace("Subscribe Error: " + error + "\n");
			else if (Array.isArray(data)) {
				timetoken = data[1].toString();
				if (this.timetoken) {
					data[0].forEach(message => {
						this.announceMessage({ actualChannel: null, message, subscribedChannel: channel, timetoken });
					});
				}
				else {
					let category = "PNConnectedCategory";
					this.announceStatus({ category });
				}
				this.timetoken = timetoken;
				this.subscribe({ channel, timetoken });
			}
			else trace("Subscribe Error: unexpected data type: " + (typeof data) + "\n");
			if (callback) {
				if (scope) callback.bind(scope);
				callback(error, data);
			}
		}, this);
	}
}

// PubNub REST API Documentation
// https://www.pubnub.com/docs/pubnub-rest-api-documentation

function makeRequest(params, callback, scope) {
	let requestData = {
		host: params.authority,
		response: String
	}
	let path = params.location;
	let requirement = { uuid:params.uuid, pnsdk:params.pnsdk };
	if (params.query && Object.keys(params.query).length) {
		path += "?" + serializeQuery(Object.assign({}, params.query, requirement));
	}
	else {
		path += "?" + serializeQuery(requirement);
	}
	requestData.path = path;
	let data = params.data || params.message;
	if (data) {
		requestData.body = JSON.stringify(data);
		requestData.headers = [
			"Content-Length", requestData.body.length,
			"Content-Type", "application/json; charset=UTF-8",
			"Connection", "close"
		];
		requestData.method = "POST";
	}

	let request = new Request(requestData);
	request.callback = (message, data) => {
		switch (message) {
			case 0:
				//debugger;
				break;
			case 1:
				//debugger;
				break;
			case 2:
				//debugger;
				break;
			case 3:
				//debugger;
				break;
			case 4:
				//debugger;
				break;
			case 5:
				//debugger;
				if (callback) {
					if (scope) callback.bind(scope);
					callback(null, data);
				}
				break;
			default:
				//debugger;
				break;
		}
	}
	return request;
}

function serializeQuery(obj, sep = "&", eq = "=") {
	let queryString = "";
	for (let key in obj) {
		if (queryString) queryString += sep;
		queryString += key + eq + encodeURIComponent(obj[key]);
	}
	return queryString;
}

export class REST_API {
	static fetchHistory(config, params, callback, scope) {
		params = Object.assign({}, config, params);
		let query = params.query = {};
		if ("stringtoken" in params) {
			query.stringtoken = params.stringtoken;
		}
		if ("count" in params) {
			query.count = params.count;
		}
		if ("reverse" in params) {
			query.reverse = params.reverse;
		}
		if ("start" in params) {
			query.start = params.start;
		}
		if ("end" in params) {
			query.end = params.end;
		}
		if ("include_token" in params) {
			query.include_token = params.include_token;
		}
		if ("auth" in params) {
			query.auth = params.auth;
			debugger; // auth not supported
		}
		let channel = encodeURIComponent(params.channel);
		params.location = `/v2/history/sub-key/${params.sub_key}/channel/${channel}`;
		return makeRequest(params, callback, scope);
	}
	static fetchTime(config, params, callback, scope) {
		params = Object.assign({}, config, params);
		params.location = "/time/0";
		return makeRequest(params, callback, scope);
	}
	static publish(config, params, callback, scope) {
		params = Object.assign({}, config, params);
		let query = params.query = {};
		if ("store" in params) {
			query.store = params.store;
		}
		if ("auth" in params) {
			query.auth = params.auth;
			debugger; // auth not supported
		}
		let channel = encodeURIComponent(params.channel);
		params.location = `/publish/${params.pub_key}/${params.sub_key}/0/${channel}/0`;
		return makeRequest(params, callback, scope);
	}
	static subscribe(config, params, callback, scope) {
		params = Object.assign({}, config, params);
		let query = params.query = {};
		if ("state" in params) {
			query.state = params.state;
			debugger; // state not supported
		}
		if ("heartbeat" in params) {
			query.heartbeat = params.heartbeat;
			debugger; // heartbeat not supported
		}
		if ("auth" in params) {
			query.auth = params.auth;
			debugger; // auth not supported
		}
		let channel = encodeURIComponent(params.channel);
		let timetoken = params.timetoken || "0"; // zero for initial subscribe
		params.location = `/subscribe/${params.sub_key}/${channel}/0/${timetoken}`;
		return makeRequest(params, callback, scope);
	}
}

export default {
	PubNub
};
