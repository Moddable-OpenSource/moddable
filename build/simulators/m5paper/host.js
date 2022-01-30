import Button from 'button';

globalThis.device = {
  peripheral: {
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
  },
};

