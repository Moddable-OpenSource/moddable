import AXP192, { kCHG_100mA } from "axp192";
import MPU6886 from "mpu6886";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";

const INTERNAL_I2C_SDA = 21;
const INTERNAL_I2C_SCL = 22;

const state = {
	handleRotation: nop
};

export default function (done) {
	// power
	global.power = new AXP192({
		sda: INTERNAL_I2C_SDA,
		scl: INTERNAL_I2C_SCL,
		onInit: function () {
			// TODO: encapsulate direct register access by class method
			this.writeByte(0x30, (this.readByte(0x30) & 0x04) | 0x02) //AXP192 30H
			this.writeByte(0x92, this.readByte(0x92) & 0xf8) //AXP192 GPIO1:OD OUTPUT
			this.writeByte(0x93, this.readByte(0x93) & 0xf8) //AXP192 GPIO2:OD OUTPUT
			this.writeByte(0x35, (this.readByte(0x35) & 0x1c) | 0xa3)//AXP192 RTC CHG
			this.setVoltage(3350) // Voltage 3.35V
			this.setLcdVoltage(2800) // LCD backlight voltage 2.80V
			this.setLdoVoltage(2, 3300) //Periph power voltage preset (LCD_logic, SD card)
			this.setLdoVoltage(3, 2000) //Vibrator power voltage preset
			this.setLdoEnable(2, true)
			this.setChargeCurrent(kCHG_100mA)

			//AXP192 GPIO4
			this.writeByte(0x95, (this.readByte(0x95) & 0x72) | 0x84)
			this.writeByte(0x36, 0x4c)
			this.writeByte(0x82, 0xff)

			this.setLcdReset(0);
			Timer.delay(20);
			this.setLcdReset(1);
			Timer.delay(20);

			this.setBusPowerMode(0); //  bus power mode_output
			Timer.delay(200);
		}
	});

	// speaker
	global.power.setSpeakerEnable(true)
	global.speaker = new AudioOut({streams: 4});
	if (config.startupSound) {
		speaker.callback = function() {this.stop()};
		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	// vibration
	global.vibration = {
		read: function() {
			return global.power.getLdoEnable(3)
		},
		write: function(v) {
			global.power.setLdoEnable(3, v)
		}
	}
	vibration.write(true)
	Timer.set(() => {
		vibration.write(false)
	}, 600)

	// accelerometer and gyrometer
	state.accelerometerGyro = new MPU6886({
		sda: INTERNAL_I2C_SDA,
		scl: INTERNAL_I2C_SCL
	});

	global.accelerometer = {
		onreading: nop
	}

	global.gyro = {
		onreading: nop
	}

	accelerometer.start = function (frequency) {
		accelerometer.stop();
		state.accelerometerTimerID = Timer.repeat(id => {
			state.accelerometerGyro.configure({
				operation: "accelerometer"
			});
			const sample = state.accelerometerGyro.sample();
			if (sample) {
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

	// autorotate
	if (config.autorotate && global.Application) {
		state.handleRotation = function (reading) {
			if (Math.abs(reading.y) > Math.abs(reading.x)) {
				if (reading.y < -0.7 && application.rotation != 180) {
					application.rotation = 180;
				} else if (reading.y > 0.7 && application.rotation != 0) {
					application.rotation = 0;
				}
			} else {
				if (reading.x < -0.7 && application.rotation != 270) {
					application.rotation = 270;
				} else if (reading.x > 0.7 && application.rotation != 90) {
					application.rotation = 90;
				}
			}
		}
		accelerometer.start(300);
	}

	done();
}

function nop() {}
