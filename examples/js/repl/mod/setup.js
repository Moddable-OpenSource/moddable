console.log("example mod set-up!")
globalThis.randomInteger = function(range) {
	if (undefined === range)
		throw new RangeError("range required");
	return Math.round(Math.random() * range);
}
