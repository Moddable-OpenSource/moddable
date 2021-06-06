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
	This example demonstrates how to monitor and control iOS device media playback from a ESP32 Moddable Zero.
	The Moddable Zero implements a BLE Apple Media Service client. Album artwork is fetched over WiFi using the iTunes Search API.
	Build the example using the build command line below, making sure to specify your WiFi network SSID and password.
	
	mcconfig -d -m -p esp32/moddable_zero ssid=<your ssid> password=<your password>

	https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleMediaService_Reference/Specification/Specification.html#//apple_ref/doc/uid/TP40014716-CH1-SW38
	https://affiliate.itunes.apple.com/resources/documentation/itunes-store-web-service-search-api/

	Media transport button graphics from KPR sample code Open Source repository:
	https://github.com/Kinoma/KPR-examples/blob/master/media-player/src/assets/media-transport-controls.png
*/

import {AMSAuthenticator, AMSClient, PlaybackState, RemoteCommandID} from "amsclient";
import Time from "time";
import Timer from "timer";
import Poco from "commodetto/Poco";
import JPEG from "commodetto/readJPEG";
import parseBMF from "commodetto/parseBMF";
import parseRLE from "commodetto/parseRLE";
import Resource from "Resource";
import {Request} from "http";
import SecureSocket from "securesocket";
import config from "mc/config";

let render = new Poco(screen);
let touch = new config.Touch;
touch.points = [{}];

let titleFont = parseBMF(new Resource("OpenSans-Semibold-18.bf4"));
let smallFont = parseBMF(new Resource("OpenSans-Regular-18.bf4"));
let blue = render.makeColor(0x19, 0x2e, 0xab);
let white = render.makeColor(255, 255, 255);
let black = render.makeColor(0, 0, 0);
let gray = render.makeColor(192, 192, 192);
let backgroundColor = black;
let textColor = white;
let buttonColor = white;

let note_bitmap = parseRLE(new Resource("note-alpha.bm4"));
let pause_bitmap = parseRLE(new Resource("pause-alpha.bm4"));
let play_bitmap = parseRLE(new Resource("play-alpha.bm4"));
let previous_bitmap = parseRLE(new Resource("previous-alpha.bm4"));
let next_bitmap = parseRLE(new Resource("next-alpha.bm4"));

const ART_WIDTH = 110;
const ART_HEIGHT = 110;

const LAYOUT = {
	"note": { x:render.width/2 - note_bitmap.width/2, y:30, bitmap:note_bitmap, width:note_bitmap.width, height:note_bitmap.height },
	"art": { x:render.width/2 - 50, y:20, width:ART_WIDTH, height:ART_HEIGHT },
	"bar": { x:40, y:30 + note_bitmap.height + 20, width:render.width - 80, height:3 },
	"title": { x:15, xTicker:0, font:titleFont, y:render.height/2 + 10, font:titleFont, width: render.width - 30, height:titleFont.height },
	"artist": { x:15, xTicker:0, font:smallFont, y:render.height/2 + 10 + titleFont.height, font:smallFont, width: render.width - 30, height:smallFont.height },
	"album": { x:15, xTicker:0, font:smallFont, y:render.height/2 + 10 + titleFont.height + smallFont.height, font:smallFont, width: render.width - 30, height:smallFont.height },
	"previous": { id:"previous", x:60, y:render.height - 20 - previous_bitmap.height, bitmap:previous_bitmap, width:previous_bitmap.width, height:previous_bitmap.height },
	"playpause": { id:"playpause", x:render.width/2 - play_bitmap.width/2, y:render.height - 20 - play_bitmap.height, width:play_bitmap.width, height:play_bitmap.height },
	"next": { id:"next", x:render.width - 60 - next_bitmap.width, y:render.height - 20 - next_bitmap.height, bitmap:next_bitmap, width:next_bitmap.width, height:next_bitmap.height },
};

const BUTTONS = [LAYOUT.previous, LAYOUT.playpause, LAYOUT.next];

class Rectangle {
	constructor(x, y, w, h) {
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
	}
	contains(x, y) {
		if (x >= this.x && x < this.x + this.w && y >= this.y && y < this.y + this.h)
			return true;
		return false;
	}
}
Object.freeze(Rectangle.prototype);

class AppleMediaServiceClient {
	constructor(render) {
		this.render = render;
	}
	start() {
		this.server = new AMSAuthenticator(this);
	}
	onAuthenticated(device) {
		this.client = new AMSPlayerClient(this.render, device);
	}
}
Object.freeze(AppleMediaServiceClient.prototype);

