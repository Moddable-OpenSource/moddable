globalThis.x = 0;
globalThis.increment = function() {
	return (1,eval)("x++");
}
globalThis.test = function() {
    trace("mod " + increment() + "\n");
}
