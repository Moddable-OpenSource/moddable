//import Digital from "pins/digital";
//import Monitor from "monitor";
import M5Button from "m5button";
import config from "mc/config";
import Timer from "timer";
import Button from "button";
import I2C from "pins/i2c";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import SMBus from "pins/smbus";

const INTERNAL_I2C = Object.freeze({
	sda: 38,
	scl: 39,
	hz: 100000
});

class Flash {
        constructor(options) {
                return new Button({
                        ...options,
                        pin: 0,
                        invert: true
                });
        }
}

globalThis.Host = Object.freeze({
        Button: {
                Default: Flash,
                Flash
        }
}, true);

export default function (done) {
        globalThis.button = {
                a: new M5Button(41)
        };

		// init M5Atomic Echo Base
		new ES8311();
		new PI4IOE5V6408();

        // start-up sound
	if (config.startupSound) {
		const speaker = new AudioOut({streams: 1});

                speaker.callback = function () {
			this.stop();
			this.close();
			Timer.set(this.done);
		};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Silence, 16000); // enqueue silence for no audio output (1 sec) of ES8311 I2S initial setup
       		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound), 1);
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();

        }

        done?.();
}

class ES8311 {
	// initialize ES8311(0x18)
	// https://github.com/m5stack/uiflow-micropython/blob/master/m5stack/libs/driver/es8311/__init__.py
        constructor(options) {
                const es = new SMBus({
        		...INTERNAL_I2C,
        		address: 0x18,
        	});
        	es.writeByte(0x00, 0x1F); // ES8311_RESET_REG00
        	Timer.delay(20);
        	es.writeByte(0x00, 0x00); // ES8311_RESET_REG00
        	es.writeByte(0x00, 0x80); // ES8311_RESET_REG00

        	// clock config
        	es.writeByte(0x01, 0xBF); // ES8311_CLK_MANAGER_REG01
        	es.readByte(0x06); // ES8311_CLK_MANAGER_REG06
        	es.writeByte(0x06, 0x03); // ES8311_CLK_MANAGER_REG06
        	es.readByte(0x02); // ES8311_CLK_MANAGER_REG02
        	es.writeByte(0x02, 0x10); // ES8311_CLK_MANAGER_REG02
        	es.writeByte(0x03, 0x10); // ES8311_CLK_MANAGER_REG03
        	es.writeByte(0x04, 0x10); // ES8311_CLK_MANAGER_REG04
        	es.writeByte(0x05, 0x00); // ES8311_CLK_MANAGER_REG05
        	es.readByte(0x06); // ES8311_CLK_MANAGER_REG06
        	es.writeByte(0x06, 0x03); // ES8311_CLK_MANAGER_REG06
        	es.readByte(0x07); // ES8311_CLK_MANAGER_REG06
        	es.writeByte(0x07, 0x00); // ES8311_CLK_MANAGER_REG07
        	es.writeByte(0x08, 0xFF); // ES8311_CLK_MANAGER_REG08

        	// format config
        	es.readByte(0x00); // ES8311_RESET_REG00
        	es.writeByte(0x00, 0x80); // ES8311_RESET_REG00
        	es.writeByte(0x09, 0x10); // ES8311_SDPIN_REG09
        	es.writeByte(0x0A, 0x10); // ES8311_SDPOUT_REG0A

                //
        	es.writeByte(0x0D, 0x01); // ES8311_SYSTEM_REG0D
        	es.writeByte(0x0E, 0x02); // ES8311_SYSTEM_REG0E
        	es.writeByte(0x12, 0x00); // ES8311_SYSTEM_REG12
                es.writeByte(0x13, 0x10); // ES8311_SYSTEM_REG13
                es.writeByte(0x1C, 0x6A); // ES8311_ADC_REG1C
                es.writeByte(0x37, 0x08); // ES8311_DAC_REG37

        	// set volume (0 - 256)
		let volume = config.es8311.volume ?? 128;
		if (volume < 0) volume = 0;
		if (volume > 255) volume = 255;
        	es.writeByte(0x32, volume); // ES8311_DAC_REG32

        	// microphone
                es.writeByte(0x17, 0xFF); // ES8311_ADC_REG17 (ADC volume)
                es.writeByte(0x14, 0x1A); // ES8311_SYSTEM_REG14
                es.writeByte(0x16, 0x01); // ES8311_ADC_REG16 (6DB)

        	es.close();
        }
}

class PI4IOE5V6408 {
	// initialize PI4IOE5V6408(0x43)
	// https://github.com/m5stack/uiflow-micropython/blob/master/m5stack/libs/base/echo.py
        constructor(options) {
                const pi = new SMBus({
        		...INTERNAL_I2C,
                        address: 0x43,
                });
	        pi.readByte(0x00); // PI4IOE_REG_CTRL
        	pi.writeByte(0x07, 0x00); // PI4IOE_REG_IO_PP
        	pi.readByte(0x07);
        	pi.writeByte(0x0D, 0xFF); // PI4IOE_REG_IO_PULLUP
        	pi.writeByte(0x03, 0x6E); // PI4IOE_REG_IO_DIR
        	pi.readByte(0x03);
        	pi.writeByte(0x05, 0xFF); // PI4IOE_REG_IO_OUT
        	pi.readByte(0x05);

        	pi.close();
        }
}

