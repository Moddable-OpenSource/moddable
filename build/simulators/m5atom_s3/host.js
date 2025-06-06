import Button from "button";

class a {
  constructor(options) {
    return new Button({
      ...options,
      buttonKey: "aButton",
    });
  }
}


debugger
globalThis.Host = Object.freeze(
  {
    Button: {
      Default: a,
      a
    },
  },
  true
);
