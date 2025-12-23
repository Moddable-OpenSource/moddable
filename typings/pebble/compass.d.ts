interface CompassOptions {
  onSample?: () => void;
}

interface CompassConfigureOptions {
  filter?: number;
}

interface CompassSample {
  heading: number;
}

declare class Compass {
  constructor(options: CompassOptions);
  close(): void;
  configure(options: CompassConfigureOptions): void;
  sample(): CompassSample;
}

export default Compass;
