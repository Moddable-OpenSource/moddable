/*---
description: 
flags: [onlyStrict]
---*/

const PWM = device.io.PWM;

const pin = $TESTMC.config.pwm.pins[0];

let pwm = new PWM({pin});

assert.throws(SyntaxError, () => pwm.write.call(new $TESTMC.HostObjectChunk, 0), "write invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => pwm.write.call(new $TESTMC.HostObject, 0), "write invalid this - HostObject");
assert.throws(SyntaxError, () => pwm.write.call(new $TESTMC.HostBuffer, 0), "write invalid this - HostBuffer");

const max = (1 << pwm.resolution) - 1;
pwm.write(max);
pwm.write(max.toString());
assert.throws(RangeError, () => pwm.write(max * 2), "write -- too big");
assert.throws(RangeError, () => pwm.write(-max), "write -- too small");
assert.throws(TypeError, () => pwm.write(1n), "write -- BigInt");
assert.throws(TypeError, () => pwm.write(Symbol()), "write -- BigInt");

pwm.close();
