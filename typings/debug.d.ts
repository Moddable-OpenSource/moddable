declare module "debug" {
  var Debug: {
    gc: (enable?: boolean) => void;
  }
  export {Debug as default};
}
