/*
* Copyright (c) 2025 Moddable Tech, Inc.
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

declare module "ChatAudioIO" {
	type ChatAudioIOOptions = {
		specifier: "googleGeminiLive" | "openAIRealtime" | "humeAIEVI" | "elevenLabsAgent" | "deepgramAgent";
		voiceName?: string;
		instructions?: string;
		apiKey?: string;

		onStateChanged?: (this: ChatAudioIO, state: number) => void;
		onInputTranscript?: (this: ChatAudioIO, text: string, more: boolean) => void;
		onOutputTranscript?: (this: ChatAudioIO, text: string, more: boolean) => void;
		onFunctionCall?: (this: ChatAudioIO, call: any, name: string, parameters: any) => void;
		onInputLevelChanged?: (this: ChatAudioIO, level: number) => void;
		onOutputLevelChanged?: (this: ChatAudioIO, level: number) => void;
	};

	class ChatAudioIO {
		static readonly FAILED: number;
		static readonly DISCONNECTED: number;
		static readonly CONNECTING: number;
		static readonly DISCONNECTING: number;
		static readonly SPEAKING: number;
		static readonly LISTENING: number;
		static readonly WAITING: number;

		readonly error: string;
		readonly state: number;
		readonly microphone: number | boolean;
		readonly volume: number;

		constructor(options: ChatAudioIOOptions);

		close(): void;

		/**
		 * Controls whether microphone input is sent to AI service.
		 * @param microphone true sends audio to cloud, false disables transmission.
		 */
		changeMicrophone(microphone: boolean): void;

		/**
		 * Changes the audio output volume.
		 * @param volume Number between 0.0 (silent) and 1.0 (maximum volume).
		 */
		changeVolume(volume: number): void;

		connect(): void;
		disconnect(): void;

		failed(message: { string: string }): void;
		sendFunctionResult(call: any, name: string, result: any): void;
		sendText(text: string): void;
	}

	export default ChatAudioIO;
}
