import ILI9341 from "ili9341";
import Digital from "pins/digital";

export default class extends ILI9341 {
	constructor(dictionary) {
		super(dictionary);

		if (Digital.read(33))		// TFT_RST
			super.command(0x21);	// invert
	}
}
