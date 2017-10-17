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

import {XML} from "xml";

let string = `<?xml version="1.0"?>
	<catalog>
		<book id="bk101">
			<author>Gambardella, Matthew</author>
			<title>XML Developer's Guide</title>
			<genre>Computer</genre>
			<price>44.95</price>
			<publish_date>2000-10-01</publish_date>
			<description>An in-depth look at creating applications with XML.</description>
		</book>
	</catalog>`;

let object = XML.parse(string);
trace(JSON.stringify(object) + "\n");
trace(XML.serialize(object) + "\n");

debugger;
