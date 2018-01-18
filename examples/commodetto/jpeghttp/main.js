import {Request} from "http"
import JPEG from "commodetto/readJPEG";
import Poco from "commodetto/Poco";

function fetch(filename) {
	return new Promise((resolve, reject) => {
		let request = new Request({
			host: "www.moddable.com", 
			path: "/example/images/"+filename,
			response: ArrayBuffer });
		
		request.callback = function(message, value, etc) {
			if (2 == message)
				trace(`${value}: ${etc}\n`);
			else if (5 == message) {
				trace(value.byteLength + "\n");
				resolve(value);
			}
		}
	});
}

function render(imageData, offset, poco) {
	let decoder = new JPEG(imageData);		
	let block;
	while (block = decoder.read()) {
		poco.begin(block.x + offset, block.y+offset, block.width, block.height);
		poco.drawBitmap(block, block.x+offset, block.y+offset);
		poco.end();
	}
}

async function main() {
	let poco = new Poco(screen);
	let gray = poco.makeColor(0, 0, 0);
	poco.begin();
	poco.fillRectangle(gray, 0, 0, poco.width, poco.height);
	poco.end();

	render(await fetch('example1.jpg'), 5, poco);
	render(await fetch('example2.jpg'), 35, poco);
	render(await fetch('example3.jpg'), 65, poco);
	render(await fetch('example4.jpg'), 90, poco);
}

export default main;