class AMSPlayerClient extends AMSClient {
	constructor(render, device) {
		super(device);
		this.render = render;
		this.tickers = [];
		this.playing = false;
		this.elapsed = 0;
		this.duration = 0;
	}
	drawMusicIcon() {
		let render = this.render;
		let note = LAYOUT.note;
		let art = LAYOUT.art;
		render.begin(art.x, art.y, art.width, art.height);
			render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
			render.drawGray(note.bitmap, white, note.x, note.y);
		render.end();
	}
	drawAlbumArt(imageData) {
		let render = this.render;
		let decoder = new JPEG(imageData);	
		let x = LAYOUT.art.x;
		let y = LAYOUT.art.y;
		let block;
		while (block = decoder.read()) {
			render.begin(block.x+x, block.y+y, block.width, block.height);
			render.drawBitmap(block, block.x+x, block.y+y);
			render.end();
		}
	}
	drawTransportControls() {
		let render = this.render;
		render.begin(LAYOUT.previous.x, LAYOUT.previous.y, render.width, LAYOUT.previous.height);
			render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
			BUTTONS.forEach(entry => {
				let bitmap = "playpause" == entry.id ? (this.playing ? pause_bitmap : play_bitmap) : entry.bitmap;
				render.drawGray(bitmap, buttonColor, entry.x, entry.y);
			});
		render.end();
	}
	drawProgressBar() {
		let render = this.render;
		let bar = LAYOUT.bar;
		render.begin(bar.x, bar.y, bar.width, bar.height);
			render.fillRectangle(gray, 0, 0, render.width, render.height);
			if (0 != this.duration)
				render.fillRectangle(blue, bar.x, bar.y, bar.width * this.elapsed / this.duration, render.height);
		render.end();
	}
	drawMetadata() {
		let render = this.render;
		let tickers = this.tickers;
		render.begin(LAYOUT.title.x, LAYOUT.title.y, LAYOUT.title.width, LAYOUT.title.height + LAYOUT.artist.height + LAYOUT.album.height);
			render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
			if (!tickers.find(ticker => ticker.id == "title"))
				render.drawText(this.title, LAYOUT.title.font, textColor, (render.width - render.getTextWidth(this.title, LAYOUT.title.font)) >> 1, LAYOUT.title.y, LAYOUT.title.width);
			if (!tickers.find(ticker => ticker.id == "artist"))
				render.drawText(this.artist, LAYOUT.artist.font, textColor, (render.width - render.getTextWidth(this.artist, LAYOUT.artist.font)) >> 1, LAYOUT.artist.y, LAYOUT.artist.width);
			if (!tickers.find(ticker => ticker.id == "album"))
				render.drawText(this.album, LAYOUT.album.font, textColor, (render.width - render.getTextWidth(this.album, LAYOUT.album.font)) >> 1, LAYOUT.album.y, LAYOUT.album.width);
		render.end();
	}
	fetchAlbumURI(path) {
		this.request = new Request({
			host: "itunes.apple.com",
			path, response: String,
			port: 443, Socket: SecureSocket, secure: {protocolVersion: 0x303}
		});
		this.request.callback = this.fetchAlbumURICallback.bind(this);
	}
	fetchAlbumURICallback(message, value, etc) {
		if (5 == message) {
			this.request.close();
			delete this.request;
			let entries = JSON.parse(value, ["resultCount","results","collectionName","artworkUrl100"]);
			if (entries.resultCount > 0) {
				let result = entries.results.find(entry => {
					return (entry.collectionName.startsWith(this.album) && ("artworkUrl100" in entry))
				});
				if (result) {
					let url = result.artworkUrl100.replace(/100x100/, `${ART_WIDTH}x${ART_HEIGHT}`);
					//trace(`album url: ${url}\n`);
					this.fetchAlbumArt(url);
				}
			}
		}
		else if (message < 0) {
			trace("fetch URI failed!\n");
			this.request.close();
			delete this.request;
		}
	}
	fetchAlbumArt(url) {
		let end = url.indexOf("/", 8);
		let host = url.slice(8, end);
		let path = url.slice(end);
		this.request = new Request({
			host, path,
			response: ArrayBuffer,
			port: 443, Socket: SecureSocket, secure: {protocolVersion: 0x303}
		});
		this.request.callback = this.fetchAlbumArtCallback.bind(this);
	}
	fetchAlbumArtCallback(message, value, etc) {
		if (5 == message) {
			this.request.close();
			delete this.request;
			//trace(`length = ${value.byteLength}\n`);
			this.drawAlbumArt(value);
		}
		else if (message < 0) {
			trace("fetch album art failed!\n");
			this.request.close();
			delete this.request;
		}
	}
	onPlaybackInfoChanged(state, rate, elapsed) {
		//trace(`<${state}> <${rate}> <${elapsed}>\n`);
		this.elapsed = elapsed;
		let playing = this.playing;
		if (PlaybackState.Playing == state)
			this.playing = true;
		else if (PlaybackState.Paused == state)
			this.playing = false;
		if (this.playing != playing)
			this.drawTransportControls();
		if (this.playing)
			this.startPlaybackTimer();
		else if (this.playbackTimer) {
			Timer.clear(this.playbackTimer);
			delete this.playbackTimer;
		}
	}
	onTrackChanged(artist, album, title, duration) {
		//trace(`<${title}> <${artist}> <${album}> <${duration}>\n`);
		if (this.artist == artist && this.album == album && this.title == title)
			return;
		if (this.request) {
			this.request.close();
			delete this.request;
		}
		if (this.textTickerTimer) {
			Timer.clear(this.textTickerTimer);
			delete this.textTickerTimer;
		}
		let render = this.render;
		let tickers = this.tickers;
		tickers.length = 0;
		if (render.getTextWidth(title, LAYOUT.title.font) > LAYOUT.title.width)
			tickers.push({ id:"title", entry:LAYOUT.title });
		if (render.getTextWidth(artist, LAYOUT.artist.font) > LAYOUT.artist.width)
			tickers.push({ id:"artist", entry:LAYOUT.artist });
		if (render.getTextWidth(album, LAYOUT.album.font) > LAYOUT.album.width)
			tickers.push({ id:"album", entry:LAYOUT.album });
		tickers.forEach(ticker => ticker.entry.xTicker = ticker.entry.x );
		this.title = title;
		this.artist = artist;
		this.album = album;
		this.elapsed = 0;
		this.duration = duration;
		let term = encodeURIComponent(album.replace(/ /g, "+"));
		let path = `/search?media=music&entity=album&attribute=albumTerm&term=${term}&limit=4`;
		//trace("https://itunes.apple.com" + path + "\n");
		this.startTextTickerTimer();
		this.startButtonTimer();
		this.drawMusicIcon();
		this.drawProgressBar();
		this.drawTransportControls();
		this.drawMetadata();
		this.fetchAlbumURI(path);
	}
	onTouchBegan(x, y) {
		this.button = this.contains(x, y);
		if (this.button) {
			let render = this.render;
			let button = this.button;
			let bitmap = "playpause" == button.id ? (this.playing ? pause_bitmap : play_bitmap) : button.bitmap;
			render.begin(button.x, button.y, button.width, button.height);
				render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
				render.drawGray(bitmap, blue, button.x, button.y);
			render.end();
		}
	}
	onTouchMoved(x, y) {
	}
	onTouchEnded(x, y) {
		if (this.button) {
			let render = this.render;
			let button = this.button;
			let id = this.button.id;
			let bitmap = "playpause" == id ? (this.playing ? pause_bitmap : play_bitmap) : button.bitmap;
			render.begin(button.x, button.y, button.width, button.height);
				render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
				render.drawGray(bitmap, white, button.x, button.y);
			render.end();
			delete this.button;
			if ("previous" == id)
				this.remoteCommand(RemoteCommandID.PreviousTrack);
			else if ("next" == id)
				this.remoteCommand(RemoteCommandID.NextTrack);
			else if ("playpause" == id)
				this.remoteCommand(this.playing ? RemoteCommandID.Pause : RemoteCommandID.Play);
		}
	}
	contains(x, y) {
		let button = BUTTONS.find(button => {
			let bounds = new Rectangle(button.x, button.y, button.width, button.height);
			if (bounds.contains(x, y))
				return true;
		});
		return button;
	}
	startButtonTimer() {
		if (this.buttonTimer)
			Timer.clear(this.buttonTimer);
		this.buttonTimer = Timer.repeat(() => {
			let points = touch.points;
			touch.read(points);
			let point = points[0];
			switch (point.state) {
				case 0:
				case 3:
					if (point.down) {
						delete point.down;
						this.onTouchEnded(point.x, point.y);
						delete point.x;
						delete point.y;
					}
					break;
				case 1:
					if (this.contains(point.x, point.y) && !point.down) {
						point.down = true;
						this.onTouchBegan(point.x, point.y);
					}
					break;
				case 2:
					this.onTouchMoved(point.x, point.y);
					break;
			}
		}, 17);
	}
	startPlaybackTimer() {
		if (this.playbackTimer)
			Timer.clear(this.playbackTimer);
		this.playbackTimer = Timer.repeat(() => {
			this.elapsed += 1;
			if (this.elapsed > this.duration)
				this.elapsed = this.duration;
			this.drawProgressBar();
		}, 1000);
	}
	startTextTickerTimer() {
		if (0 == this.tickers.length)
			return;
		if (this.textTickerTimer)
			Timer.clear(this.textTickerTimer);
		this.textTickerTimer = Timer.repeat(() => {
			let tickers = this.tickers;
			let render = this.render;
			tickers.forEach(ticker => {
				let entry = ticker.entry;
				let text = this[ticker.id] + "   ";
				let font = entry.font;
				let textWidth = render.getTextWidth(text, font);
				render.begin(entry.x, entry.y, entry.width, entry.height);
					render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
					render.drawText(text, font, textColor, entry.xTicker, entry.y);
					if (entry.xTicker + textWidth < entry.width)
						render.drawText(text, font, textColor, entry.xTicker + textWidth, entry.y);
				render.end();
				if (entry.xTicker + textWidth == 0)
					entry.xTicker = 0;
				else
					--entry.xTicker;
			});
		}, 75);
	}
}
Object.freeze(AMSPlayerClient.prototype);

render.begin();
	render.fillRectangle(backgroundColor, 0, 0, render.width, render.height);
render.end();

let client = new AppleMediaServiceClient(render);
client.start();
