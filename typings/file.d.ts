declare module "file" {
  /**
   * The `File` class provides access to files.
   */
  export class File {
    constructor(path: string, write?: boolean);
    /**
     * 
     */
    read<T extends (
      typeof ArrayBuffer | typeof String
    )>(
      type: T,
      bytes?: number
    ): T extends typeof String ? string : InstanceType<T>;

    write(
      value: ArrayBuffer | string,
      ...moreValues: (ArrayBuffer | string)[]
    ): void;

    close(): void;

    readonly length: number;
    position: number;

    static delete(path: string): void;
    static exists(path: string): boolean;
    static rename(from: string, to: string): void;
  }
  type IteratorDirectoryEntry = {name: string, length: undefined};
  type IteratorFileEntry = {name: string, length: number};
  class Iterator {
    constructor(path: string);
    next(): IteratorDirectoryEntry | IteratorFileEntry | undefined;
  }

  class System {
    static config(): {maxPathLength: number}
    static info(): {used: number, total: number};
  }
}