/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import config from "mc/config";

import Time from "time";

const PROD_NAME = "ModClock";
const SPACES = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";

export class html_content {

	static clockScripts(style, server) {
		return `
<script src="${server}current_version.js?${Time.ticks}"></script>
<script type="text/javascript">
	let lastSelected = "${style.tag}";
	function showStyleDiv(select) {
		let lastBlock = 'hidden_style_' + lastSelected;
		let newBlock = 'hidden_style_' + select.value;
		document.getElementById(lastBlock).style.display = "none";
		document.getElementById(newBlock).style.display = "block";
		lastSelected = select.value;
	}
	function postToURL(url, params) {
		var form = document.createElement("form");
		form._submit_function_ = form.submit;
		form.setAttribute("method", "post");
		form.setAttribute("action", url);
		for (var key in params) {
			var hiddenField = document.createElement("input");
			hiddenField.setAttribute("type", "hidden");
			hiddenField.setAttribute("name", key);
			hiddenField.setAttribute("value", params[key]);
			form.appendChild(hiddenField);
		}
		document.body.appendChild(form);
		form._submit_function_();
	}

	function fmtClr(c) {
		c = c.trim();
		if (c[0] == "#")
			c = c.slice(1);
		if (c.length < 3)
			c = (c + "000").slice(0,2);
		if (c.length < 4)
			return "#" + c[0] + "0" + c[1] + "0" + c[2] + "0";
		return ("#" + c + "000").slice(0,7);
	}

</script>\n`;
	}


	static resetPrefsResp() {
		return `Resetting prefs.<p>Reconnect to the <b>clock</b> access point and go to <a href=http://clock.local>http://clock.local</a> to configure.<p>`;
	}

	static changeSSIDResp(ssid, name) {
		return `Trying ssid:<b>${ssid}</b>. Reconnect to the Wi-Fi access point ${ssid} and try <a href="http://${name}.local">http://${name}.local</a><br>`;
	}

	static redirectHead(name, sec=3, path="") {
		return `<html><head><meta http-equiv="refresh" content="${sec}; url=http://${name}.local${path.length?path:""}"><title>${name}</title></head>`;
	}

	static bodyPrefix() {
		return `<body>`;
	}

	static bodySuffix(msg) {
		return `<div><hr>${msg}</div>\n</body></html>`;
	}

	static noAccessPointSet() {
		return `<h2>Please set the name of your clock and connect to an access point.<p></h2>`;
	}

	static accessPointSection(apList, selected="", name) {
		let body = `<h2>Access point</h2><form action="/set-ssid" method="post"><div><label name="ssid">SSID: </label>`;

	    if (apList.length > 0) {
            apList.sort(function(a, b) {
                a = a.ssid.toLowerCase();
                b = b.ssid.toLowerCase();
                if (a === b)
                    return 0;
                return (a > b) ? +1 : -1;
            });
            
	        body += '<select class="select-css" name="ssid_select">';
			apList.forEach(function(ap) {
				body += `<option value="${ap.ssid}" `;
				if (ap.ssid == selected)
					body += " selected";
				body += `>${ap.ssid}</option>`;
			});

			body += `</select>${SPACES}Other:`;
		}

		body += `<input type="text" width="64" id="ssid" name="ssid"><p>
<label for="password">password: </label>
<input type="password" id="password" name="password" minlength="8"></div><p>
<div><label name="name">Name:</label>
<input type="text" width="64" id="name" name="clock_name" value="${name}"></div><p>
<div><button class="button pressButton">Set SSID</button></div>
</form>`;
/*
	    if (apList.length > 0)
			body += `<div><button class="button pressButton" onclick="postToURL('http://${name}.local/rescanSSID',{submit:'submit'})">Rescan SSIDs</button></div>`;
*/
		return body;
	}


	static selection(name, items, selected, indexed=1, sclass="select-css") {
		let x = `<select class="${sclass}" name="${name}">`;
		for (let i=0; i<items.length; i++) {
			if (1 === indexed) {
				x += `<option value="${i}"`;
				if (selected == i)
					x += ` selected`;
				x += `>${items[i]}</option>`;
			}
			else {
				x += `<option value="${items[i]}"`;
				if (selected === items[i])
					x += ` selected`;
				x += `>${items[i]}</option>`;
			}
		}
		x += `</select>`;
		return x;
	}

