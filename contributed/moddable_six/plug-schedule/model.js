import BIN from "BinaryMessage";

const model = {
	VERSION: 0x1001,
	home: {
		View: "Home",
	},
	settings: {
		View: "Settings",
		comments: 'Joining Wi-Fi network and setting timezone allows for automatic time detection',
		items: [
			{
				Item: "SettingItem",
				id: "time",
				name: 'Time',
				View: "SetTime",
				title: 'Set Time',
			},
			{
				Item: "SettingItem",
				id: "network",
				name: 'Network',
				View: "Networks",
				title: 'Networks',
			},
			{
				Item: "SettingItem",
				id: "timezoneName",
				name: 'Timezone',
				View: "Timezone",
				title: 'Timezone',
				comments: 'Join Wi-Fi network to automatically get time'
			},
			{
				Item: "DSTSettingItem",
				id: "dst",
				name: 'Daylight Savings',
			},
		],
	},
	internal: {
		timezone: { value:3, retention:2 },
		dst: { value:0, retention:2 },
	},
	internalFormat: [
		{ type:BIN.Uint16, name:"VERSION" },
		{ type:BIN.Uint8, name:"timezone" },
		{ type:BIN.Uint8, name:"dst" },
	],
	internalKey: "IN",
	plug: {
		schedule: { value:1, retention:2 },
		startTime: { value:8 * 3600, retention:2 },
		stopTime: { value:17 * 3600, retention:2 },
	},
	plugFormat: [
		{ type:BIN.Uint16, name:"VERSION" },
		{ type:BIN.Uint8, name:"schedule" },
		{ type:BIN.Int32, name:"startTime" },
		{ type:BIN.Int32, name:"stopTime" },
	],
	plugKeys: [ "P0", "P1" ],
	plugNameKeys: [ "N0", "N1" ],
	ssidKey: "ID",
	passwordKey: "PW",
	authenticationKey: "AT",
	
	timeout: 30000,
	timezones: [
		"Samoa",
		"Hawaii",
		"Alaska",
		"Pacific",
		"Mountain",
		"Central",
		"Eastern",
		"Atlantic",
		"Uruguay",
		"SGSSI",
		"Azores",
		"Greenwich Mean",
		"Central European",
		"Eastern European",
		"Indian Ocean",
		"Arabian",
		"Pakistan",
		"Bangladesh",
		"Thailand",
		"China",
		"Japan",
		"Australian Eastern",
		"Vanuatu",
		"New Zealand",
	]
};

export default Object.freeze(model, true);
