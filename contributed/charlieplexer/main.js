class Charlieplexer {
  constructor(pins) {
    this.initializePins(pins);
  }

  initializePins(pins) {
    this.pins = pins.map(p => new Digital(p, Digital.Input));
  }

  scan() {
    for (let i = 0; i < this.pins.length - 1; i++) {
      for (let j = i + 1; j < this.pins.length; j++) {
        const p1 = this.pins[i];
        const p2 = this.pins[j];
        p1.mode(Digital.Output);
        p2.mode(Digital.Output);

        p1.write(1);
        p2.write(0);
        Timer.delay(100);

        p1.write(0);
        p2.write(1);
        Timer.delay(100);

        p1.mode(Digital.Input);
        p2.mode(Digital.Input);
      }
    }
  }
}
