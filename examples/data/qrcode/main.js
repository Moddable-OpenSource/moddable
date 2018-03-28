import QRCode from "qrcode";
import Poco from "commodetto/Poco";

// generate QR code
let qr = QRCode({input: "Welcome to Moddable"});
let size = qr.size;
qr = new Uint8Array(qr);

// trace QR code to console
for (let y = 0; y < size; y++) {
	for (let x = 0; x < size; x++) {
		if (qr[(y * size) + x])
			trace("X");
		else
			trace(".");
	}
	trace("\n");
}

// render QR code to screen
let render = new Poco(screen);

render.begin();
render.fillRectangle(render.makeColor(0, 0, 255), 0, 0, render.width, render.height);
render.end();

let margin = 10;
let available = Math.min(render.width - (margin * 2), render.height - (margin * 2));
let pixels = Math.floor(available / size);
margin += (available - (pixels * size)) >> 1;

for (let y = 0; y < size; y++) {
	render.begin(margin, margin + (y * pixels), size * pixels, pixels);
	render.fillRectangle(render.makeColor(255, 255, 255), 0, 0, render.width, render.height);
	for (let x = 0; x < size; x++) {
		if (qr[(y * size) + x])
			render.fillRectangle(render.makeColor(0, 0, 0), margin + (x * pixels), margin + (y * pixels), pixels, pixels);
	}
	render.end();
}
