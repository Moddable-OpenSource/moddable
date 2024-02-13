import Button from "button";

class A {
  constructor(options) {
    return new Button({
      ...options,
      buttonKey: "aButton",
    });
  }
}
class B {
  constructor(options) {
    return new Button({
      ...options,
      buttonKey: "bButton",
    });
  }
}
class C {
  constructor(options) {
    return new Button({
      ...options,
      buttonKey: "cButton",
    });
  }
}

globalThis.Host = Object.freeze(
  {
    Button: {
      Default: A,
      A,
      B,
      C,
    },
  },
  true
);
