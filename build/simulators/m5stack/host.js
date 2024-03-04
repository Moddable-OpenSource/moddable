import Button from "button";

class a {
  constructor(options) {
    return new Button({
      ...options,
      buttonKey: "aButton",
    });
  }
}
class b {
  constructor(options) {
    return new Button({
      ...options,
      buttonKey: "bButton",
    });
  }
}
class c {
  constructor(options) {
    return new Button({
      ...options,
      buttonKey: "cButton",
    });
  }
}

debugger
globalThis.Host = Object.freeze(
  {
    Button: {
      Default: a,
      a,
      b,
      c,
    },
  },
  true
);
