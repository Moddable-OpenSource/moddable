import Bitmap from "commodetto/Bitmap";
import ReadGIF from "commodetto/ReadGIF";
import Resource from "Resource";

const gifImage = {
	__proto__: Content.prototype,
	_create($, it) @ "PiuGIFImage_create",

	load(path, colorized) {
		const gif = new ReadGIF(("string" === typeof path) ? new Resource(path) : path, colorized ? {pixelFormat: Bitmap.Gray16} : {});
		this.duration = gif.duration;
		this.time = 0;
		return gif;
	},
	unload(gif) {
		gif.close();
	},
};
export const GIFImage = Template(gifImage);
Object.freeze(gifImage);
globalThis.GIFImage = GIFImage;
export default GIFImage;