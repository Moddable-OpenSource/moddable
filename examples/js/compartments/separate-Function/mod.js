globalThis.x = 0;
globalThis.increment = new Function("return x++");
globalThis.test = function() {
    trace("mod " + increment() + "\n");
}
