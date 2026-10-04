/*
* Copyright (c) 2026 Moddable Tech, Inc.
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

declare module "captiveportal" {
	export type CaptivePortalPhase = "initializing" | "ready" | "connected" | "waiting" |
		"connecting" | "provisioned" | "failed";

	export interface CaptivePortalAccess {
		SSID: string;
		password?: string;
	}

	export interface CaptivePortalCredentials {
		SSID: string;
		password?: string;
		channel?: number;
		secure?: boolean;
	}

	export interface CaptivePortalNetwork {
		SSID: string;
		RSSI: number;
		channel: number;
		security?: string;
	}

	export interface CaptivePortalPage {
		content: string | ArrayBuffer | HostBuffer;
		mimeType: string;
	}

	export interface CaptivePortalOptions {
		onPage: (this: CaptivePortal, path: string) => CaptivePortalPage | undefined;

		onStatus?: (this: CaptivePortal, phase: CaptivePortalPhase, detail?: CaptivePortalAccess | CaptivePortalCredentials) => void;
		onInfo?: (this: CaptivePortal, msg: any) => void;
		onClose?: (this: CaptivePortal) => void;
		onError?: (this: CaptivePortal, error: Error) => void;

		SSID?: string;
		password?: string;
		channel?: number;
	}

	class CaptivePortal {
		constructor(options: CaptivePortalOptions);

		close(): void;

		readonly SSID: string | undefined;
	}

	export default CaptivePortal;
}
