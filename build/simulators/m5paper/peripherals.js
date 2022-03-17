import Control from 'control';

export class Digital extends Control {
  #pin;
  #mode;
  #value;
	constructor(options) {
    super(options);
		this.#pin = options.pin;
		this.#mode = options.mode;
    this.#value = options.value || 0;
    this.#updateJson();
	}

	read() {
		return this.#value
	}

	write(value) {
    this.#value = Number(value) ? 1 : 0;
    this.#updateJson();
	}

  close() {}

	get format() {
		return "number";
	}

	set format(value) {
		if ("number" !== value)
			throw new RangeError;
	}

  onJSON(json) {
    if ('digital' in json && json.digital.pin === this.#pin) {
      this.value = json.digital.value;
    }
  }

  #updateJson() {
    this.postJSON({
      digital: {
        pin: this.#pin,
        mode: this.#mode,
        value: this.#value
      }
    })
  }
}
Digital.Input = 0;
Digital.InputPullUp = 1;
Digital.InputPullDown = 2;
Digital.InputPullUpDown = 3;

Digital.Output = 8;
Digital.OutputOpenDrain = 9;

Digital.Rising = 1;
Digital.Falling = 2;

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
