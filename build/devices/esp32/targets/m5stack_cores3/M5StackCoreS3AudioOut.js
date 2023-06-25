import AudioOut from "pins/audioout-original"

/**
 * A special AudioOut implementation for M5Stack CoreS3
 * CoreS3 has an amplifier IC (AW88298).
 * The user must set the sample rate to the IC before playing audio.
 */
export default class M5StackCoreS3AudioOut extends AudioOut {
  constructor(options) {
    super(options)
    if (globalThis.amp == null) {
      trace('amp not found\n')
    } else {
      globalThis.amp.sampleRate = this.sampleRate
    }
  }
}