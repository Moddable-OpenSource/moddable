class NeoPixel @ "xs_neopixel_destructor" {
	constructor(dictionary) @ "xs_neopixel";

	close() @ "xs_neopixel_close";
	update() @ "xs_neopixel_update";

	makeRGB(r, g, b) @ "xs_neopixel_makeRGB";	// r, g, b are 0 to 255
	makeHSB(h, s, b) @ "xs_neopixel_makeHSB";	// h is 0 to 359, s and b are 0 to 1000

	setPixel(index, color) @ "xs_neopixel_setPixel";
	fill(color, index, count) @ "xs_neopixel_fill";

	set brightness(value) @ "xs_neopixel_brightness_set";
	get brightness() @ "xs_neopixel_brightness_get";

	get length() @ "xs_neopixel_length_get";
	get byteLength() @ "xs_neopixel_byteLength_get";
}
Object.freeze(NeoPixel.prototype);

export default NeoPixel;
