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

export class html_content {

	static clockScripts(style, server) {
		return `
<script src="${server}current_version.js?${Time.ticks}"></script>
<script src="${server}html5kellycolorpicker.min.js"></script>
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
</script>\n`;
	}


	static resetPrefsResp() {
		return `Resetting prefs.<p>Reconnect to the <b>clock</b> access point and go to <a href=http://clock.local>http://clock.local</a> to configure.<p>`;
	}

	static changeSSIDResp(ssid, name) {
		return `Trying ssid:<b>${ssid}</b>. Reconnect to the Wi-Fi access point ${ssid} and try <a href="http://${name}.local">http://${name}.local</a><br>`;
	}

	static redirectHead(name, sec=3) {
		return `<html><head><meta http-equiv="refresh" content="${sec}; url=http://${name}.local"><title>${name}</title></head>`;
	}

	static bodyPrefix() {
		return `<body>`;
	}

	static bodySuffix(msg) {
		return `<div><hr>${msg}</div>\n</body></html>`;
	}

	static noAccessPointSet() {
		return `Please set the name of your clock and connect to an access point.<p>`;
	}

	static accessPointSection(apList, selected="", name) {
		let body = `<h2>Access point</h2><form action="/set-ssid" method="post"><div><label name="ssid">SSID: </label>`;

	    if (apList.length > 0) {
	        body += '<select name="ssid_select">';
			apList.forEach(function(ap) {
				body += `<option value="${ap.ssid}" `;
				if (ap.ssid == selected)
					body += " selected";
				body += `>${ap.ssid}</option>`;
			});

			body += `</select>&nbsp;&nbsp;Other:`;
		}

		body += `<input type="text" id="ssid" name="ssid"><p>
<label for="password">password: </label>
<input type="password" id="password" name="password" minlength="8"></div><p>
<div><label name="name">Name:</label>
<input type="text" id="name" name="clock_name" value="${name}"></div><p>
<div class="button"><button>Set SSID</button></div>
</form>`;
	    if (apList.length > 0)
			body += `<div class="button"><button onclick="postToURL('http://${name}.local/rescanSSID',{submit:'submit'})">Rescan SSIDs</button></div>`;
		return body;
	}


	static selection(name, items, selected) {
		let x = `<select name="${name}">`;
		for (let i=0; i<items.length; i++) {
			x += `<option value=${i}`;
			if (selected == i)
				x += ` selected`;
			x += `>${items[i]}</option>`;
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
	document.getElementById("id_update_html").innerHTML = "<h2>Update Available</h2>Clock version: ${clock.getbuildstring()}<p>Server version: <b>" + current_version() + "</b><p><div class='button'><button onclick=\\"postToURL('http://${clock.prefs.name}.local/checkForUpdate',{submit:'submit'})\\">Update</button></div>";
else
	document.getElementById("id_update_html").innerHTML = "<h2>Up to date</h2>Version: ${clock.getbuildstring()}<p>";</script>`;
		}
		else if (clock.ota.error !== 0) {
			body += `<hr><h2>Update</h2><div><b>Error on previous update attempt: ${clock.ota.error}.</b></div><p>`;
			clock.ota = undefined;
		}
		return body;
	}

	static clockStyleSection(clock) {
		let i;
		let body = `<form action="/style" method="post"><h2>Style</h2><div><label>Style:</label><select id="style" name="style" onchange="showStyleDiv(this)">`;

    	for (i=0; i<clock.styles.length; i++) {
        	body += `<option value="${clock.styles[i].tag}"`;
        	if (clock.currentStyle === clock.styles[i])
            	body += ' selected';
        	body += `>${clock.styles[i].name}</option>`;
    	}
    	body += '</select>';

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

    	// brightness sliders
    	body += `<div><label>Brightness:</label>`;
    	body += html_content.slider("brightness", clock.prefs.brightness);

    	body += `</div><p><h2>Tail</h2>Tail Brightness:</label>`;

    	body += html_content.slider("tail_brightness", clock.prefs.tail_brightness);
    	body += `</div><p><div><label name="extra">Extra pixels: </label>
<input type="text" id="extra" name="extra" value="${clock.prefs.extra}">
<select name="tail_order">`;

    	for (i=0; i<clock.display.supportedFormats.length; i++) {
        	body += `<option value=${i}`;
        	if (clock.prefs.tail_order == clock.display.supportedFormats[i])
				body += ' selected';
       		body += `>${clock.display.supportedFormats[i]}</option>`;
		}

    	body += `</select></div><p><div class="button"><button>Set Style</button></div></form>`;
		return body;
	}

	static clockOptionsSection(prefs) {
		let i;
		let body = `<h2>Clock Options</h2><form action="/clockOptions" method="post"><div><label>Name:</label>&nbsp;&nbsp;&nbsp;
<input type="text" id="clock_name" name="clock_name" value="${prefs.name}">
&nbsp;&nbsp;<a href="http://${prefs.name}.local">http://<b>${prefs.name}</b>.local</a></div><div><label>Timezone:</label>`;

	    body += html_content.selection("timezone", html_content.tzNames, prefs.tz);
		body += `</div><p><div><label>12 hour:</label>
<input type="checkbox" name="twelve" ${(prefs.twelve?" checked":"")} value="1">
&nbsp;&nbsp;<label>Daylight Savings:</label>
<input type="checkbox" name="dst"  ${(prefs.dst?" checked":"")} value="1">
</div><p><div><label>Layout:</label>
<select name="layout">`;

		for (i=0; i<config.seven_segments.length; i++) {
			body += '<option value=' + i;
			if (undefined !== prefs.layout && prefs.layout == i)
				body += ' selected';
			body += '>' + config.seven_segments[i].name + '</option>';
		}
		body += `</select>&nbsp;&nbsp;&nbsp;<label>Pin:</label><select name="pin">`;

		for (i=0; i<prefs.available_pins.length; i++) {
			body += `<option value=${prefs.available_pins[i]}`;
			if (undefined !== prefs.pin && prefs.pin == prefs.available_pins[i])
				body += ' selected';
			body += '>' + prefs.available_pins[i] + '</option>';
		}
		body += `</select></div><p><div class="button"><button>Set Clock options</button></div></form>`;
		return body;
	}

	static clockResetPrefsSection(clock) {
		return `<h2>Reset all</h2>Reset all preferences to factory default.<p>You'll have to reconnect to the <b>clock</b> access point and reconfigure all of your settings.<p><div class="button"><button onclick="postToURL('http://${clock.prefs.name}.local/reset',{submit:'submit'})">Reset all</button>`;
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

