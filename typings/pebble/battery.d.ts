interface BatteryOptions {
  onSample?: () => void;
}

interface BatterySample {
  percent: number;
  charging: boolean;
  plugged: boolean;
}

declare class Battery {
  constructor(options: BatteryOptions);
  close(): void;
  configure(options: {}): void;
  sample(): BatterySample;
}

export default Battery;
