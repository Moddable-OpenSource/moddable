import Poco from "commodetto/Poco";
import {Outline} from "commodetto/outline";
import Timer from "timer";

let poco = new Poco(screen);
let black = poco.makeColor(0, 0, 0);
poco.begin();
	poco.fillRectangle(black, 0, 0, poco.width, poco.height);
poco.end();

Timer.repeat(function() {
	let x = Math.random() * poco.width;
	let y = Math.random() * poco.height;
	let width = (Math.random() * 50) + 5;
	let height = (Math.random() * 50) + 5;
	let color = poco.makeColor(255 * Math.random(), 255 * Math.random(), 255 * Math.random());
	poco.begin(x, y, width, height);
		poco.fillRectangle(~color, x, y, width, height);
		const path = new Outline.CanvasPath();
		const cx = width >> 1;
		const cy = height >> 1;
		path.ellipse(cx, cy, cx, cy, 0, 0, 2 * Math.PI);
		path.closePath();
		const outline = Outline.fill(path);
		poco.blendOutline(color, 255, outline, x, y);
	poco.end();
}, 16);

