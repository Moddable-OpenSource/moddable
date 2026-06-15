/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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

/*
        "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270": "$(MODDABLE)/modules/drivers/sensors/bmi270/bmi270"

3-axis accelerometer and gyroscope
	[BMI270](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf)
3-axis magnetometer
	[BMM150](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmm150-ds001.pdf)

*/

import Timer from "timer";

const Register = Object.freeze({
	CHIP_ID_ADDR:			0x00,
	ERR_REG_ADDR:			0x02,
	STATUS_ADDR:			0x03,
	AUX_X_LSB_ADDR:         0x04,
	ACC_X_LSB_ADDR:         0x0C,
	GYR_X_LSB_ADDR:         0x12,
	SENSORTIME_ADDR:		0x18,
	INT_STATUS_0_ADDR:      0x1C,
	INT_STATUS_1_ADDR:      0x1D,
	SC_OUT_0:               0x1F,
	WR_GEST_ACT:            0x20,
	INTERNAL_STATUS_ADDR:	0x21,
	TEMPERATURE_0_ADDR:     0x22,
	FEAT_PAGE_ADDR:			0x2F,
	FEATURES_ADDR:			0x30,
		// page 0
		ACT_OUT: 0x34,
		// page 1
		GEN_SET_1: 0x34,
		ANYMO_1: 0x3c,
		ANYMO_2: 0x3e,
		// page 2
		NOMO_1: 0x30,
		NOMO_2: 0x32,
		SIGMO_1: 0x34,
		SIGMO_2: 0x3E,
		// page 6
		SC_26: 0x32,
		WR_GEST1: 0x36,
		WR_GEST2: 0x38,
		WR_GEST3: 0x3A,
		WR_GEST4: 0x3C,
		// page 7
		WR_WAKEUP1: 0x30,
		WR_WAKEUP2: 0x32,
		WR_WAKEUP3: 0x34,
		WR_WAKEUP4: 0x36,
		WR_WAKEUP5: 0x38,
		WR_WAKEUP6: 0x3A,
		WR_WAKEUP7: 0x3C,
	ACC_RANGE_ADDR:			0x41,
	GYR_RANGE_ADDR:			0x43,
	AUX_DEV_ID_ADDR:		0x4B,
	AUX_IF_CONF_ADDR:		0x4C,
	AUX_RD_ADDR:			0x4D,
	AUX_WR_ADDR:			0x4E,
	AUX_WR_DATA_ADDR:		0x4F,
	INT_LATCH_ADDR: 		0x55,
	INT1_MAP_FEAT_ADDR:     0x56,
	INT2_MAP_FEAT_ADDR:     0x57,
	INT_MAP_DATA_ADDR:		0x58,
	INIT_CTRL_ADDR:			0x59,
	INIT_ADDR_0:			0x5B,
	INIT_DATA_ADDR:			0x5E,
	IF_CONF_ADDR:			0x6B,
	PWR_CONF_ADDR:			0x7C,
	PWR_CTRL_ADDR:			0x7D,
	CMD_REG_ADDR:			0x7E,
		SOFT_RESET_CMD:			0xB6,
	AUX_TRIM_REGISTERS:		0x5D
});

const Sensor = Object.freeze({
	IMU_AUX:	0x01,
	IMU_GYRO:   0x02,
	IMU_ACCEL:  0x04,
	IMU_TEMP:   0x08
});

const Status = Object.freeze({
	AUX_BUSY:	0b0000_0100,
	CMD_RDY:		0b0001_0000,
	DRDY_AUX:	0b0010_0000,
	DRDY_GYR:	0b0100_0000,
	DRDY_ACC:	0b1000_0000,
});

