declare module "Resource" {
  var Resource: {
    new (path: string): ArrayBuffer | HostBuffer
    exists(path: string): boolean;
  };
  export {Resource as default};
}
