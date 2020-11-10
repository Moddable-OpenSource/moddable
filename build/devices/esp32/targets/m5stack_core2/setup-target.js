import AXP192 from "axp192";
import MPU6886 from "mpu6886";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";

const INTERNAL_I2C = Object.freeze({
	sda: 21,
	scl: 22
});

const state = {
  handleRotation: nop,
};

export default function (done) {
	// power
	globalThis.power = new Power();

	// speaker
	power.speaker.enable = true;

	// start-up sound
	if (config.startupSound) {
		const speaker = new AudioOut({streams: 1});
		speaker.callback = function () {
			this.stop();
			this.close();
			this.done();
		};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

  // vibration
  globalThis.vibration = {
    read: function () {
      return power.vibration.enable;
    },
    write: function (v) {
      power.vibration.enable = v;
    },
  };

  vibration.write(true);
  Timer.set(() => {
    vibration.write(false);
  }, 600);

  // accelerometer and gyrometer
  state.accelerometerGyro = new MPU6886(INTERNAL_I2C);

  globalThis.accelerometer = {
    onreading: nop,
  };

  globalThis.gyro = {
    onreading: nop,
  };

  accelerometer.start = function (frequency) {
    accelerometer.stop();
    state.accelerometerTimerID = Timer.repeat((id) => {
      state.accelerometerGyro.configure({
        operation: "accelerometer",
      });
      const sample = state.accelerometerGyro.sample();
      if (sample) {
        state.handleRotation(sample);
        accelerometer.onreading(sample);
      }
    }, frequency);
  };

  gyro.start = function (frequency) {
    gyro.stop();
    state.gyroTimerID = Timer.repeat((id) => {
      state.accelerometerGyro.configure({
        operation: "gyroscope",
      });
      const sample = state.accelerometerGyro.sample();
      if (sample) {
        let { x, y, z } = sample;
        const temp = x;
        x = y * -1;
        y = temp * -1;
        z *= -1;
        gyro.onreading({
          x,
          y,
          z,
        });
      }
    }, frequency);
  };

  accelerometer.stop = function () {
    if (undefined !== state.accelerometerTimerID)
      Timer.clear(state.accelerometerTimerID);
    delete state.accelerometerTimerID;
  };

  gyro.stop = function () {
    if (undefined !== state.gyroTimerID) Timer.clear(state.gyroTimerID);
    delete state.gyroTimerID;
  };

  // autorotate
  if (config.autorotate && globalThis.Application) {
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
    };
    accelerometer.start(300);
  }

  done?.();
}

class Power extends AXP192 {
  constructor() {
    super(INTERNAL_I2C);

    // TODO: encapsulate direct register access by class method
    this.writeByte(0x30, (this.readByte(0x30) & 0x04) | 0x02); //AXP192 30H
    this.writeByte(0x92, this.readByte(0x92) & 0xf8); //AXP192 GPIO1:OD OUTPUT
    this.writeByte(0x93, this.readByte(0x93) & 0xf8); //AXP192 GPIO2:OD OUTPUT
    this.writeByte(0x35, (this.readByte(0x35) & 0x1c) | 0xa3); //AXP192 RTC CHG

    // main power line
    this._dcdc1.voltage = 3350;
    this.chargeCurrent = AXP192.CHARGE_CURRENT.Ch_100mA;

    // LCD
    this.lcd = this._dcdc3;
    this.lcd.voltage = 2800;

    // internal LCD logic
    this._ldo2.voltage = 3300;
    this._ldo2.enable = true;

    // Vibration
    this.vibration = this._ldo3;
    this.vibration.voltage = 2000;

    // Speaker
    this.speaker = this._gpio2;

    // AXP192 GPIO4
    this.writeByte(0x95, (this.readByte(0x95) & 0x72) | 0x84);
    this.writeByte(0x36, 0x4c);
    this.writeByte(0x82, 0xff);
    this.resetLcd();
    this.busPowerMode = 0; //  bus power mode_output
    Timer.delay(200);
  }

  resetLcd() {
    this._gpio4.enable = false
    Timer.delay(20);
    this._gpio4.enable = true
  }

  set busPowerMode(mode) {
    if (mode == 0) {
      this.writeByte(0x91, (this.readByte(0x91) & 0x0f) | 0xf0);
      this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x02); //set GPIO0 to LDO OUTPUT , pullup N_VBUSEN to disable supply from BUS_5V
      this.writeByte(0x12, this.readByte(0x12) | 0x40); //set EXTEN to enable 5v boost
    } else {
      this.writeByte(0x12, this.readByte(0x12) & 0xbf); //set EXTEN to disable 5v boost
      this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x01); //set GPIO0 to float , using enternal pulldown resistor to enable supply from BUS_5VS
    }
  }
}

function nop() {}
