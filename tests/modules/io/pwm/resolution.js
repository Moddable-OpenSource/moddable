/*---
description: 
flags: [onlyStrict]
---*/

const PWM = device.io.PWM;

const pin = $TESTMC.config.pwm.pins[0];

class TestPWM extends PWM {
	getResolution() {
		return super.resolution;
	}
}

let pwm = new TestPWM({pin});
assert.throws(SyntaxError, () => pwm.getResolution.call(new $TESTMC.HostObjectChunk), "get resolution invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => pwm.getResolution.call(new $TESTMC.HostObject), "get resolution invalid this - HostObject");
assert.throws(SyntaxError, () => pwm.getResolution.call(new $TESTMC.HostBuffer), "get resolution invalid this - HostBuffer");
pwm.close();

pwm = new PWM({pin});
assert.throws(TypeError, () => pwm.resolution = pwm.resolution, "cannnot set resolution"); 
pwm.close();
