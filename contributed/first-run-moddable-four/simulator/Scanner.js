import Timer from "timer";

export default class Scanner {
	constructor(target) {
		this.discoveries = new Map();
		this.timer = Timer.repeat(() => {
			if (this.discoveries.size < 20) {
				let address = "";
				for (let i = 0; i < 6; i++) {
					if (i > 0)
						address += ":";
					const value = Math.round(Math.random() * 255);
					if (value < 16)
						address += "0";
					address += value.toString(16).toUpperCase();
				}
				let rssi = -30 - Math.round(Math.random() * 60);
				this.discoveries.set(address, { address, rssi });
				target.defer("onDiscovered", this.discoveries);
			}
		}, 500);
	}
	close() {
		Timer.clear(this.timer);
	}
}
