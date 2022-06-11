// WAVESHARE  - Pico LCD 1.3
// https://www.waveshare.com/wiki/Pico-LCD-1.3

import Button from "button";

class A {
  constructor(options) {
    return new Button({ ...options, invert: true, pin: 15 });
  }
}

globalThis.Host = Object.freeze(
  {
    Button: {
      Default: A,
      A,
      B: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 17 });
        }
      },
      X: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 19 });
        }
      },
      Y: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 21 });
        }
      },
      UP: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 2 });
        }
      },
      DOWN: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 18 });
        }
      },
      LEFT: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 16 });
        }
      },
      RIGHT: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 20 });
        }
      },
      CTRL: class {
        constructor(options) {
          return new Button({ ...options, invert: true, pin: 3 });
        }
      },
    },
  },
  true
);

export default function (done) {
  done();
}
