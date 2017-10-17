/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

function isIdentifierFirst(c) {
	return (((65 <= c) && (c <= 90)) || ((97 <= c) && (c <= 122)) || (c == 24) || (c == 95)) ? 1 : 0;
}

function isIdentifierNext(c) {
	return (((48 <= c) && (c <= 57)) || ((65 <= c) && (c <= 90)) || ((97 <= c) && (c <= 122)) || (c == 24) || (c == 95)) ? 1 : 0;
}

function nameToIdentifier(name) {
	var result = "";
	var c = name.length;
	var i = 0
	var code = name.charCodeAt(i);
	if (isIdentifierFirst(code))
		result += name[i];
	else
		result += "_";
	for (i++; i < c; i++) {
		var code = name.charCodeAt(i);
		if (isIdentifierNext(code))
			result += name[i];
		else
			result += "_";
	}
	return result;
}

function layerToItem(layer, frameDuration) {
	var item = {
		file: layer.source.file,
		path: layer.name,
		width: layer.width,
		height: layer.height,
	};
	var name = item.path;
	var dot = name.lastIndexOf(".");
	item.name = name.slice(0, dot);
	item.id = nameToIdentifier(item.name);
	var anchors = layer.anchorPoint;
	var positions = layer.position;
	var anchor = anchors.valueAtTime(0, false);
	var position = positions.valueAtTime(0, false);
	item.x = Math.round(position[0] - anchor[0]);
	item.y = Math.round(position[1] - anchor[0]);
	var c = positions.numKeys;
	if (c > 1) {
		var time = item.time = positions.keyTime(1);
		var limit = positions.keyTime(c);
		item.duration = limit - item.time;
		item.xSteps = [];
		item.ySteps = [];
		for (; time < limit; time += frameDuration) {
			anchor = anchors.valueAtTime(time, false);
			position = positions.valueAtTime(time, false);
			item.xSteps.push(Math.round(position[0] - anchor[0]));
			item.ySteps.push(Math.round(position[1] - anchor[0]));
		}
		anchor = anchors.valueAtTime(limit, false);
		position = positions.valueAtTime(limit, false);
		item.xSteps.push(Math.round(position[0] - anchor[0]));
		item.ySteps.push(Math.round(position[1] - anchor[0]));
	}
	return item;
}

function writePositions(file, item) {
	if (item.duration) {
		file.write('const ', item.id, 'X = ');
		writeSteps(file, item.xSteps);
		file.write(';\n');
		file.write('const ', item.id, 'Y = ');
		writeSteps(file, item.ySteps);
		file.write(';\n');
	}
}

function writeSteps(file, steps) {
	var c = steps.length;
	var step = steps[0];
	for (var i = 1; i < c; i++) {
		if (step != steps[i])
			break;
	}
	file.write('[ '); 
	if (i == c)
		file.write(step); 
	else {
		for (var i = 0; i < c; i++) {
			if (i)
				file.write(',');
			file.write(steps[i]); 
		}
	}
	file.write(' ]'); 
}

