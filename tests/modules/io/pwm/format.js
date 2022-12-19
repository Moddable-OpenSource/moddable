/*---
description: 
flags: [onlyStrict]
---*/

const PWM = device.io.PWM;

const pin = $TESTMC.config.pwm.pins[0];

let pwm = new PWM({pin, format: "number"});
pwm.close();

pwm = new PWM({pin});
assert.sameValue("number", pwm.format, `default format"`);
pwm.close();

assert.throws(RangeError, () => new PWM({pin, format: "buffer"}), "invalid format - buffer"); 
assert.throws(RangeError, () => new PWM({pin, format: "nonsense"}), "invalid format - nonsense"); 

pwm = new PWM({pin, format: "number"});
assert.sameValue("number", pwm.format, `format = "number"`);

assert.throws(RangeError, () => pwm.format = "buffer", "set invalid format - buffer"); 
assert.sameValue("number", pwm.format, `format = "number"`);

pwm.format = "number";
assert.sameValue("number", pwm.format, `format = "number"`);
	
pwm.close();
