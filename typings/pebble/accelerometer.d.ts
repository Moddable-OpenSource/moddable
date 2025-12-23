type TapDirection = "+x" | "-x" | "+y" | "-y" | "+z" | "-z";

interface AccelerometerOptions {
  onSample?: () => void;
  onTap?: (direction: TapDirection) => void;
  onDoubleTap?: (direction: TapDirection) => void;
}

interface AccelerometerConfigureOptions {
  hz: 10 | 25 | 50 | 100;
}

interface AccelerometerSample {
  x: number;
  y: number;
  z: number;
}

declare class Accelerometer {
  constructor(options: AccelerometerOptions);
  close(): void;
  configure(options: AccelerometerConfigureOptions): void;
  sample(): AccelerometerSample;
}

export default Accelerometer;
