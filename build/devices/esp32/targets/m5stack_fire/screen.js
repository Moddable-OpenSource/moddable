import ILI9341 from "ili9341";
import Digital from "pins/digital";

export default class extends ILI9341 {
	constructor(dictionary) {
		let pin33 = new Digital(33, Digital.InputPullDown);	// TFT_RST
		let isIps = pin33.read();	// Ips LCD version is pullup externally 
		pin33.mode(Digital.Output);	// change digital output mode
		pin33.write(1);
		super(dictionary);
		if (isIps)
			super.command(0x21);	// invert
	}
}