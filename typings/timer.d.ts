declare module "timer" {
  type TimerID = number;
  type TimerCallback = (id: TimerID) => void;
  var Timer: {
    set: (
      callback: TimerCallback,
      interval?: number,
      repeat?: boolean
    ) => TimerID,
    repeat: (
      callback: TimerCallback,
      interval?: number,
    ) => TimerID
    schedule: (
      id: TimerID,
      interval?: number,
      repeat?: boolean
    ) => void;
    clear: (id: TimerID) => void;
    delay: (ms: number) => void;
  }
  export {Timer as default};
}