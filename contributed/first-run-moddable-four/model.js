/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */


const model = {
	menu: {
		View: "Menu",
		items: [
			{
				icon: "about",
				label: 'About',
				View: "About",
				items: [
					{
						title: 'Hardware',
						items: [
							'nRF52840 MCU',
							'1 MB flash',
							'256 KB RAM',
							'BLE',
							'128x128 reflective display',
							'Jog-wheel',
							'Back button',
							'12 external GPIOs',
							'LIS3DH accelerometer',
							'CR2032 battery socket',
						]
					},
					{
						title: 'Software',
						items: [
							'Moddable SDK',
							'ECMA-419 embedded APIs',
							'XS JavaScript engine (ECMAScript 2022)',
							'BLE peripheral and central',
							'Energy management',
							'Piu UI framework',
							'Poco renderer',
							'QR Core rendering',
							'Automatic dithering',
						]
					},
					{
						title: 'Tools',
						items: [
							'JavaScript debugger',
							'JavaScript profiler',
							'TypeScript support',
							'Graphic asset conversion',
						]
					},
					{
						title: 'Learn More',
						items: [],
						url: "https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/moddable-four.md"
				
					}
				],
			},
			{
				icon: "bleWeb",
				label: 'Web BLE',
				View: "BLEWeb"
			},
			{
				icon: "wake",
				label: 'Wake',
				View: "Wake"
			},
			{
				icon: "dither",
				label: 'Dither',
				View: "Dither"
			},
			{
				icon: "accelerometer",
				label: 'Accelerometer',
				View: "Gravity"
			},
			{
				icon: "date",
				label: 'Date',
				View: "SetDate"
			},
			{
				icon: "time",
				label: 'Time',
				View: "SetTime"
			},
		],
	},
	timeout: 30000,
};

export default Object.freeze(model, true);