	static slider(name, value, min=1, max=255) {
		return `<input type="hidden" id="${name}" name="${name}" value="${value}">
<input type="range" min="${min}" max="${max}" value="${value}" class="slider" id="${name}Range"> Value: <span id="${name}_range_val"></span>

<script>
var slider${name} = document.getElementById("${name}Range");
document.getElementById("${name}_range_val").innerHTML = slider${name}.value;
slider${name}.oninput = function() {
	document.getElementById("${name}").value = slider${name}.value;
	document.getElementById("${name}_range_val").innerHTML = slider${name}.value;
}
</script>`;
	}

	static get_version(tag) {
		return `<script>document.getElementById("${tag}").innerHTML = current_version();</script>`;
	}

	static clockUpdateCheck(clock) {
		let body = "";
		if (undefined === clock.ota) {
			body += `<span id="id_update_html">Checking for update</span><p><script type="text/javascript">
if (check_update("${clock.getbuildstring().trim()}"))
	document.getElementById("id_update_html").innerHTML = "<h2>Update Available</h2>Clock version: ${clock.getbuildstring()}<p>Server version: <b>" + current_version() + "</b><p>" + version_description() + "<p><div><button class=\\"button pressButton\\" onclick=\\"postToURL('http://${clock.prefs.name}.local/checkForUpdate',{submit:'submit'})\\">Update</button></div>";
else
	document.getElementById("id_update_html").innerHTML = "<h2>Up to date</h2>Version: ${clock.getbuildstring()}<p>"+version_description();</script>`;
		}
		else if (clock.ota.error !== 0) {
			body += `<hr><h2>Update</h2><div><b>Error on previous update attempt: ${clock.ota.error}.</b></div><p>`;
			clock.ota = undefined;
		}
		return body;
	}

	static clockStyleSection(clock, server) {
		let i;
		let body = `<form action="/style" method="post"><h2>Style</h2><div><label>Style:</label><select class="select-css" id="style" name="style" onchange="showStyleDiv(this)">`;

    	for (i=0; i<clock.styles.length; i++) {
        	body += `<option value="${clock.styles[i].tag}"`;
        	if (clock.currentStyle === clock.styles[i])
            	body += ' selected';
        	body += `>${clock.styles[i].name}</option>`;
    	}
    	body += '</select><p>';

    	for (i=0; i<clock.styles.length; i++) {
        	body += `<div id="hidden_style_${clock.styles[i].tag}"`;
        	if (clock.currentStyle === clock.styles[i])
            	body += ` style="display:block;">`;
        	else
            	body += ` style="display:none;">`;
        	body += clock.styles[i].options_html();
        	body += `</div>`;
    	}

    	body += `</div><p>`;
    	body += clock.currentStyle.base_options_html();

    	// brightness slider
    	body += `<div><label>Brightness:</label>`;
    	body += html_content.slider("brightness", clock.prefs.brightness);

		body += `</div><p><div><button class="button pressButton">Set Style</button></div></form>`;
		return body;
	}

	static selectOptions(name, set, value, useIndex=0) {
		let body = `<select class="select-css" name="${name}">`;
		if (useIndex) {
			for (let i=0; i<set.length; i++) {
				body += `<option value=${i}`;
				if (undefined !== value && value === i)
					body += ' selected';
				body += '>' + set[i].name + '</option>';
			}
		}
		else {
			for (let i=0; i<set.length; i++) {
				body += `<option value=${set[i]}`;
				if (undefined !== value && value == set[i])
					body += ' selected';
				body += '>' + set[i] + '</option>';
			}
		}
		return body + `</select>`;
	}
	
	static timeFormatted(v) {
		let m = v%100;
		let h = (v/100)|0;
		return `${("0"+h).slice(-2)}:${("0"+m).slice(-2)}`;
	}

	static clockTailSection(clock) {
		let i;
		let body = `<h2>Tail</h2>
<form action="/tail" method="post">
<div><p>
<input type="checkbox" name="tail_on"  ${(clock.prefs.tail_on?" checked":"")} value="1">${SPACES}
<label>Tail on</label>

<p><input type="checkbox" name="tail_sched"  ${(clock.prefs.tail_sched?" checked":"")} value="1">${SPACES}
<label>Use schedule</label>${SPACES}
Turn on at: <input type="time" name="tail_time_on" value="${html_content.timeFormatted(clock.prefs.tail_time_on)}">${SPACES}
Turn off at: <input type="time" name="tail_time_off" value="${html_content.timeFormatted(clock.prefs.tail_time_off)}">

<p><label>Display all pixels as tail</label>
<input type="checkbox" name="tail_only" ${(clock.prefs.tail_only?" checked":"")} value="1"><p>
<label>Tail Brightness:</label>
${html_content.slider("tail_brightness", clock.prefs.tail_brightness)}
</div><p>
<div><label name="extra">Tail length: </label>
<input type="text" id="extra" name="extra" value="${clock.prefs.extra}">
${html_content.selectOptions("tail_order", clock.display.supportedFormats, clock.prefs.tail_order)}
</div><p>
<div><button class="button pressButton">Set Tail options</button></div></form>`;
		return body;
	}