/*
const Config = Object.freeze({
	Enable: {
		Accelerometer: 0b0100,
		Gyroscope:     0b0010,
		Magnetometer:  0b0001,
		Thermometer:   0b1000
	},
	Accel_Range: {
		RANGE_2_G:   0b00,
		RANGE_4_G:   0b01,
		RANGE_8_G:   0b10,
		RANGE_16_G:  0b11
	},
	Accel_DataRate: {
		DATARATE_1600_HZ:   0x0C,
		DATARATE_800_HZ:    0x0B,
		DATARATE_400_HZ:    0x0A,
		DATARATE_200_HZ:    0x09,
		DATARATE_100_HZ:    0x08,
		DATARATE_50_HZ:     0x07,
		DATARATE_25_HZ:     0x06,
		DATARATE_12p5_HZ:   0x05,
		DATARATE_6p25_HZ:   0x04,
		DATARATE_3p1_HZ:    0x03,
		DATARATE_1p5_HZ:    0x02,
		DATARATE_0p78_HZ:   0x01
	},
	Gyro_Range: {
		RANGE_2000:  0b00,
		RANGE_1000:  0b01,
		RANGE_500:   0b10,
		RANGE_250:   0b11,
		RANGE_125:  0b100
	},
	Gyro_DataRate: {
		DATARATE_25_HZ:     0x06,
		DATARATE_50_HZ:     0x07,
		DATARATE_100_HZ:    0x08,
		DATARATE_200_HZ:    0x09,
		DATARATE_400_HZ:    0x0A,
		DATARATE_800_HZ:    0x0B,
		DATARATE_1600_HZ:   0x0C,
		DATARATE_3200_HZ:   0x0D,
	}
}, true);
*/

const Gconversion = 9.80665;
const ACCEL_SCALER = Object.freeze([
		Gconversion * (2.0 / 32768),
		Gconversion * (4.0 / 32768),
		Gconversion * (8.0 / 32768),
		Gconversion * (16.0 / 32768)
]);
const GYRO_SCALER = Object.freeze([
		(2000 / 32768),
		(1000 / 32768),
		(500 / 32768),
		(250 / 32768),
		(125 / 32768)
]);


