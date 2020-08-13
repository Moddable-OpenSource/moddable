declare module "pins/audioin" {
  class AudioIn {
    public constructor();
    public close(): void;
    public read(samples: number): number;
    public readonly sampleRate: number;
    public readonly bitsPerSample: number;
    public readonly numChannels: number;
  }
  export { AudioIn as default };
}
