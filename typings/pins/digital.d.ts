declare module "pins/digital" {
  type Input = 0;
  type InputPullUp = 1;
  type InputPullDown = 2;
  type InputPullUpDown = 3;
  type Output = 8;
  type OutputOpenDrain = 9;
  type Mode =
    | Input
    | InputPullUp
    | InputPullDown
    | InputPullUpDown
    | Output
    | OutputOpenDrain;
  export class Digital {
    public constructor(dictionary: {
      pin: number;
      mode: number;
      port?: string;
    });
    public constructor(pin: number, mode: number);
    public constructor(port: string, pin: number, mode: number);
    public read(): number;
    public write(value: number): void;
    public mode(mode: Mode): void;
    public static read(pin: number): number;
    public static write(pin: number, value: number): void;
    public static readonly Input: Input;
    public static readonly InputPullUp: InputPullUp;
    public static readonly InputPullDown: InputPullDown;
    public static readonly InputPullUpDown: InputPullUpDown;
    public static readonly Output: Output;
    public static readonly OutputOpenDrain: OutputOpenDrain;
  }
  export { Digital as default };
}
