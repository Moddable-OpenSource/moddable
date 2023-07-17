import Timer from "timer";

export default class UARTServer {
	constructor(target) {
		this.target = target;
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onPasskey();
		}, 2000);
	}
	close() {
		if (this.timer)
			Timer.clear(this.timer);
	}
	onConnected() {
		this.target.defer("onConnected");
	}
	onPasskey() {
		let passkey = Math.round(Math.random() * 999999);
		this.target.defer("onPasskey", passkey.toString().padStart(6, "0"));
		this.timer = Timer.set(() => {
			delete this.timer;
			this.onConnected();
		}, 2000);
	}
	transmit(event) {
	}
}