function f() {
	var folder = Folder.selectDialog();
	if (!folder) return;
	var uri = folder.absoluteURI + "/";
	
	var project = app.project;
	var composition = project.activeItem;
	var id = nameToIdentifier(composition.name);
	var layers = composition.layers;
	var c = layers.length;
	var items = [];
	var frameDuration = composition.frameDuration;
	for (var i = 1; i <= c; i++) {
		var layer = layers[i];
		if (layer instanceof AVLayer) {
			var source = layer.source;
			if (source) {
				var file = source.file;
				if (file) {
					items.unshift(layerToItem(layer, frameDuration));
				}
			}
		}
	}
	c = items.length;
	for (var i = 0; i < c; i++) {
		var item = items[i];
		item.file.copy(uri + item.path); 
	}

	var file = new File(uri + "manifest.json");
	file.open("w","TEXT","????");
	file.write('{\n'); 
	file.write('\t"creation": {\n'); 
	file.write('\t\t"stack": "512",\n'); 
	file.write('\t},\n'); 
	file.write('\t"include": "$(MODDABLE)/examples/piu/all.json",\n'); 
	file.write('\t"modules": {\n'); 
	file.write('\t\t"*": "./main",\n'); 
	file.write('\t},\n'); 
	file.write('\t"resources":{\n'); 
	file.write('\t\t"*": [\n'); 
	file.write('\t\t\t"./main",\n'); 
	for (var i = 0; i < c; i++) {
		var item = items[i];
		file.write('\t\t\t"./', item.name, '",\n'); 
	}
	file.write('\t\t]\n'); 
	file.write('\t},\n'); 
	file.write('}\n'); 
	file.write('\n'); 
	file.close();
	
	var file = new File(uri + "main.js");
	file.open("w","TEXT","????");
	
	file.write('import { Skin, Texture, Behavior, Content, Container, Application } from "piu/MC";\n'); 
	file.write('import Timeline from "piu/Timeline";\n'); 
	file.write('\n'); 
	
	var color = composition.bgColor;
	file.write('const ', id, 'Skin = { fill: rgb(', Math.round(color[0] * 255), ', ', Math.round(color[1] * 255), ', ', Math.round(color[2] * 255), ') };\n'); 
	for (var i = 0; i < c; i++) {
		var item = items[i];
		file.write('const ', item.id, 'Skin = { texture:{ path:"', item.path, '" }, x:0, y:0, width:', item.width, ', height:', item.height, ' };\n');
		writePositions(file, item);
	}
	file.write('\n'); 
	
	file.write('let ', id, 'Screen = Container.template($ => ({\n'); 
	file.write('\twidth:', composition.width, ', height:', composition.height, ', skin:', id, 'Skin,\n'); 
	file.write('\tcontents: [\n'); 
	for (var i = 0; i < c; i++) {
		var item = items[i];
		file.write('\t\tContent($, { anchor:"', item.id, '", left:', item.x, ', top:', item.y, ', skin:', item.id, 'Skin } ),\n'); 
	}	
	file.write('\t]\n'); 
	file.write('}));\n'); 
	file.write('\n'); 

	file.write('class ', id, 'Timeline extends Timeline {\n'); 
	file.write('\tconstructor(screen, data) {\n'); 
	file.write('\t\tsuper();\n');
	for (var i = 0; i < c; i++) {
		var item = items[i];
		file.write('\t\tlet ', item.id, ' = data.', item.id, ';\n'); 
	}	
	for (var i = 0; i < c; i++) {
		var item = items[i];
		if (item.duration)
			file.write('\t\tthis.on(', item.id, ', { x:', item.id, 'X, y:', item.id, 'Y }, ', Math.round(1000 * item.duration), ', null, ',  Math.round(1000 * item.time), ', 0);\n');
	}	
	file.write('\t}\n'); 
	file.write('}\n'); 
	file.write('\n'); 
	
	file.write('let ', id, 'Application = Application.template($ => ({\n'); 
	file.write('\tBehavior: class extends Behavior {\n'); 
	file.write('\t\tonCreate(application) {\n'); 
	file.write('\t\t\tlet data = {};\n'); 
	file.write('\t\t\tlet screen = new ', id, 'Screen(data);\n'); 
	file.write('\t\t\tapplication.add(screen);\n'); 
	file.write('\t\t\tthis.timeline = new ', id, 'Timeline(screen, data);\n'); 
	file.write('\t\t\tapplication.duration = this.timeline.duration;\n'); 
	file.write('\t\t\tapplication.start();\n'); 
	file.write('\t\t}\n'); 
	file.write('\t\tonFinished(application) {\n'); 
	file.write('\t\t\tapplication.time = 0;\n'); 
	file.write('\t\t\tapplication.start();\n'); 
	file.write('\t\t}\n'); 
	file.write('\t\tonTimeChanged(application) {\n'); 
	file.write('\t\t\tthis.timeline.fraction = application.fraction;\n'); 
	file.write('\t\t}\n'); 
	file.write('\t},\n'); 
	file.write('}));\n'); 
	file.write('\n'); 
	
	file.write('export default new ', id, 'Application(null, { displayListLength:4096, touchCount:0 });\n'); 
	file.write('\n'); 
	
	file.close();
}

f();
