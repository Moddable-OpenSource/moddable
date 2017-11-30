import Timer from "timer";

export default class WiFi  {
	constructor(dictionary, onNotify) {
		Timer.set(() => {
			onNotify("connect");
			onNotify("gotIP");
		}, 500)
	}
	close() {}

	static set mode(value) {
		trace("WiFi mode is ", value, "\n");
	}
	static get mode() {}
	static scan(dictionary, callback) {
		Timer.set(() => {
			for (let i = 0; i < 12; i++)
				  callback({ssid: "M-" + i.toString().padStart(2, "0"), rssi: 127, channel: 1, hidden: false, authentication: (i & 1)  ? "none" : "wpa2_psk"});
			callback(null);
		}, 2000)
	}
	static connect(dictionary) {}
	static accessPoint(dictionary) {}
	static get status(){}
}

Object.freeze(WiFi.prototype);
