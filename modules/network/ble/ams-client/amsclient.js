 /*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 /*
 	https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleMediaService_Reference/Specification/Specification.html#//apple_ref/doc/uid/TP40014716-CH1-SW48
 */

import BLEClient from "bleclient";
import BLEServer from "bleserver";
import {uuid} from "btutils";

const EntityID = {
	Player: 0,
	Queue: 1,
	Track: 2
}
Object.freeze(EntityID);

const EntityUpdateFlags = {
	Truncated: (1 << 0),
}
Object.freeze(EntityUpdateFlags);

const TrackAttributeID = {
	Artist: 0,
	Album: 1,
	Title: 2,
	Duration: 3
}
Object.freeze(TrackAttributeID);

const PlayerAttributeID = {
	Name: 0,
	PlaybackInfo: 1,
	Volume: 2
}
Object.freeze(TrackAttributeID);

const RemoteCommandID = {
	Play: 0,
	Pause: 1,
	TogglePlayPause: 2,
	NextTrack: 3,
	PreviousTrack: 4,
	VolumeUp: 5,
	VolumeDown: 6,
	AdvanceRepeatMode: 7,
	AdvanceShuffleMode: 8,
	SkipForward: 9,
	SkipBackward: 10,
	LikeTrack: 11,
	DislikeTrack: 12,
	BookmarkTrack: 13
}
Object.freeze(RemoteCommandID);

const PlaybackState = {
	Paused: 0,
	Playing: 1,
	Rewinding: 2,
	FastForwarding: 3,
}
Object.freeze(PlaybackState);

const QueueAttributeID = {
	Index: 0,
	Count: 1,
	ShuffleMode: 2,
	RepeatMode: 3,
}
Object.freeze(QueueAttributeID);

const ShuffleMode = {
	Off: 0,
	One: 1,
	All: 2,
}
Object.freeze(ShuffleMode);

const RepeatMode = {
	Off: 0,
	One: 1,
	All: 2,
}
Object.freeze(RepeatMode);

class AMSAuthenticator extends BLEServer {
	constructor(client) {
		super();
		this.client = client;
		this.AMS_UUID = uuid`89D3502B-0F36-433A-8EF4-C502AD55F8DC`;
	}
	onReady() {
		this.deviceName = "Moddable";
		this.securityParameters = { mitm:true };
		this.onDisconnected();
	}
	onConnected(device) {
		this.device = device;
		this.stopAdvertising();
	}
	onAuthenticated() {
		this.client.onAuthenticated(this.device);
	}
	onDisconnected() {
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, solicitationUUID128List: [this.AMS_UUID]}
		});
	}
}
Object.freeze(AMSAuthenticator.prototype);

class AMSClient extends BLEClient {
	constructor(device) {
		super();
		this.device = device;
		this._supportedRemoteCommands = new Uint8Array;
	}
	onReady() {
		this.AMS_UUID = uuid`89D3502B-0F36-433A-8EF4-C502AD55F8DC`;
		this.REMOTE_COMMAND_CHARACTERISTIC_UUID = uuid`9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2`;
		this.ENTITY_UPDATE_CHARACTERISTIC_UUID = uuid`2F7CABCE-808D-411F-9A0C-BB92BA96C102`;
		this.ENTITY_ATTRIBUTE_CHARACTERISTIC_UUID = uuid`C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7`;
		this.securityParameters = { mitm:true };
	}
	onSecurityParameters() {
		this.connect(this.device);
	}
	onConnected(device) {
		device.discoverPrimaryService(this.AMS_UUID);
	}
	onServices(services) {
		if (services.length)
			services[0].discoverAllCharacteristics();
	}
	onCharacteristics(characteristics) {
		characteristics.forEach(characteristic => {
			let uuid = characteristic.uuid;
			if (uuid.equals(this.REMOTE_COMMAND_CHARACTERISTIC_UUID))
				this.remoteCommandCharacteristic = characteristic;
			else if (uuid.equals(this.ENTITY_UPDATE_CHARACTERISTIC_UUID))
				this.entityUpdateCharacteristic = characteristic;
			else if (uuid.equals(this.ENTITY_ATTRIBUTE_CHARACTERISTIC_UUID))
				this.entityAttributeCharacteristic = characteristic;
		});
		if (this.remoteCommandCharacteristic)
			this.remoteCommandCharacteristic.enableNotifications();
	}
	onCharacteristicNotificationEnabled(characteristic) {
		if (characteristic.uuid.equals(this.REMOTE_COMMAND_CHARACTERISTIC_UUID)) {
			if (this.entityUpdateCharacteristic)
				this.entityUpdateCharacteristic.enableNotifications();
		}
		else if (characteristic.uuid.equals(this.ENTITY_UPDATE_CHARACTERISTIC_UUID)) {
			let update;
			update = Uint8Array.of(EntityID.Track, TrackAttributeID.Artist, TrackAttributeID.Album, TrackAttributeID.Title, TrackAttributeID.Duration);
			characteristic.writeWithoutResponse(update.buffer);
			update = Uint8Array.of(EntityID.Player, PlayerAttributeID.PlaybackInfo);
			characteristic.writeWithoutResponse(update.buffer);
		}
	}
	onCharacteristicNotification(characteristic, buffer) {
		if (characteristic.uuid.equals(this.ENTITY_UPDATE_CHARACTERISTIC_UUID)) {
			let entityUpdate = new Uint8Array(buffer);
			let entityID = entityUpdate[0];
			let attributeID = entityUpdate[1];
			let flags = entityUpdate[2];
			let value = String.fromArrayBuffer(buffer.slice(3));
		
			if (EntityID.Track == entityID) {
				if (TrackAttributeID.Artist == attributeID)
					this._nextTrack = { artist:value };
				else if (TrackAttributeID.Album == attributeID)
					this._nextTrack.album = value;
				else if (TrackAttributeID.Title == attributeID)
					this._nextTrack.title = value;
				else if (TrackAttributeID.Duration == attributeID)
					this._nextTrack.duration = parseFloat(value);
				if (4 == Object.keys(this._nextTrack).length)
					this.onTrackChanged(this._nextTrack.artist, this._nextTrack.album, this._nextTrack.title, this._nextTrack.duration);
			}
			else if (EntityID.Player == entityID) {
				let parts = value.split(',');
				let playbackState = parseInt(parts[0]);
				let playbackRate = parseFloat(parts[1]);
				let elapsedTime = parseFloat(parts[2]);
				this.onPlaybackInfoChanged(playbackState, playbackRate, elapsedTime);
			}
		}
		else if (characteristic.uuid.equals(this.REMOTE_COMMAND_CHARACTERISTIC_UUID)) {
			this._supportedRemoteCommands = new Uint8Array(buffer);
		}
	}
	remoteCommand(command) {
		if (this._supportedRemoteCommands.includes(command))
			this.remoteCommandCharacteristic.writeWithoutResponse(Uint8Array.of(command).buffer);
	}
	onPlaybackInfoChanged(state, rate, elapsed) {
	}
	onTrackChanged(artist, album, title, duration) {
	}
}
Object.freeze(AMSClient.prototype);

export {AMSAuthenticator, AMSClient, RemoteCommandID, PlaybackState, QueueAttributeID, ShuffleMode, RepeatMode};
