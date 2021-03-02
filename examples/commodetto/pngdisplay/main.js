import {Server} from "http";
import MDNS from "mdns";
import PNG from "commodetto/PNG";
import Poco from "commodetto/Poco";
import Bitmap from "commodetto/Bitmap";
import Convert from "commodetto/Convert";
import config from "mc/config";
import Net from "net";
import WiFi from "wifi";

/*
	curl -T $MODDABLE/examples/commodetto/pngdisplay/test.png http://pngdisplay.local/upload

	Details about this application: https://blog.moddable.com/blog/pngdisplay
*/
const HOSTNAME = "pngdisplay";

function callback(message, value, etc) {
	switch (message) {
		case 2:
			this.isPut = "put" === etc.toLowerCase();
			fill(this.isPut ? poco.makeColor(0, 255, 0) : poco.makeColor(255, 0, 0));
			break;
		case 3:
			if ("content-length" === value) {
				this.byteLength = parseInt(etc);
				try {
					this.png = new Uint8Array(new SharedArrayBuffer(this.byteLength));
				}
				catch {
					fill(poco.makeColor(255, 0, 0));
					return;
				}
				this.png.position = 0;
			}
			break;
		case 4:
			return undefined !== this.png;
		case 5:
			if (!this.png) return;
			const data = new Uint8Array(this.read(ArrayBuffer, value));
			this.png.set(data, this.png.position);
			this.png.position += data.byteLength;

			poco.begin(0, (poco.height - 4) >> 1, poco.width * (this.png.position / this.png.byteLength), 4);
			poco.fillRectangle(poco.makeColor(255, 255, 255), 0, 0, poco.width, poco.height);
			poco.end();
			break;
		case 8:
			if (!this.png)
				return {status: 500};
			try {
				render(this.png.buffer);
				delete this.png;
				return {status: 200};
			}
			catch {
				delete this.png;
				return {status: 500};
			}
			break;
	}
}

function render(data) {
	const gray = poco.makeColor(128, 128, 128);
	const png = new PNG(data);
	const width = png.width, height = Math.min(png.height, poco.height), channels = png.channels;

	if ((8 !== png.depth) || ((3 !== channels) && (4 !== channels)) || png.palette)
		return fill(poco.makeColor(255, 0, 0));

	const pixelFormat = Bitmap[config.format];
	const convert = new Convert((3 == channels) ? Bitmap.RGB24 : Bitmap.RGBA32, pixelFormat);
	const scanOut = new ArrayBuffer((width * Bitmap.depth(pixelFormat)) >> 3);
	let bits;
	if ((0 === config.rotation) || (180 == config.rotation))
		bits = new Bitmap(width, 1, pixelFormat, scanOut, 0);
	else
		bits = new Bitmap(1, width, pixelFormat, scanOut, 0);
	const reverse = (config.rotation >= 180) ? new Uint16Array(scanOut) : undefined;

	fill(gray);
	for (let y = 0; y < height; y++) {
		convert.process(png.read().buffer, scanOut);

		reverse?.reverse();

		poco.begin(0, y, width, 1);
		poco.drawBitmap(bits, 0, y);
		poco.end();
	}
}

function fill(color) {
	poco.begin();
	poco.fillRectangle(color, 0, 0, poco.width, poco.height);;
	poco.end();
}

export default function() {
	global.poco = new Poco(screen, {rotation: config.rotation});

	if (!Net.get("SSID", "station")) {
		WiFi.accessPoint({
			ssid: HOSTNAME,
			channel: 8,
		});
	}

	const mdns = new MDNS({hostName: HOSTNAME}, function(message, value) {
		switch (message) {
			case 1:
				fill(poco.makeColor(255, 255, 255));
				trace(`MDNS - claimed hostname is "${value}"\n`);
				break;

			default:
				if (message < 0) {
					trace("MDNS - failed to claim, give up\n");
					fill(poco.makeColor(255, 0, 0));
				}
				break;
		  }
	});
	(new Server).callback = callback;
	fill(poco.makeColor(64, 64, 64));
};
