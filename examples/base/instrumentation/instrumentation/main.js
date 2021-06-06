/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import Instrumentation from "instrumentation";

const instruments = [
	"",
	"Pixels Drawn",
	"Frames Drawn",
	"Network Bytes Read",
	"Network Bytes Written",
	"Network Sockets",
	"Timers",
	"Files",
	"Poco Display List Used",
	"Piu Command List Used",
	"System Free Memory",
	"XS Slot Heap Used",
	"XS Chunk Heap Used",
	"XS Keys Used",
	"XS Garbage Collection Count",
	"XS Modules Loaded",
	"XS Stack Used",
];

for (let i = 1; i < 17; i++)
	trace(`${instruments[i]}: ${Instrumentation.get(i)}\n`);
