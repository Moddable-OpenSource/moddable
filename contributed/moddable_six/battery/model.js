import BIN from "BinaryMessage";

const temperatureUnitRange = ["°F", "°C"];
const typeRange = ["Gel", "Flooded", "Sealed", "LiFePo"];

const model = {
	VERSION: 0x1000,
	home: {
		View: "Home",
		items: [
			{
				View: "Status",
				title: 'Status',
			},
			{
				View: "InputOutput",
				title: 'Input/Output',
			},
			{
				View: "StateOfCharge",
				title: 'Charge',
			},
			{
				View: "Settings",
				title: 'Settings',
				items: [
					{
						id: "type",
						name: 'Battery Type',
						unit: '',
						View: "Type",
						title: 'Battery Type',
						range: typeRange, 
					},
					{
						id: "capacity",
						name: 'Battery Capacity',
						unit: 'Ah',
						View: "IntPad",
						title: 'Capacity (Ah)',
					},
					{
						id: "alarm",
						unit: 'Ah',
						name: 'Alarm',
						View: "IntPad",
						title: 'Alarm (Ah)',
					},
					{
						id: "currentLimit",
						name: 'Current Limit',
						unit: 'A',
						View: "IntPad",
						title: 'Current Limit (A)',
					},
					{
						id: "fullChargeString",
						name: 'Full Charge',
						unit: 'V',
						View: "FloatPad",
						title: 'Full Charge (V)',
					},
					{
						id: "zeroChargeString",
						name: 'Zero Charge',
						unit: 'V',
						View: "FloatPad",
						title: 'Zero Charge (V)',
					},
					{
						id: "temperatureUnit",
						name: 'Battery Temp',
						unit: '',
						View: "TemperatureUnit",
						title: 'Battery Temp',
					},
					{
						name: 'History',
						unit: '',
					},
					{
						name: 'Battery to 100%',
					},
				],
			},
		],
	},
	internal: {
		type: { value:"LiFePo", retention:2 },
		capacity: { value:230, retention:2 },
		temperatureUnit: { value:"°F", retention:2 },
		fullCharge: { value:13.3, retention:2 },
		zeroCharge: { value:11.6, retention:2 },
		alarm: { value:46, retention:2 },
		currentLimit: { value:30, retention:2 },
	},
	internalFormat: [
		{ type:BIN.Uint16, name:"VERSION" },
		{ type:BIN.Uint8, name:"type", range:typeRange },
		{ type:BIN.Uint16, name:"capacity" },
		{ type:BIN.Uint8, name:"temperatureUnit", range:temperatureUnitRange },
		{ type:BIN.Float32, name:"fullCharge" },
		{ type:BIN.Float32, name:"zeroCharge" },
		{ type:BIN.Uint16, name:"alarm" },
		{ type:BIN.Uint16, name:"currentLimit" },
	],
	internalKey: "IN",
	timeout: 30000,
};

export default Object.freeze(model, true);
