import SNTP from "network/sntp";

new SNTP({
	host: "pool.ntp.org",
	onTime(value) {
		trace(new Date(value), "\n");
	},
	onError() {
		debugger;
	},
});
