declare class WebStorage {
  constructor(kvp: any);
  get length(): number;
  key(index: number): string | null;
  getItem(key: string): string | null;
  setItem(key: string, value: string): void;
  remove(key: string): void;
  clear(): void;
}

export default WebStorage;
