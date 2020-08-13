declare module "pins/pwm" {
  class PWM {
    public constructor(dictionary: { pin: number; port?: string });
    public write(value: number): void;
    public close(): void;
  }
  export { PWM as default };
}
