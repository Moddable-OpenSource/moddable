// Test fixture for Wi-Fi tests.
//
// Brings the Wi-Fi interface to a known starting state before a test runs:
// disconnected and using DHCP. This clears any static IP configuration left
// behind by a previous test, since tests run consecutively in the same
// process and share the platform's network interface state.

import WiFi from "embedded:network/interface/wifi";

export default function resetWiFi() {
	const wifi = new WiFi({});
	wifi.disconnect();
	wifi.configure({static: null});
	wifi.close();
}
