declare module "preference" {
  var Preference: {
    set: (domain: string, key: string, value: string) => void;
    get: (domain: string, key: string) => string | undefined;
    delete: (domain: string, key: string) => void;
    keys: (domain: string) => string[];
  }
  export {Preference as default};
}
