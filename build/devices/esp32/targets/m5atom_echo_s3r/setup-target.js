import config from "mc/config";
import Timer from "timer";

export default function (done) {
	new ES8311();

	// Enable speaker power amplifier
	const paEnable = new device.io.Digital({
		pin: 18,
		mode: device.io.Digital.Output
	})
	paEnable.write(1)

	done?.();
}

class ES8311 {
	constructor() {
		const es = new device.io.SMBus({
			...device.I2C.internal,
			address: 0x18,
			hz: 100_000,
		});
		es.writeUint8(0x00, 0x1F); // ES8311_RESET_REG00
		Timer.delay(20);
		es.writeUint8(0x00, 0x00); // ES8311_RESET_REG00
		es.writeUint8(0x00, 0x80); // ES8311_RESET_REG00

		// clock config
		es.writeUint8(0x01, 0xBF); // ES8311_CLK_MANAGER_REG01
		es.readUint8(0x06); // ES8311_CLK_MANAGER_REG06
		es.writeUint8(0x06, 0x03); // ES8311_CLK_MANAGER_REG06
		es.readUint8(0x02); // ES8311_CLK_MANAGER_REG02
		es.writeUint8(0x02, 0x10); // ES8311_CLK_MANAGER_REG02
		es.writeUint8(0x03, 0x10); // ES8311_CLK_MANAGER_REG03
		es.writeUint8(0x04, 0x10); // ES8311_CLK_MANAGER_REG04
		es.writeUint8(0x05, 0x00); // ES8311_CLK_MANAGER_REG05
		es.readUint8(0x06); // ES8311_CLK_MANAGER_REG06
		es.writeUint8(0x06, 0x03); // ES8311_CLK_MANAGER_REG06
		es.readUint8(0x07); // ES8311_CLK_MANAGER_REG06
		es.writeUint8(0x07, 0x00); // ES8311_CLK_MANAGER_REG07
		es.writeUint8(0x08, 0xFF); // ES8311_CLK_MANAGER_REG08

		// format config
		es.readUint8(0x00); // ES8311_RESET_REG00
		es.writeUint8(0x00, 0x80); // ES8311_RESET_REG00
		es.writeUint8(0x09, 0x10); // ES8311_SDPIN_REG09
		es.writeUint8(0x0A, 0x10); // ES8311_SDPOUT_REG0A

		//
		es.writeUint8(0x0D, 0x01); // ES8311_SYSTEM_REG0D
		es.writeUint8(0x0E, 0x02); // ES8311_SYSTEM_REG0E
		es.writeUint8(0x12, 0x00); // ES8311_SYSTEM_REG12
		es.writeUint8(0x13, 0x10); // ES8311_SYSTEM_REG13
		es.writeUint8(0x1C, 0x6A); // ES8311_ADC_REG1C
		es.writeUint8(0x37, 0x08); // ES8311_DAC_REG37

		// set volume (0 - 256)
		let volume = config.es8311.volume ?? 128;
		if (volume < 0) volume = 0;
		if (volume > 255) volume = 255;
		es.writeUint8(0x32, volume); // ES8311_DAC_REG32

		// microphone
		es.writeUint8(0x17, 0xFF); // ES8311_ADC_REG17 (ADC volume)
		es.writeUint8(0x14, 0x1A); // ES8311_SYSTEM_REG14
		es.writeUint8(0x16, 0x01); // ES8311_ADC_REG16 (6DB)

		es.close();
	}
}
