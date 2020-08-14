declare module "pins/servo" {
  class Servo {
    public constructor(dictionary: { pin: number; min?: number; max?: number });
    public write(degrees: number): void;
    public writeMicroseconds(us: number): void;
  }
  export { Servo as default };
}
