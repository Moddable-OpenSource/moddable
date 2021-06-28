/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
/*
 * Read a GPS coordinate and plot a map.
 * Uses a OpenStreetMap style server from mapbox.com.
 * Make an account and use your token in MAPBOX_TOKEN.
 * build line:
 *		mcconfig -d -m -p esp32/moddable_zero ssid=<your-ssid> password=<ssid-password>
 */

import Timer from "timer";
import SIM7100 from "sim7100";

import {Request} from "http";
import JPEG from "commodetto/readJPEG";
import Poco from "commodetto/Poco";
import config from "mc/config";

//const MAPBOX_TOKEN = "---your token from mapbox.com here---";
//const MAPBOX_STYLE = "--- (optional) style link here ---";

const MAPBOX_BASE_MAP = "/v4/mapbox.satellite";

const MAP_ZOOM = 16;

let frame = {};

/*
 * http request callback
 * Receives JPEG data and adds it to the JPEG decoder.
 * When frame is ready, cycle through the JPEG blocks
 * and draw them to the screen.
 */
function http_request_callback(msg, val, etc) {
	switch (msg) {
		case 1:
			if (undefined !== val)
				trace(val);
			if (undefined !== etc)
				trace(etc);
			trace("\n");
			frame.jpeg = new JPEG();
			break;
		case 4:
			frame.jpeg.push(this.read(ArrayBuffer));
			break;
		case 5:
			frame.jpeg.push();
			break;
	}

	while (frame.jpeg.ready) {
		let block = frame.jpeg.read();
		poco.begin(block.x, block.y, block.width, block.height);
		poco.drawBitmap(block, block.x, block.y);
		poco.end();
	}
}

/*
 * Map tile calculation
 * from: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Lon..2Flat._to_tile_numbers
 */
function lon2tile(lon,zoom) {
	return (Math.floor((lon+180)/360*Math.pow(2,zoom)));
}
function lat2tile(lat,zoom) {
	return (Math.floor((1-Math.log(Math.tan(lat*Math.PI/180) + 1/Math.cos(lat*Math.PI/180))/Math.PI)/2 *Math.pow(2,zoom)));
}

/*
 * Set up a HTTP request to fetch a map tile.
 */
function showMap(gps) {
	let tile_x = lon2tile(gps.lon, gps.zoom);
	let tile_y = lat2tile(gps.lat, gps.zoom);
	let path = `${MAPBOX_BASE_MAP}/${gps.zoom}/${tile_x}/${tile_y}.jpg`;
	path = path + "?access_token=" + MAPBOX_TOKEN;
	if (undefined !== MAPBOX_STYLE)
		"&style=" + MAPBOX_STYLE;

	let request = new Request({
			host: 		"api.mapbox.com",
			path: 		path,
			headers:	[ "User-Agent", "gpstest/0.1 moddable/1.0" ],
		});
	request.callback = http_request_callback;
}

/*
 * Main
 * Sets up the GPS module, gets a reading and calls showMap()
 */

export default function() {
	let sim7100 = new SIM7100();

	sim7100.onReady = function() {
		sim7100.enableGPS();
	}
	sim7100.onGPSEnabled = function(device) {
		sim7100.getGPS();
	};
	sim7100.onGPSChanged = function(gps) {
		gps.zoom = MAP_ZOOM;
		showMap(gps);
	}

	global.poco = new Poco(screen);
	let gray = poco.makeColor(20, 20, 20);

	poco.begin();
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
	poco.end();

	sim7100.start();
}


