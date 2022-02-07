import Button from 'button';
import { RTC, Battery } from 'peripherals';

globalThis.device = {
  peripheral: {
    RTC: RTC,
    button: {
      A: class {
        constructor(options) {
          return new Button({
            ...options,
            buttonKey: 'aButton',
          });
        }
      },
      B: class {
        constructor(options) {
          return new Button({
            ...options,
            buttonKey: 'bButton',
          });
        }
      },
      C: class {
        constructor(options) {
          return new Button({
            ...options,
            buttonKey: 'cButton',
          });
        }
      },
    },
    battery: {
      Default: Battery,
    },
  },
};
