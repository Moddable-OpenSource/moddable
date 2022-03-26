import Button from 'button';
import { RTC, Battery } from 'peripherals';

globalThis.device = {	
  pin: {
    powerMain: 2,
    powerExternal: 5,
    powerEPD: 23,
    touchInterrupt: 36,
    epdSelect: 15,     
    epdBusy: 27,
    batteryVoltage: 35
  },
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
