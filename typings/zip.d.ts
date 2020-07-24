declare module "zip" {
  class Zip {
    constructor(buffer: ArrayBuffer | HostBuffer);
    file(path: string): import("file").File;
    iterate(path: string): import("file").Iterator;
    map(path: string): HostBuffer;
  }
  export {Zip as default};
}
