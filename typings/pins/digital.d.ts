declare module "pins/digital" {
  class Digital {
    public constructor(dictionary: {
      pin: number;
      mode: number;
      port?: string;
    });
    public constructor(pin: number, mode: number);
    public constructor(port: string, pin: number, mode: number);
    public read(): number;
    public write(value: number): void;
    public mode(mode: number): void;
    public static read(pin: number): number;
    public static write(pin: number, value: number): void;
    public static readonly Input: number;
    public static readonly InputPullUp: number;
    public static readonly InputPullDown: number;
    public static readonly InputPullUpDown: number;
    public static readonly Output: number;
    public static readonly OutputOpenDrain: number;
  }
  module Digital {
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
    type Rising = 1;
    type Falling = 2;
    type Edge = Rising | Falling;
  }
  class Monitor {
    public constructor(dictionary: {
      pin: number;
      port?: string;
      mode: Digital.Mode;
      edges: Digital.Edge;
    });
    public onChanged(callback: () => void): void;
    public read(): number;
    public close(): void;
    public rises: number;
    public falls: number;
  }
  export { Digital as default, Monitor };
}
