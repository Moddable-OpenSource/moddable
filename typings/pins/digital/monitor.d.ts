declare module "pins/digital/monitor" {
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
  class Monitor {
    public constructor(dictionary: {
      pin: number;
      port?: string;
      mode: Mode;
      edges: Edge;
    });
    public onChanged(callback: () => void): void;
    public read(): number;
    public close(): void;
    public readonly rises: number;
    public readonly falls: number;
  }
  export { Monitor as default };
}
