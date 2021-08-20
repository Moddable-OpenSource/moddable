import OTA from "ota";
import {Request} from "http";

const host = "YOUR_HOST_HERE";
const path = "/YOUR/PATH/HERE.bin"

const request = new Request({host, path});
request.callback = function(message, value, etc) {
	switch (message) {
		case Request.status:
			if (200 !== value)
				throw new Error("unexpected http status");
			break;

		case Request.header:
			if ("content-length" === value) {
				try {
					this.byteLength = parseInt(etc);
					this.ota = new OTA({byteLength: this.byteLength});
					this.received = 0;
				}
				catch (e) {
					throw new Error("unable to start OTA: " + e);
				}
			}
			break;

		case Request.responseFragment: {
			const bytes = this.read(ArrayBuffer);
			this.received += bytes.byteLength;
			this.ota.write(bytes);
			trace(`received ${this.received} of ${this.byteLength}\n`);
			} break;

		case Request.responseComplete:
			this.ota.complete();
			trace("ota complete\n");
			break;

		default:
			if (message < 0) {
				this.ota.cancel();
				throw new Error("http error");
			}
			break;
	}
}