	static clockOptionsSection(clock) {
		let i;
		let body = `<h2>Clock Options</h2>
<form action="/options" method="post">
<div><label>Name:</label>${SPACES}
<input type="text" width=25 id="clock_name" name="clock_name" value="${clock.prefs.name}">
${SPACES}<a href="http://${clock.prefs.name}.local">
http://<b>${clock.prefs.name}</b>.local</a></div><p>
<div><label>Timezone:</label>
${html_content.selection("timezone", html_content.tzNames, clock.prefs.tz)}
${SPACES}<label>12 hour:</label>
<input type="checkbox" name="twelve" ${(clock.prefs.twelve?" checked":"")} value="1">
${SPACES}<label>Daylight Savings:</label>
${html_content.selectOptions("dst", clock.prefs.dst_types, clock.prefs.dst_types[clock.prefs.dst], 0)}
</div><p>
<div><label>Layout:</label>
${html_content.selectOptions("layout", config.seven_segments, clock.prefs.layout, 1)}
${SPACES}
<label>Pin: </label>
${html_content.selection("pin", clock.prefs.neopixel_pins, clock.prefs.pin, 0)}${SPACES}
<label>Leading zero</label>
<input type="checkbox" name="zero" ${(clock.prefs.zero?" checked":"")} value="1">
</div><p><div>
<label>Button Pins: Left: </label>
${html_content.selection("buttonA", clock.prefs.button_pins, clock.prefs.buttonA, 0)}
${SPACES}<label>Right: </label>
${html_content.selection("buttonB", clock.prefs.button_pins, clock.prefs.buttonB, 0)}
</select></div>
<p>
<div><button class="button pressButton">Set Clock options</button></div></form>`;
		return body;
	}

	static clockSetTimeSection(clock) {
		let now = new Date();
		let t = now.getHours() * 100 + now.getMinutes();
		let ret = `<h2>Set time</h2><p>Set time manually if you don't have a network connection.<p>
<form action="/setTime" method="post">
Time: <input type="time" name="set_clock_time" value="${html_content.timeFormatted(t)}"><p>
<div><button class="button pressButton">Set Time</button></div></form>`;
		return ret;
	}

	static clockResetPrefsSection(clock) {
		return `<h2>Reset all</h2>Reset all preferences to factory default.<p>You'll have to reconnect to the <b>clock</b> access point and reconfigure all of your settings.<p><div><button class="button pressButton" onclick="postToURL('http://${clock.prefs.name}.local/reset',{submit:'submit'})">Reset all</button></div>`;
	}

	
	static head(name, server) {
		return `<html><head><title>${name}</title><link rel="stylesheet" href="${server}clock.css" type="text/css"><meta charset="utf-8"/></head>`;
	}

	static masthead(prod, name) {
		return `<p class="inline prodTitle">ModClock</p> ${SPACES}${SPACES}${SPACES}<p class="inline configLink">http://${name}.local</p>`;
	}

	static ota_status(val, max) {
		return `<div class="high_impact"><b>Updating</b> - Received ${val} of ${max === undefined ? "unknown" : max}</div><p>`;
	}

	static selection_bar(selections, selected) {
		let msg = `<div>`;
		for (let i=0; i<selections.length; i++) {
			msg += `<a href="${selections[i].link}" class="button ${i==selected?"buttonHilite":"buttonShadow"}"`;
			if (undefined !== selections[i].target)
				msg += `target="${selections[i].target}"`;
			msg += `>${selections[i].title}</a>`;
		}

		return msg + `</div>`;
	}

};


html_content.apMsgHead = `<title>${PROD_NAME}</title></head>`;
html_content.apMsgPrefix = `<body>`;
html_content.apMsgSuffix = `</body></html>`;

html_content.tzNames = [
	"Samoa", "Hawaii", "Alaska", "Pacific", "Mountain", "Central",
	"Eastern", "Atlantic", "Uruguay", "SGSSI", "Azores", "Greenwich Mean",
	"Central European", "Eastern European", "Indian Ocean", "Arabian",
	"Pakistan", "Bangladesh", "Thailand", "China", "Japan",
	"Australian Eastern", "Vanuatu", "New Zealand",
];

Object.freeze(html_content, 1);
Object.freeze(html_content.prototype, 1);

export default (html_content);

