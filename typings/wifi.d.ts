/*
* Copyright (c) 2019-2020 Bradley Farias
*
*   This file is part of the Moddable SDK Tools.
*
*   The Moddable SDK Tools is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   The Moddable SDK Tools is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
*
*/

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