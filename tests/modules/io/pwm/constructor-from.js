/*---
description: 
flags: [onlyStrict]
---*/

const PWM = device.io.PWM;

const pin = $TESTMC.config.pwm.pins[0];

if ($TESTMC.config.pwm.from) {
	let pwm, next;

	assert.throws(SyntaxError, () => new PWM({from: 1}));
	assert.throws(SyntaxError, () => new PWM({from: {}}));
	assert.throws(SyntaxError, () => new PWM({from: "123"}));
	assert.throws(SyntaxError, () => new PWM({from: Function}));
	assert.throws(SyntaxError, () => new PWM({from: $TESTMC.HostObject}));
	assert.throws(SyntaxError, () => new PWM({from: $TESTMC.HostObjectChunk}));
	assert.throws(SyntaxError, () => new PWM({from: $TESTMC.HostBuffer}));

	pwm = new PWM({pin});
	next = new PWM({from: pwm});
	assert.throws(SyntaxError, () => pwm.write(0), "original no longer usable");
	pwm.close();
	next.write(0);		// close of original doesn't effect next
	assert.throws(Error, () => new PWM({pin}), "pin should be in use");
	next.close();
	assert.throws(SyntaxError, () => next.write(0), "next no longer usable");

	$TESTMC.config.pwm.pins.forEach(pin => {
		$TESTMC.config.pwm.hzs.forEach(hz => {
			$TESTMC.config.pwm.resolutions.forEach(resolution => {
				const ports = $TESTMC.config.pwm.ports.length ? $TESTMC.config.pwm.ports : [0];		// fake port if none
				ports.forEach(port => {
					pwm = new PWM({pin, port, hz, resolution});
					next = new PWM({from: pwm});
					assert.sameValue(resolution, next.resolution, `resolution = ${resolution}`);
					assert.sameValue(hz, next.hz, `hz = ${hz}`);
					next.close();
				});
			});
		});
	});
	
	pwm = new PWM({pin, hz: $TESTMC.config.pwm.hzs[0]});
	next = new PWM({from: pwm, hz: $TESTMC.config.pwm.hzs.at(-1)});
	assert.sameValue($TESTMC.config.pwm.hzs.at(-1), next.hz, `hz = ${$TESTMC.config.pwm.hzs.at(-1)}}`);
	next.close();

	pwm = new PWM({pin, resolution: $TESTMC.config.pwm.resolutions[0]});
	next = new PWM({from: pwm, resolution: $TESTMC.config.pwm.resolutions.at(-1)});
	assert.sameValue($TESTMC.config.pwm.resolutions.at(-1), next.resolution, `resolution = ${$TESTMC.config.pwm.resolutions.at(-1)}}`);
	next.close();
}
