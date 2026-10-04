declare module "global" {

  import Device from "embedded:provider/builtin"

  export {}
  global {
    var device: typeof Device;
  }
}
