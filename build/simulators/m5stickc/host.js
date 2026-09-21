import device from "embedded:provider/builtin";
import Button from 'button';
import { RTC, Battery } from 'peripherals';
import LED from "led";

Object.assign(device, {
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
});
