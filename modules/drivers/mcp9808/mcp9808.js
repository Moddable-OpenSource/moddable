/*
	Microchip MCP9808 temperature sensor - http://ww1.microchip.com/downloads/en/DeviceDoc/25095A.pdf
*/

import SMBus from "pins/smbus";

class MCP9808 extends SMBus {
	constructor(dictionary) {
		super(Object.assign({address: 0x18}, dictionary));

		if (0x0054 !== super.readWord(6, true))
			throw new Error("invalid manufacturer id");
		if (0x0400 !== super.readWord(7, true))
			throw new Error("unexpected device id / revision");

		super.writeWord(4, 0x0FFF, true);		// max-out T Crit so it never triggers
		super.writeWord(1, 0x0020);				// reset configuration register (clear interrupt!)
	}

	configure(dictionary) {
		if (dictionary.alert) {
			const alert = dictionary.alert;
			if ((undefined !== alert.above) && (undefined !== alert.below)) {
				if (undefined !== alert.above)
					super.writeWord(2, (alert.above * 16) & 0x0FFF, true);			// upper limit		@@ ignoring negative
				else
					super.writeWord(2, 0x0FFF, true);			// upper limit

				if (undefined !== alert.below)
					super.writeWord(3, (alert.below * 16) & 0x0FFF, true);			// lower limit		@@ ignoring negative
				else
					super.writeWord(3, 0x1000, true);			// lower limit

				super.writeWord(1, super.readWord(1, true) | (8 | 1), true);		// enable interrupt
			}
			else
				super.writeWord(1, super.readWord(1, true) & ~(8 | 1), true);		// disable interrupt
		}
	}

	sample() {
		let config = super.readWord(1, true);
		if (config & 0x0010)
			super.writeWord(1, config | 0x0020, true);	// clear interrupt

		const value = super.readWord(5, true);
		let temperature = (value & 0x0FFF) / 16;		//@@ not sure about negative... check data sheet
		if (value & 0x1000)
			temperature -= 256;
		return {temperature};
	}
}
Object.freeze(MCP9808.prototype);

export default MCP9808;