export default class BMI270 {
	#io;
	#available = 0;
	#enabled = 0;
	#view;			// 24 bytes
	#tempView;		// 2 bytes
	#accRange = 0x02;
	#gyroRange = 0x00;
	#trim;
	#trimData;		// 16 bytes (can share with featureView?)
	#featPage;
	#featureView;	// 16 bytes
	#onSignificantMotion;
	#onStepCount;
	#onActivityChange;
	#onWristWakeup;
	#onWristGesture;
	#onNoMotion;
	#onAnyMotion;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x68,			// or 0x69
			...options.sensor
		});

		if (io.readUint8(Register.CHIP_ID_ADDR) != 0x24)
			xsError("not BMI270");
		
		io.writeUint8(Register.CMD_REG_ADDR, Register.SOFT_RESET_CMD);

		this.#available = Sensor.IMU_ACCEL | Sensor.IMU_GYRO | Sensor.IMU_TEMP;

		this.#view = new DataView(new ArrayBuffer(24));
		this.#tempView = new DataView(new ArrayBuffer(2));
		this.#featureView = new DataView(new ArrayBuffer(16));

		Timer.delay(10);	// let reset

		this.#writeUint8_sync(Register.PWR_CONF_ADDR, 0x00);	// disable power_save

		this.#uploadConfig();

		while (1 !== io.readUint8(Register.INTERNAL_STATUS_ADDR))
			Timer.delay(1);	// wait for asic init_ok

		if (options.sensor.enable & Sensor.IMU_AUX) {
		try {
			// try to setup attached BMM150
			this.#auxSetupMode(0x10);
			this.#auxWriteUint8(0x4B, 0x83);		// SW Reset & power on
			this.#auxReadUint8(0x40);				// Whoami?
			if (0x32 === this.#auxReadUint8(0x40)) {
				this.#available |= Sensor.IMU_AUX;

				this.#writeUint8_sync(Register.AUX_IF_CONF_ADDR, 0x8F);		// Manual + burst 8

				this.#trimData = new DataView(new ArrayBuffer(16));
				this.#trim = {};
				this.#auxReadBuffer(Register.AUX_TRIM_REGISTERS, this.#trimData);

				this.#trim.dig_x1 = this.#trimData.getUint8(0);
				this.#trim.dig_y1 = this.#trimData.getUint8(1);
				this.#trim.dig_x2 = this.#trimData.getUint8(4);
				this.#trim.dig_y2 = this.#trimData.getUint8(5);
				let msb = this.#trimData.getUint8(9) << 8;
				this.#trim.dig_z1 = msb | this.#trimData.getUint8(8);
				msb = this.#trimData.getUint8(7) << 8;
				this.#trim.dig_z2 = msb | this.#trimData.getUint8(6);
				msb = this.#trimData.getUint8(13) << 8;
				this.#trim.dig_z3 = msb | this.#trimData.getUint8(12);
				msb = this.#trimData.getUint8(3) << 8;
				this.#trim.dig_z4 = msb | this.#trimData.getUint8(2);
				this.#trim.dig_xy1 = this.#trimData.getUint8(15);
				this.#trim.dig_xy2 = this.#trimData.getUint8(14);
				msb = (this.#trimData.getUint8(11) & 0x7f) << 8;
				this.#trim.dig_xyz1 = msb | this.#trimData.getUint8(10);
				

				this.#auxWriteUint8(0x4C, 0x38);		// normal mode, ODR 30Hz
			
				this.#writeUint8_sync(Register.AUX_IF_CONF_ADDR, 0x4F);		// FCU_WRITE_EN + burst len 8
				this.#writeUint8_sync(Register.AUX_RD_ADDR, 0x42);			// BMM150 I2C Data X LSB
			}
		}
		catch {
			/* this block intentionally left blank */
		}
		}

		this.#onSignificantMotion = options.onSignificantMotion;
		this.#onStepCount = options.onStepCount;
		this.#onActivityChange = options.onActivityChange;
		this.#onWristWakeup = options.onWristWakeup;
		this.#onWristGesture = options.onWristGesture;
		this.#onNoMotion = options.onNoMotion;
		this.#onAnyMotion = options.onAnyMotion;

		if (options.sensor.enable)
			this.#enabled = options.sensor.enable;
		else
			this.#enabled = this.#available;

		this.#writeUint8_sync(Register.PWR_CTRL_ADDR, this.#enabled);
	}

	#uploadConfig() {
		const io = this.#io;
		this.#writeUint8_sync(Register.INIT_CTRL_ADDR, 0x00);

		// upload config file
		io.writeBuffer(Register.INIT_DATA_ADDR, this.BMI270_config());

		this.#writeUint8_sync(Register.INIT_CTRL_ADDR, 0x01);
	}

	#updateFeaturePage(page) {
		this.#featPage = page;
		this.#writeUint8_sync(Register.FEAT_PAGE_ADDR, page);
		this.#io.readBuffer(Register.FEATURES_ADDR, this.#featureView);
	}

	#readFeature16(register) {
		const ret = register - 0x30;
		return this.#featureView.getInt16(ret, true);
	}

	#writeFeature16(page, address, value) {
		if (page != this.#featPage) {
			this.#featPage = page;
			this.#writeUint8_sync(Register.FEAT_PAGE_ADDR, page);
		}
		this.#writeUint16_sync(address, value);
	}

	configure(options) {
		let opt;
		let value;

		// general settings
		if ((undefined !== options.order) || (undefined !== options.invert)) {
			this.#updateFeaturePage(1);
			value = this.#readFeature16(Register.GEN_SET_1);

			switch(options.order) {
				case "xyz": value = 0b00_010_001_000; break;
				case "xzy": value = 0b00_001_010_000; break;
				case "yxz": value = 0b00_010_000_001; break;
				case "yzx": value = 0b00_000_010_001; break;
				case "zxy": value = 0b00_001_000_010; break;
				case "zyx": value = 0b00_000_001_010; break;
			}

			if (options.invert) {
				for (let i=0; i<options.invert.length; i++) {
					if (options.invert[i] === "x") value |= 0b00_000_000_100;
					if (options.invert[i] === "y") value |= 0b00_000_100_000;
					if (options.invert[i] === "z") value |= 0b00_100_000_000;
				}
			}

			this.#writeFeature16(1, Register.GEN_SET_1, value);
		}
		if (undefined !== options.latched)
			this.#writeUint8_sync(Register.INT_LATCH_ADDR, options.latched);

		// accelerometer
		opt = options.accelerometer;
		if (opt?.range) {
			this.#accRange = opt.range & 0b11;
			this.#writeUint8_sync(Register.ACC_RANGE_ADDR, this.#accRange);
		}

		// gyroscope
		opt = options.gyroscope;
		if (opt?.range) {
			this.#gyroRange = opt.range & 0b111;
			this.#writeUint8_sync(Register.GYR_RANGE_ADDR, this.#gyroRange);
		}

		// *** Feature config
			// interrupt 1 for data
		this.#writeUint8_sync(Register.INT_MAP_DATA_ADDR, 0x0F);
		this.#writeUint8_sync(Register.INT1_MAP_FEAT_ADDR, 0x00);

			// interrupt 2 for features
		this.#writeUint8_sync(Register.INT2_MAP_FEAT_ADDR, 0xFF);

		// any motion
		opt = options.anyMotion;
		if (opt) {
			this.#updateFeaturePage(1);
			value = this.#readFeature16(Register.ANYMO_1);
			if (opt.duration) {
				value &= 0b1110_0000_0000_0000;
				value |= opt.duration & 0b0001_1111_1111_1111;
			}
			if (opt.axis) {
				value &= 0b0001_1111_1111_1111;
				value |= (opt.axis & 0b111) << 13;
			}
			this.#writeFeature16(1, Register.ANYMO_1, value);
			value = this.#readFeature16(Register.ANYMO_2);
			if (undefined !== opt.threshold) {
				value &= 0b1111_1000_0000_0000;
				value |= (opt.threshold & 0b0000_0111_1111_1111);
			}
			if (undefined !== opt.enable) {
				if (opt.enable)
					value |= 0b1000_0000_0000_0000;
				else
					value &= 0b0111_1111_1111_1110;
			}
			this.#writeFeature16(1, Register.ANYMO_2, value);

		}
		// no motion
		opt = options.noMotion;
		if (opt) {
			this.#updateFeaturePage(2);
			value = this.#readFeature16(Register.NOMO_1);
			if (undefined !== opt.duration) {
				value &= 0b1110_0000_0000_0000;
				value |= opt.duration & 0b0001_1111_1111_1111;
			}
			if (undefined !== opt.axis) {
				value &= 0b0001_1111_1111_1111;
				value |= (opt.axis & 0b111) << 13;
			}
			this.#writeFeature16(2, Register.NOMO_1, value);
			value = this.#readFeature16(Register.NOMO_2);
			if (undefined !== opt.threshold) {
				value &= 0b1111_1000_0000_0000;
				value |= (opt.threshold & 0b0000_0111_1111_1111);
			}
			if (undefined !== opt.enable) {
				if (opt.enable)
					value |= 0b1000_0000_0000_0000;
				else
					value &= 0b0111_1111_1111_1111;
			}
			this.#writeFeature16(2, Register.NOMO_2, value);
		}
		// significant motion
		opt = options.significantMotion;
		if (opt) {
			if (undefined !== opt.blockSize)
				this.#writeFeature16(2, Register.SIGMO_1, opt.blockSize);
			if (undefined !== opt.enable) {
				this.#updateFeaturePage(2);
				value = this.#readFeature16(Register.SIGMO_2) & 0b1;
				if (opt.enable)
					value |= 0b0000_0000_0000_0001;
				else
					value &= 0b1111_1111_1111_1110;
				this.#writeFeature16(2, Register.SIGMO_2, value);
			}
					
		}
		//  step count
		opt = options.stepCounter;
		if (opt) {
			this.#updateFeaturePage(6);
			value = this.#readFeature16(Register.SC_26);
			if (undefined !== opt.watermark_level) {
				value &= 0b1111_1100_0000_0000;
				value |= (opt.watermark_level & 0b0000_0011_1111_1111);
			}
			if (undefined !== opt.reset_counter) {
				if (opt.reset_counter)
					value |= 0b0000_0100_0000_0000;
				else
					value &= 0b1111_1011_1111_1111;
			}
			if (undefined !== opt.enable_detector) {
				if (opt.enable_detector)
					value |= 0b0000_1000_0000_0000;
				else
					value &= 0b1111_0111_1111_1111;
			}
			if (undefined !== opt.enable_counter) {
				if (opt.enable_counter)
					value |= 0b0001_0000_0000_0000;
				else
					value &= 0b1110_1111_1111_1111;
			}
			if (undefined !== opt.enable_activity) {
				if (opt.enable_activity)
					value |= 0b0010_0000_0000_0000;
				else
					value &= 0b1101_1111_1111_1111;
			}
			this.#writeFeature16(6, Register.SC_26, value);
		}
		//  step activity
		opt = options.activityChange;
		if (opt) {
			this.#updateFeaturePage(6);
			value = this.#readFeature16(Register.SC_26);
			if (undefined !== opt.enable) {
				if (opt.enable)
					value |= 0b0010_0000_0000_0000;
				else
					value &= 0b1101_1111_1111_1111;
			}
			this.#writeFeature16(6, Register.SC_26, value);
		}
		// Wrist Wakeup
		opt = options.wristWakeup;
		if (opt) {
			if (undefined !== opt.min_angle_focus)
				this.#writeFeature16(7, Register.WR_WAKEUP2, opt.min_angle_focus);
			if (undefined !== opt.min_angle_nonfocus)
				this.#writeFeature16(7, Register.WR_WAKEUP3, opt.min_angle_nonfocus);
			if (undefined !== opt.max_tilt_lr)
				this.#writeFeature16(7, Register.WR_WAKEUP4, opt.max_tilt_lr);
			if (undefined !== opt.max_tilt_ll)
				this.#writeFeature16(7, Register.WR_WAKEUP5, opt.max_tilt_ll);
			if (undefined !== opt.max_tilt_pd)
				this.#writeFeature16(7, Register.WR_WAKEUP6, opt.max_tilt_pd);
			if (undefined !== opt.max_tilt_pu)
				this.#writeFeature16(7, Register.WR_WAKEUP7, opt.max_tilt_pu);
			if (undefined !== opt.enable) {
				this.#updateFeaturePage(7);
				value = this.#readFeature16(Register.WR_WAKEUP1);
				if (opt.enable)
					value |= 0b0000_0000_0001_0000;
				else
					value &= 0b1111_1111_1110_1111;
				this.#writeFeature16(7, Register.WR_WAKEUP1, value);
			}
		}
		// Wrist Gesture
		opt = options.wristGesture;
		if (opt) {
			this.#updateFeaturePage(6);
			value = this.#readFeature16(Register.WR_GEST1);
			if (undefined !== opt.wearable_arm) {
				if (opt.wearable_arm)
					value |= 0b0000_0000_0001_0000;
				else
					value &= 0b1111_1111_1110_1111;
			}
			if (undefined !== opt.enable) {
				if (opt.enable)
					value |= 0b0000_0000_0010_0000;
				else
					value &= 0b1111_1111_1101_1111;
			}
			this.#writeFeature16(6, Register.WR_GEST1, value);
			if (undefined !== opt.min_flick_peak)
				this.#writeFeature16(6, Register.WR_GEST2, opt.min_flick_peak);
			if (undefined !== opt.min_flick_samples)
				this.#writeFeature16(6, Register.WR_GEST3, opt.min_flick_samples);
			if (undefined !== opt.max_duration)
				this.#writeFeature16(6, Register.WR_GEST4, opt.max_duration);
		}

	}

	enable(sensors) {
		this.#enabled |= sensors;
		this.#writeUint8_sync(Register.PWR_CTRL_ADDR, this.#enabled);
	}

	disable(sensors) {
		this.#enabled &= ~sensors;
		this.#writeUint8_sync(Register.PWR_CTRL_ADDR, this.#enabled);
	}
