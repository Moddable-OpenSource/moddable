/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc. ,Satoshi Tanaka
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import Analog from "embedded:io/analog";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";
import PulseWidth from "embedded:io/pulsewidth";
import AXP192 from "embedded:peripheral/Power/axp192";
import SH200Q from "embedded:sensor/Accelerometer-Gyroscope/SH200Q";
import MPU6886 from "embedded:sensor/Accelerometer-Gyroscope/MPU6886";
import RTC from "embedded:RTC/BM8563";

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 32,
			clock: 33
		},
		internal: {
			io: I2C,
			data: 21,
			clock: 22
		},
		hat: {
			io: I2C,
			data: 0,
			clock: 26
		}
	},
	Serial: {
		default: {
			io: Serial,
			port: 1,
			receive: 3,
			transmit: 1
		}
	},
	SPI: {
		default: {
			io: SPI,
			clock: 13,
			out: 15,
			port: 1
		}
	},
	Analog: {
		default: {
			io: Analog,
			pin: 36
		}
	},
	io: { Analog, Digital, DigitalBank, I2C, PulseCount, PWM, Serial, SMBus, SPI, PulseWidth },
	pin: {
		button: 37,
		led: 10,
		displayDC: 23,
		displaySelect: 5
	},
	sensor: {
		IMU: class {
			constructor(options) {
				let isMPU6886 = true;
				let probe;
				try {
					probe = new SMBus({
						...device.I2C.internal,
						io: SMBus,
						hz: 400_000,
						address: 0x68,		// MPU6886
					});
					probe.readUint8(0x74);
				} catch {
					isMPU6886 = false;
				} finally {
					probe?.close();
				}

				const IMUBase = isMPU6886 ? MPU6886 : SH200Q;
				const IMUClass = class extends IMUBase {
					sample() {
						const result = super.sample();
						result.accelerometer.x *= -1;
						result.accelerometer.y *= -1;
						result.gyroscope.y *= -1;
						return result;
					}
				};

				return new IMUClass({
					...options,
					sensor: {
						...device.I2C.internal,
						io: device.io.SMBus
					}
				});
			}
		}
	},
	peripheral: {
		Power: class extends AXP192 {
			constructor(options) {
				super({
					...options,
					peripheral: {
						...device.I2C.internal,
						io: device.io.SMBus
					}
				});
				this.writeByte(0x10, 0xff); // OLED VPP Enable
				this.writeByte(0x28, 0xff); // Enable LDO2&LDO3, LED&TFT 3.3V
				this.writeByte(0x82, 0xff); // Enable all the ADCs
				this.writeByte(0x33, 0xc0); // Enable Charging, 100mA, 4.2V End at 0.9
				this.writeByte(0xb8, 0x80); // Enable Colume Counter
				this.writeByte(0x12, 0x4d); // Enable DC-DC1, OLED VDD, 5B V EXT
				this.writeByte(0x36, 0x5c); // 128ms power on, 4s power off
				this.writeByte(0x42, 0x03); // Enable PekIRQ
				this.writeByte(0x91, 0xA0); // Set MIC voltage to 2.8V
				this.writeByte(0x90, 0x02); // gpio0
			}
			// value 0 - 100 %
			set brightness(value) {
				if (value <= 0)
					value = 7;
				else if (value >= 100)
					value = 15;
				else
					value = (value / 100) * 8 + 7;
				this.writeByte(0x28, (this.readByte(0x28) & 0x0F) | (value << 4));
			}
		},
		RTC: class {
			constructor(options) {
				return new RTC({
					...options,
					clock: {
						...device.I2C.internal,
						io: SMBus,
					},
				});
			}
		},
	}
};

export default device;
