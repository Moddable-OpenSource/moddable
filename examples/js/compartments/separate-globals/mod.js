globalThis.x = 0;
globalThis.increment = function() {
    return x++;
}
globalThis.test = function() {
    trace("mod " + increment() + "\n");
}
