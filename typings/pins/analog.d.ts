declare module "pins/analog" {
  class Analog {
    static read(pin: number): number;
  }
  export { Analog as default };
}
