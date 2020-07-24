declare module "time" {
  var Time: {
    set: (
      seconds: number
    ) => void,
    timezone: number;
    dst: number;
    ticks: number;
  }
  export {Time as default};
}