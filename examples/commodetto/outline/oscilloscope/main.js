import Poco from "commodetto/Poco";
import {Outline} from "commodetto/outline";
import Timer from "timer";

let poco = new Poco(screen, {rotation:90});
let background = poco.makeColor(0, 0, 0);
let foreground = poco.makeColor(0, 255, 0);

function drawSamplesPolygon(samples, length, y, h) {
	const delta = poco.width / (length - 1);
	const points = new Array(2 * length);
	for (let i = 0; i < length; i++) {
		let j = i << 1;
		points[j] = delta * i;
		points[j + 1] = y + ((samples[i] / 255) * h);
	}
	const path = Outline.PolygonPath(0, y, ...points, poco.width, y);
	const outline = Outline.fill(path);
	poco.begin(0, 0, poco.width, 120);
		poco.fillRectangle(background, 0, 0, poco.width, poco.height);
		poco.blendOutline(foreground, 255, outline);
	poco.end();
}

function drawSamplesOutline(samples, length, y, h) {
	const dx = poco.width / (length - 1);
	const cx = dx / 3;
	let px = 0;
	let py = y + ((samples[0] / 255) * h);
	const path = new Outline.FreeTypePath();
	path.beginSubpath(px, py, true);
	for (let i = 1; i < length; i++) {
		let qx = px + dx;
		let qy =  y + ((samples[i] / 255) * h);
		path.conicTo(px + cx, py, qx - cx, qy, qx, qy);
		px = qx;
		py = qy;
	}
	path.endSubpath();
	const outline = Outline.stroke(path, 2);
	poco.begin(0, 120, poco.width, 120);
		poco.fillRectangle(background, 0, 120, poco.width, 120);
		poco.blendOutline(foreground, 255, outline);
	poco.end();
}

const h = poco.height / 4;
const length = 33;
const samples = new Int8Array(length).fill(0);
Timer.repeat(function() {
	samples.copyWithin(0, 1);
	samples[length - 1] = Math.floor(255 * Math.random()) - 128;
	drawSamplesPolygon(samples, length, h, h);
	drawSamplesOutline(samples, length, 3 * h, h);
}, 50);
