/*---
description: 
flags: [onlyStrict]
---*/

const PWM = device.io.PWM;

const pin = $TESTMC.config.pwm.pins[0];

class TestPWM extends PWM {
	getHz() {
		return super.hz;
	}
}

let pwm = new TestPWM({pin});
assert.throws(SyntaxError, () => pwm.getHz.call(new $TESTMC.HostObjectChunk), "get resolution invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => pwm.getHz.call(new $TESTMC.HostObject), "get resolution invalid this - HostObject");
assert.throws(SyntaxError, () => pwm.getHz.call(new $TESTMC.HostBuffer), "get resolution invalid this - HostBuffer");
pwm.close();

pwm = new PWM({pin});
assert.throws(TypeError, () => pwm.hz = pwm.hz, "cannnot set hz"); 
pwm.close();
