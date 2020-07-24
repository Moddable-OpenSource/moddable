declare module "wifi" {
  type WiFiOptions = {
    bssid?: string,
    ssid: string,
    password?: string
  }
  type WiFiCallback = (message: "connect" | "gotIP" | "disconnect") => void
  type WiFiScanCallback = (item: {
    ssid: string,
    authentication: string,
    rssi: number,
    bssid: string,
  } | null) => void;
  type StationMode = 1;
  type AccessPointMode = 2;
  class WiFi {
    constructor(options: WiFiOptions, callback: WiFiCallback);
    close(): void;
    static scan(options: {hidden?: boolean, channel: number}, callback: WiFiScanCallback): void;
    mode: StationMode | AccessPointMode;
    static connect(options?: WiFiOptions);
    static accessPoint(options: {
      ssid: string,
      password?: string,
      channel?: number,
      hidden?: boolean,
      interval?: number,
      max?: number
    });
  }
  export {WiFi as default};
}