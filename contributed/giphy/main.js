/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
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

import {} from "piu/MC";
import SCREENS from "screens";
import {Request} from "http"
import SecureSocket from "securesocket";
import Flash from "flash";
import ReadGIF from "commodetto/ReadGIF";
import config from "mc/config";

if (config.ssid === "YOUR_SSID_HERE")
	throw new Error("Wi-Fi SSID required");
const API_KEY = config.apiKey;
if (API_KEY === "YOUR_API_KEY_HERE")
	throw new Error("Giphy API Key required");

const keys = Object.freeze(["data", "images", "username", "title", "fixed_width", "url", "size", "user", "display_name", "meta", "status"]);

class GiphyAppBehavior extends Behavior {
	onCreate(application) {
		if (application.rotation !== undefined && application.width > application.height)
			application.rotation = 90;
	}
	onStringEntered(application, string) {
		application.defer("searchGiphy", string, 0);
	}
	searchGiphy(application, string, offset) {
		application.empty();
		application.purge();
		application.add(new SCREENS.GIFScreen({}));

		trace(`Searching for ${string} gifs...\n`)
		let request = this.request = new Request({
			host: "api.giphy.com", 
			path: `/v1/gifs/search?api_key=${API_KEY}&q=${encodeURIComponent(string)}&limit=1&offset=${offset}&rating=g&lang=en`, 
			response: String,
			port: 443, 
			Socket: SecureSocket, 
			secure: {protocolVersion: 0x303} 
		});
		request.callback = function(message, value, etc) {
			if (Request.responseComplete === message) {
				let response = JSON.parse(value, keys);
				if (!response.data.length) {
					application.distribute("onError", `No GIFs found for search query "${string}"`);
				} else {
					let item = response.data[0];
					if (Number(item.images.fixed_width.size) > 2_090_000) { // Don't try to download images that are too big to fit into flash
						application.defer("searchGiphy", string, offset+1);
					} else {
						application.distribute("updateTitle", item.title);
						application.defer("downloadGIF", item.images.fixed_width.url);
						trace(item.images.fixed_width.url, "\n")
					}
				}
			}
		}
	}
	downloadGIF(application, url){
		let ready = false;
		let partition = new Flash("xs");
		let blockSize = partition.blockSize;
		partition.erase(0);
		const buffer = partition.map();
		let blockIndex = 1;
		let offset = 0;
		let pathIndex = url.indexOf("/media/");
		let host = (url.indexOf("https") > -1)? url.slice(8, pathIndex) : url.slice(7, pathIndex);
		let path = url.slice(pathIndex);
		let reader;
		let request = this.request = new Request({
			host,
			path,
			port: 443, 
			Socket: SecureSocket, 
			secure: {protocolVersion: 0x303, verify: false } 
		})
		request.callback = function(message, value, etc) {
			switch (message) {
				case Request.header:
					if ("content-length" === value)
						this.length = parseInt(etc, 10);
					break;

				case Request.responseFragment: {
					const data = this.read(ArrayBuffer);
					const byteLength = data.byteLength;
					while ((offset + byteLength) > blockIndex * blockSize) {
						partition.erase(blockIndex);
						blockIndex++;
					}
					partition.write(offset, byteLength, data);
					offset += byteLength;

					application.distribute("onUpdateProgress", offset/this.length);
					if (ready) break;

					if (!reader) {
						try {
							reader = new ReadGIF(buffer, {available: offset});
						} catch {
							return;
						}
					}

					reader.available = offset;
					ready = reader.ready;
					if (!ready) break;

					application.first.defer("showFirstFrame");
					reader = undefined;
					} break;

				case Request.responseComplete:
					trace(`Transfer complete.\n`);
					reader = undefined;
					application.first.defer("onGIFDownloaded");
					break;
			}
		}
	}
	stopDownload(application) {
		this.request.close();
		application.defer("openKeyboard");
	}
	stopGIF(application) {
		application.defer("openKeyboard");
	}
	openKeyboard(application) {
		application.empty();
		application.purge();
		application.add(new SCREENS.KeyboardScreen({}));
	}
}

const GiphyApp = Application.template($ => ({
	contents: [
		new SCREENS.KeyboardScreen($)
	],
	Behavior: GiphyAppBehavior
}));

export default function() {
	return new GiphyApp({}, { commandListLength:2448, displayListLength:2600, touchCount:1 })
}



