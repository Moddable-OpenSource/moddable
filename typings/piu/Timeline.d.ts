
declare module "piu/Timeline" {
  class Timeline {
    duration: number;
    fraction: number;
    time: number;
    from(
      target: object,
      fromProperties: object,
      duration: number,
      easing?: string,
      delay?: number
    ): Timeline;
    on(
      target: object,
      onProperties: object,
      duration: number,
      easing?: number,
      delay?: number
    ): Timeline;
    seekTo(time: number): void;
    to(
      target: object,
      fromProperties: object,
      duration: number,
      easing?: string,
      delay?: number
    ): Timeline;
  }
  export { Timeline as default }
}
