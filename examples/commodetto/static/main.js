import Poco from "commodetto/Poco";
import Timer from "timer";

class PocoCustom extends Poco {
	fillStatic(x, y, w, h, brightness = 255) @ "xs_poco_fillStatic";
}

const render = new PocoCustom(screen, {pixels: screen.width * 16});

render.begin();
	render.fillRectangle(render.makeColor(0, 0, 0), 0, 0, render.width, render.height);
render.end();

Timer.repeat(() => {
	render.begin(10, 10, render.width - 20, render.height - 20);
		render.fillStatic(0, 0, render.width, render.height);
	render.end();
}, 33);
