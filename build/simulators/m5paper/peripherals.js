import Control from 'control';

export class Battery extends Control {
  #voltage;

  constructor(options) {
    super(options);
    this.#voltage = 4300; // in mV
    this.postJSON({
      battery: this.#voltage,
    });
  }
  read() {
    return this.#voltage;
  }
  onJSON(json) {
    if ('battery' in json) {
      this.#voltage = json.battery;
    }
  }
}

export class RTC extends Control {
  #config;

  constructor(options) {
    super(options);
    this.#config = { time: undefined, enabled: true };
    this.postJSON({
      rtc: this.#config,
    });
  }

  enabled() {
    return this.#config.enabeld;
  }

  get time() {
    if (this.#config.time) {
      return this.#config.time;
    }
  }
  set time(time) {
    this.#config.time = time;
    this.postJSON({
      rtc: this.#config,
    });
  }

  onJSON(json) {
    if ('rtc' in json) {
      this.#config.time = this.#config.time || json.time;
      this.#config.enabled = this.#config.enabled || Boolean(json.enabled);
    }
  }
}
