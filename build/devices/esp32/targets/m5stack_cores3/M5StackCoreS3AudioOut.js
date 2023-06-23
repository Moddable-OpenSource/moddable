import AudioOut from "pins/audioout-original"
import SMBus from "pins/smbus"

const INTERNAL_I2C = Object.freeze({
	sda: 12,
	scl: 11
});

/**
 * AW88298 amplifier IC
 */
class AW88298 extends SMBus {
  #rateTable
  constructor() {
    super({ address: 0x36, ...INTERNAL_I2C });
    this.#rateTable = [4, 5, 6, 8, 10, 11, 15, 20, 22, 44];
    this.writeWord(0x05, 0x0008, true); // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
    this.writeWord(0x61, 0x0673, true); // boost mode disabled
    this.writeWord(0x04, 0x4040, true); // I2SEN=1 AMPPD=0 PWDN=0
    this.volume = 250
  }

  /**
   * @note with ESP-IDF, 11025Hz and its multiples are not available for the slight gap between the clock of ESP32S3 and AW88298 PLL
   * @fixme should reset sampleRate if the different value specified in AudioOut#constructor
   */
  set sampleRate(sampleRate) {
    let rateData = 0;
    let rate = Math.round((sampleRate + 1102) / 2205);
    while (rate > this.#rateTable[rateData] && ++rateData < this.#rateTable.length);
    rateData |= 0x14c0; // I2SRXEN=1 CHSEL=01(left) I2SFS=11(32bits)
    this.writeWord(0x06, rateData, true);
  }

  set volume(volume) {
    const vdata = Math.round(Math.min(256, Math.max(0, volume)))
    this.writeWord(0x0c, ((256 - vdata) << 8) | 0x64, true)
  }

  get volume() {
    const vdata = this.readByte(0x0c)
    return 256 - vdata
  }
}

/**
 * A special AudioOut implementation for M5Stack CoreS3
 * CoreS3 has an amplifier IC (AW88298).
 * The user must set the sample rate to the IC before playing audio.
 */
export default class M5StackCoreS3AudioOut extends AudioOut {
  constructor(options) {
    super(options)
    if (globalThis.amp == null) {
      globalThis.amp = new AW88298();
    }
    globalThis.amp.sampleRate = this.sampleRate
  }
}