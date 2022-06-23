/*---
description: 
flags: [onlyStrict]
---*/

const PWM = device.io.PWM;

assert.throws(SyntaxError, () => new PWM, "no options"); 
assert.throws(Error, () => new PWM(1), "invalid options - number"); 
assert.throws(Error, () => new PWM("one"), "invalid options - string"); 
assert.throws(Error, () => new PWM("1n"), "invalid options - BigInt"); 
assert.throws(Error, () => new PWM({}), "invalid options - empty"); 

const pin = $TESTMC.config.pwm.pins[0];
let pwm = new PWM({pin});

assert.throws(Error, () => new PWM({pin}), "pin in use"); 
pwm.close();

pwm = new PWM({pin});		// can allocate after close
pwm.close();

pwm = new PWM({pin: pin.toString()});
pwm.close();

$TESTMC.config.invalidPins.forEach(pin => {
	assert.throws((pin < 0) ? RangeError : Error, () => new PWM({pin}), "invalid pin"); 
});

/*
	port
*/
if ($TESTMC.config.pwm.ports.length) {
	$TESTMC.config.pwm.ports.forEach(port => {
		pwm = new PWM({pin, port});
		pwm.close();
	});

	pwm = new PWM({pin, port: $TESTMC.config.pwm.ports[0]});
	assert.throws(Error, () => new PWM({pin: $TESTMC.config.pwm.pins[1], port: $TESTMC.config.pwm.ports[0]}), "can't re-use port"); 
	pwm.close();
}

/*
	hz
*/
assert.throws(RangeError, () => new PWM({pin, hz: 0}));
assert.throws(RangeError, () => new PWM({pin, hz: -1}));
assert.throws(Error, () => new PWM({pin, hz: 1_000_000}));
assert.throws(TypeError, () => new PWM({pin, hz: Symbol()}));
assert.throws(TypeError, () => new PWM({pin, hz: 1n}));

const hz = $TESTMC.config.pwm.hzs[0];
pwm = new PWM({pin, hz: hz.toString()});
assert.sameValue(hz, pwm.hz, `hz = ${hz} (as string)`);
pwm.close();

pwm = new PWM({pin, hz: hz + 0.1});
assert.sameValue(hz, pwm.hz, `hz = ${hz} (as float)`);
pwm.close();

$TESTMC.config.pwm.hzs.forEach(hz => {
	pwm = new PWM({pin, hz});
	assert.sameValue(hz, pwm.hz, `hz = ${hz}`);
	pwm.close();
});

/*
	resolution
*/
assert.throws(RangeError, () => new PWM({pin, resolution: 0}));
assert.throws(RangeError, () => new PWM({pin, resolution: -1}));
assert.throws(RangeError, () => new PWM({pin, resolution: 1_000_000}));
assert.throws(TypeError, () => new PWM({pin, resolution: Symbol()}));
assert.throws(TypeError, () => new PWM({pin, resolution: 1n}));

const resolution = $TESTMC.config.pwm.resolutions[0];
pwm = new PWM({pin, resolution: resolution.toString()});
assert.sameValue(resolution, pwm.resolution, `resolution = ${resolution} (as string)`);
pwm.close();

pwm = new PWM({pin, resolution: resolution + 0.1});
assert.sameValue(resolution, pwm.resolution, `resolution = ${resolution} (as float)`);
pwm.close();

$TESTMC.config.pwm.resolutions.forEach(resolution => {
	pwm = new PWM({pin, resolution});
	assert.sameValue(resolution, pwm.resolution, `resolution = ${resolution}`);
	pwm.close();
});
