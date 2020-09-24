import Digital from "pins/digital";
import Monitor from "monitor";
import AXP192 from "axp192";
import SH200Q from "sh200q";
import MPU6886 from "mpu6886";
import I2C from "pins/i2c";
import Timer from "timer";

import config from "mc/config";

const state = {
	handleRotation: nop
};

export default function (done) {
	global.button = {
		a: new Monitor({
			pin: 37,
			mode: Digital.InputPullUp,
			edge: Monitor.Rising | Monitor.Falling
		}),
		b: new Monitor({
			pin: 39,
			mode: Digital.InputPullUp,
			edge: Monitor.Rising | Monitor.Falling
		}),
	};
	button.a.onChanged = button.b.onChanged = nop;

  global.power = new Power();

  state.accelerometerGyro = new IMU();

	global.accelerometer = {
		onreading: nop
	}

	global.gyro = {
		onreading: nop
	}

	//trace('The Temp:', state.accelerometerGyro.sampleTemp(), '\n');

	accelerometer.start = function (frequency) {
		accelerometer.stop();
		state.accelerometerTimerID = Timer.repeat(id => {
			state.accelerometerGyro.configure({
				operation: "accelerometer"
			});
			const sample = state.accelerometerGyro.sample();
			if (sample) {
				sample.y *= -1;
				sample.z *= -1;
				state.handleRotation(sample);
				accelerometer.onreading(sample);
			}
		}, frequency);
	}

	gyro.start = function (frequency) {
		gyro.stop();
		state.gyroTimerID = Timer.repeat(id => {
			state.accelerometerGyro.configure({
				operation: "gyroscope"
			});
			const sample = state.accelerometerGyro.sample();
			if (sample) {
				let {
					x,
					y,
					z
				} = sample;
				const temp = x;
				x = y * -1;
				y = temp * -1;
				z *= -1;
				gyro.onreading({
					x,
					y,
					z
				});
			}
		}, frequency);
	}

	accelerometer.stop = function () {
		if (undefined !== state.accelerometerTimerID)
			Timer.clear(state.accelerometerTimerID);
		delete state.accelerometerTimerID;
	}

	gyro.stop = function () {
		if (undefined !== state.gyroTimerID)
			Timer.clear(state.gyroTimerID);
		delete state.gyroTimerID;
	}

	if (config.autorotate && global.Application) {
		state.handleRotation = function (reading) {
			if (Math.abs(reading.y) > Math.abs(reading.x)) {
				if (reading.y < -0.7 && application.rotation != 90) {
					application.rotation = 90;
				} else if (reading.y > 0.7 && application.rotation != 270) {
					application.rotation = 270;
				}
			} else {
				if (reading.x < -0.7 && application.rotation != 180) {
					application.rotation = 180;
				} else if (reading.x > 0.7 && application.rotation != 0) {
					application.rotation = 0;
				}
			}
		}
		accelerometer.start(300);
	}

	done();
}

function nop() {}

class Power extends AXP192 {
  constructor() {
    super({
      sda: 21,
      scl: 22,
    });
    // TODO: Use class method rather than directly accessing register
    this.write(0x10, 0xff); // OLED VPP Enable
    this.write(0x28, 0xff); // Enable LDO2&LDO3, LED&TFT 3.3V
    this.write(0x82, 0xff); // Enable all the ADCs
    this.write(0x33, 0xc0); // Enable Charging, 100mA, 4.2V End at 0.9
    this.write(0xb8, 0x80); // Enable Colume Counter
    this.write(0x12, 0x4d); // Enable DC-DC1, OLED VDD, 5B V EXT
    this.write(0x36, 0x5c); // PEK
    this.write(0x90, 0x02); // gpio0
  }

  set brightness(brightness) {
    const b = (brightness & 0x0f) << 4;
    this.writeByte(0x28, b);
  }

  /**
   * sets the screen brightness
   * @param {*} brightness brightness between 7-15
   * @deprecated Use setter
   */
  setBrightness(brightness) {
    trace("WARNING: AXP192#setBrightness is deprecated. use setter");
    this.brightness = brightness;
  }

  /**
   * @deprecated
   */
  initialize() {
    trace(
      "WARNING: AXP192#initialize is deprecated. no need to initialize explicitly"
    );
  }
}

class IMU {
	constructor() {
		const probe = new I2C({
			address: 0x68,		// MPU6886
			throw: false
		});
		const result = probe.write(0x75);
		probe.close();

		return (result instanceof Error) ? new SH200Q : new MPU6886;
	}
}
