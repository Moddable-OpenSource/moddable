/*---
description: 
flags: [onlyStrict]
---*/

const PWM = device.io.PWM;

const pin = $TESTMC.config.pwm.pins[0];

let pwm = new PWM({pin});

assert.throws(SyntaxError, () => pwm.close.call(new $TESTMC.HostObjectChunk), "close invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => pwm.close.call(new $TESTMC.HostObject), "close invalid this - HostObject");
assert.throws(SyntaxError, () => pwm.close.call(new $TESTMC.HostBuffer), "close invalid this - HostBuffer");

pwm.close();

assert.throws(SyntaxError, () => pwm.write(0), "write unavailable after close");

pwm.close();		// mutliple close is allowed