/*
#traceStatus(status) {
	let a = "";
	if (status & 0b0000_0100) a += "B";
	if (status & 0b0001_0000) a += "C";
	if (status & 0b0010_0000) a += "M";
	if (status & 0b0100_0000) a += "G";
	if (status & 0b1000_0000) a += "A";
	return a;
}

#traceInt0(intstat) {
	let a = "";
	if (intstat & 0b0000_0001) a += "Sigmo";
	if (intstat & 0b0000_0010) a += "Step";
	if (intstat & 0b0000_0100) a += "Act";
	if (intstat & 0b0000_1000) a += "Wake";
	if (intstat & 0b0001_0000) a += "Gesture";
	if (intstat & 0b0010_0000) a += "No";
	if (intstat & 0b0100_0000) a += "Any";
	return a;
}

#traceInt1(intstat) {
	let a = "";
	if (intstat & 0b0000_0001) a += "F";
	if (intstat & 0b0000_0010) a += "W";
	if (intstat & 0b0000_0100) a += "E";
	if (intstat & 0b0010_0000) a += "M";
	if (intstat & 0b0100_0000) a += "G";
	if (intstat & 0b1000_0000) a += "A";
	return a;
}

#traceError(error) {
	let a = "";
	if (error & 0b0000_0001) a + "x";
	if (error & 0b0001_1110) a + "I";
	if (error & 0b0100_0000) a + "F";
	if (error & 0b1000_0000) a + "M";
	return a;
}
*/

	#triggerFeatures(intstat) {
		const io = this.#io;

		if (intstat & 0b0000_0001) {
			// significant motion
			if (this.#onSignificantMotion)
				this.#onSignificantMotion();
		}
		if (intstat & 0b0000_0010) {
			// step counter
			const stepCount = io.readUint16(Register.SC_OUT_0, true);
			if (this.#onStepCount)
				this.#onStepCount(stepCount);
		}
		if (intstat & 0b0000_0100) {
			// step activity
			const act = (io.readUint8(Register.WR_GEST_ACT) & 0b1_1000) >> 3;
//			this.#updateFeaturePage(0);
//			const act = this.#readFeature16(Register.ACT_OUT);
			if (this.#onActivityChange)
				this.#onActivityChange(act);
		}
		if (intstat & 0b0000_1000) {
			// wrist wakeup
			const gest = (io.readUint8(Register.WR_GEST_ACT) & 0b111);
			if (this.#onWristWakeup)
				this.#onWristWakeup(gest);
		}
		if (intstat & 0b0001_0000) {
			// wrist gesture
			const gest = (io.readUint8(Register.WR_GEST_ACT) & 0b111);
			if (this.#onWristGesture)
				this.#onWristGesture(gest);
		}
		if (intstat & 0b0010_0000) {
			// no motion
			if (this.#onNoMotion)
				this.#onNoMotion();
		}
		if (intstat & 0b0100_0000) {
			// any motion
			if (this.#onAnyMotion)
				this.#onAnyMotion();
		}
	}

	sample() {
		const io = this.#io;
		const view = this.#view;

		const intstat0 = io.readUint8(Register.INT_STATUS_0_ADDR);
//		const intstat1 = io.readUint8(Register.INT_STATUS_1_ADDR);
		const ready = io.readUint8(Register.STATUS_ADDR);
//		const error = io.readUint8(Register.ERR_REG_ADDR);
//		const internalStat = io.readUint8(Register.INTERNAL_STATUS_ADDR);

// trace(`ready: [${this.#traceStatus(ready)}] int0: [${this.#traceInt0(intstat0)}] int1: [${this.#traceInt1(intstat1)}] err: [${this.#traceError(error)}] ${internalStat}\n`);

		io.readBuffer(Register.AUX_X_LSB_ADDR, view);
		

		let result = {};
		if ((ready & Status.DRDY_AUX) && (this.#enabled & Sensor.IMU_AUX)) {
			let x = (view.getInt8(1) * 32) | view.getInt8(0);
			let y = (view.getInt8(3) * 32) | view.getInt8(2);
			let z = (view.getInt8(5) * 128) | view.getInt8(4);
			let r = (view.getInt8(7) << 6) | view.getInt8(6);

			result.magnetometer = {
				x: this.#compensateX(x, r),
				y: this.#compensateY(y, r),
				z: this.#compensateZ(z, r),
			};
		}
		if ((ready & Status.DRDY_GYR) && (this.#enabled & Sensor.IMU_GYRO)) {
			result.gyroscope = {
				x: view.getInt16(14, true) * GYRO_SCALER[this.#gyroRange],
				y: view.getInt16(16, true) * GYRO_SCALER[this.#gyroRange],
				z: view.getInt16(18, true) * GYRO_SCALER[this.#gyroRange]
			};
		}
		if ((ready & Status.DRDY_ACC) && (this.#enabled & Sensor.IMU_ACCEL)) {
			result.accelerometer = {
				x: view.getInt16(8, true) * ACCEL_SCALER[this.#accRange],
				y: view.getInt16(10, true) * ACCEL_SCALER[this.#accRange],
				z: view.getInt16(12, true) * ACCEL_SCALER[this.#accRange]
			};
		}
		if (this.#enabled & Sensor.IMU_TEMP) {
			io.readBuffer(Register.TEMPERATURE_0_ADDR, this.#tempView);
			let temp = this.#tempView.getInt16(0, true);
			temp = 23.0 + temp / 512.0;
			result.thermometer = {
				temperature: temp
			};
		}

		this.#triggerFeatures(intstat0);

		return result;
	}

	BMI270_config() { return native("xs_bmi270_config_file").call(this); }

	// BMM150 connected to the BMI270
	#auxSetupMode(addr) {
		this.#writeUint8_sync(Register.IF_CONF_ADDR, 0x20);		// enable aux i2c
		this.#writeUint8_sync(Register.PWR_CONF_ADDR, 0x00);	// disable powersave
		this.#writeUint8_sync(Register.PWR_CTRL_ADDR, 0x0E);	// disable aux sensor
		this.#writeUint8_sync(Register.AUX_IF_CONF_ADDR, 0x80);	// disable aux sensor
		return this.#writeUint8_sync(Register.AUX_DEV_ID_ADDR, addr << 1);
	}

	#auxWriteUint8(reg, data) {
		this.#writeUint8_sync(Register.AUX_WR_DATA_ADDR, data);
		return this.#writeUint8_sync(Register.AUX_WR_ADDR, reg)
	}

	#auxReadUint8(reg) {
		const io = this.#io;
		this.#writeUint8_sync(Register.AUX_IF_CONF_ADDR, 0x80);		// enable r/w, burst 1
		this.#writeUint8_sync(Register.AUX_RD_ADDR, reg);
		return io.readUint8(Register.AUX_X_LSB_ADDR);
	}

	#auxReadBuffer(reg, buffer) {
		const io = this.#io;
		let len = buffer.byteLength;
		let addr = reg;
		let loc = 0;

		this.#writeUint8_sync(Register.AUX_IF_CONF_ADDR, 0x8F);		// enable r/w, burst 8
		while (len > 0) {
			let burst = 8;
			this.#writeUint8_sync(Register.AUX_RD_ADDR, addr);		// set where to read from
			while (len > 0 && burst-- > 0)
				buffer.setUint8(loc++, io.readUint8(Register.AUX_X_LSB_ADDR + burst));
			len -= 8;
			addr += 8;
		}
	}

	#writeUint8_sync(reg, data) {
		const io = this.#io;
		io.writeUint8(reg, data);

		let retry = 3;
		while ((io.readUint8(Register.STATUS_ADDR) & 0b100) && retry--)
			Timer.delay(1);

		return 1;
	}

	#writeUint16_sync(reg, data, endian=false) {
		const io = this.#io;
		io.writeUint16(reg, data, endian);

		let retry = 3;
		while ((io.readUint8(Register.STATUS_ADDR) & 0b100) && retry--)
			Timer.delay(1);

		return 1;
	}

	#compensateX(x, r) {
		const process_comp_x0 = this.#trim.dig_xyz1 * 16384.0 / r;
		let ret = process_comp_x0 - 16384.0;
		const process_comp_x1 = this.#trim.dig_xy2 * (ret * ret / 268435456.0);
		const process_comp_x2 = process_comp_x1 + ret * this.#trim.dig_xy1 / 16384.0;
		const process_comp_x3 = this.#trim.dig_x2 + 160.0;
		const process_comp_x4 = x * ((process_comp_x2 + 256.0) * process_comp_x3);
		ret = ((process_comp_x4 / 8192.0) + ((this.#trim.dig_x1) * 8.0)) / 16.0;

		return ret;
	}
	#compensateY(y, r) {
		const process_comp_y0 = this.#trim.dig_xyz1 * 16384.0 / r;
		let ret = process_comp_y0 - 16384.0;
		const process_comp_y1 = this.#trim.dig_xy2 * (ret * ret / 268435456.0);
		const process_comp_y2 = process_comp_y1 + ret * this.#trim.dig_xy1 / 16384.0;
		const process_comp_y3 = this.#trim.dig_y2 + 160.0;
		const process_comp_y4 = y * ((process_comp_y2 + 256.0) * process_comp_y3);
		ret = ((process_comp_y4 / 8192.0) + ((this.#trim.dig_y1) * 8.0)) / 16.0;

		return ret;
	}
	#compensateZ(z, r) {
		const process_comp_z0 = z - this.#trim.dig_z4;
		const process_comp_z1 = r - this.#trim.dig_xyz1;
		const process_comp_z2 = this.#trim.dig_z3 * process_comp_z1;
		const process_comp_z3 = this.#trim.dig_z1 * r / 32768.0;
		const process_comp_z4 = this.#trim.dig_z2 + process_comp_z3;
		const process_comp_z5 = (process_comp_z0 * 131072.0) - process_comp_z2;
		let ret = (process_comp_z5 / ((process_comp_z4) * 4.0)) / 16.0;

		return ret;
	}
}
