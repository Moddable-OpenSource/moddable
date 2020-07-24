declare module "mc/config" {
  var Config: {file: {root: string}} & Record<string, any>;
  export {Config as default};
}
