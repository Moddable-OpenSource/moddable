import Control from 'control';

export class Battery extends Control {
  #battery;

  constructor(options) {
    super(options);
    this.#battery = 1.0;
    this.postJSON({
      battery: this.#battery,
    });
  }
  read() {
    return this.#battery;
  }
  onJSON(json) {
    if ('battery' in json) {
      this.#battery = json.battery;
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
    return this.#config.enabled;
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
