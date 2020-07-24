declare module "mdns" {
  type MDNSService = {
    name: string,
    protocol: string,
    port: number,
    txt: Record<string, string>
  };
  class MDNS {
    constructor(
      options: {
        hostName: string
      }, callback?: (message: number, value?: string) => void
    );
    monitor(
      service: string,
      callback: (
        service: string,
        instance: MDNSService
      ) => void
    ): void;
    add(service: MDNSService): void;
    update(service: MDNSService): void;
    remove(service: MDNSService): void;
    remove(serviceType: string): void;
  }
  export {MDNS as default};
}
