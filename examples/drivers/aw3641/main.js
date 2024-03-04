/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

import Timer from "timer";
import AW3641 from "aw3641";

//const val=AW3641.off;
//const val=AW3641.Time220ms_Brightness100;
//const val=AW3641.Time220ms_Brightness90;
//const val=AW3641.Time220ms_Brightness80;
//const val=AW3641.Time220ms_Brightness70;
//const val=AW3641.Time220ms_Brightness60;
const val=AW3641.Time220ms_Brightness50;
//const val=AW3641.Time220ms_Brightness40;
//const val=AW3641.Time220ms_Brightness30;
//const val=AW3641.Time1_3s_Brightness100;
//const val=AW3641.Time1_3s_Brightness90;
//const val=AW3641.Time1_3s_Brightness80;
//const val=AW3641.Time1_3s_Brightness70;
//const val=AW3641.Time1_3s_Brightness60;
//const val=AW3641.Time1_3s_Brightness50;
//const val=AW3641.Time1_3s_Brightness40;
//const val=AW3641.Time1_3s_Brightness30;	

const lamp = new AW3641({pin:26,});

Timer.repeat(() => {
	lamp.flash(val);
	trace("flash\n");
}, 2000);


