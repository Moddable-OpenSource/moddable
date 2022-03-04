import Button from 'button';
import { RTC, Battery } from 'peripherals';
import LED from "led";

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
    },
    battery: {
      Default: Battery,
    },
    LED: { Default: LED }
  },
};
