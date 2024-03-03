/*---
description: 
flags: [module]
---*/

import WiFi from "wifi";

const options = await $NETWORK.wifi();

if ("off" in WiFi.Mode) {
	WiFi.mode = WiFi.Mode.none;
	assert.sameValue(WiFi.mode, WiFi.Mode.none);  

	WiFi.mode = WiFi.Mode.off;
	assert.sameValue(WiFi.mode, WiFi.Mode.off);  

	// Wi-Fi operations like scan & connect power Wi-Fi back up
	WiFi.scan({}, ap => {
		return $DONE("unexpected callback");
	});
	WiFi.scan();

	assert.sameValue(WiFi.mode, WiFi.Mode.station);  

	WiFi.mode = WiFi.Mode.off;
	assert.sameValue(WiFi.mode, WiFi.Mode.off);  

	let w = new WiFi(options, (msg, value) => {
		throw new RangeError("unexpected");
	});
	w.close();
	WiFi.disconnect();

	assert.sameValue(WiFi.mode, WiFi.Mode.station);  
}
