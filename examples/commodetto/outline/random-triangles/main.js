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
		poco.blendPolygon(color, 255, 
			x, y, 
			x + width, y, 
			x + (width >> 1), y + height);
	poco.end();
}, 16);
