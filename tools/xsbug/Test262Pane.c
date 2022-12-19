/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 */

#include "piuCode.h"
#include "yaml.h"

static yaml_node_t *fxGetMappingValue(yaml_document_t* document, yaml_node_t* mapping, char* name);

yaml_node_t *fxGetMappingValue(yaml_document_t* document, yaml_node_t* mapping, char* name)
{
	yaml_node_pair_t* pair = mapping->data.mapping.pairs.start;
	while (pair < mapping->data.mapping.pairs.top) {
		yaml_node_t* key = yaml_document_get_node(document, pair->key);
		if (!strcmp((char*)key->data.scalar.value, name)) {
			return yaml_document_get_node(document, pair->value);
		}
		pair++;
	}
	return NULL;
}

void Test262Context_getMetadata(xsMachine* the)
{
	FILE* file = NULL;
	size_t size;
	char* buffer = NULL;
	char* begin;
	char* end;
	yaml_parser_t _parser;
	yaml_parser_t* parser = NULL;
	yaml_document_t _document;
	yaml_document_t* document = NULL;
	yaml_node_t* root;
	yaml_node_t* value;
	int sloppy = 0;
	int strict = 0;
	int module = 1;
	int async = 0;
	int pending = 0;
	xsVars(1);
	xsResult = xsNewObject();
	xsVar(0) = xsNewArray(0);	
	
	file = fopen(xsToString(xsArg(0)), "rb");
	if (!file) goto bail;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (!size) goto bail;
	fseek(file, 0, SEEK_SET);
	buffer = malloc(size + 1);
	if (!buffer) goto bail;
	if (fread(buffer, 1, size, file) != size) goto bail;	
	buffer[size] = 0;
	fclose(file);
	file = NULL;
	
	begin = strstr(buffer, "/*---");
	if (!begin) goto bail;
	begin += 5;
	end = strstr(begin, "---*/");
	if (!end) goto bail;

	if (!yaml_parser_initialize(&_parser)) goto bail;
	parser = &_parser;
	yaml_parser_set_input_string(parser, (unsigned char *)begin, end - begin);
	if (!yaml_parser_load(parser, &_document)) goto bail;
	document = &_document;
	root = yaml_document_get_root_node(document);
	if (!root) goto bail;
		
	sloppy = 1;	
	strict = 1;	
	module = 0;	
	value = fxGetMappingValue(document, root, "includes");
	if (value) {
		yaml_node_item_t* item = value->data.sequence.items.start;
		while (item < value->data.sequence.items.top) {
			yaml_node_t* node = yaml_document_get_node(document, *item);
			xsCall1(xsVar(0), xsID_push, xsString((char*)node->data.scalar.value));
			item++;
		}
	}

	value = fxGetMappingValue(document, root, "negative");
	if (value) {
		value = fxGetMappingValue(document, value, "type");
		xsSet(xsResult, xsID_negative, xsString(value->data.scalar.value));
	}
	else
		xsSet(xsResult, xsID_negative, xsNull);


	value = fxGetMappingValue(document, root, "flags");
	if (value) {
		yaml_node_item_t* item = value->data.sequence.items.start;
		while (item < value->data.sequence.items.top) {
			yaml_node_t* node = yaml_document_get_node(document, *item);
			if (!strcmp((char*)node->data.scalar.value, "onlyStrict")) {
				sloppy = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "noStrict")) {
				strict = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "module")) {
				sloppy = 0;
				strict = 0;
				module = 1;
			}
			else if (!strcmp((char*)node->data.scalar.value, "raw")) {
				strict = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "async")) {
				async = 1;
			}
			else if (!strcmp((char*)node->data.scalar.value, "CanBlockIsTrue")) {
				sloppy = 0;
				strict = 0;
				module = 0;
				pending = 1;
			}
			item++;
		}
	}

	value = fxGetMappingValue(document, root, "features");
	if (value) {
		yaml_node_item_t* item = value->data.sequence.items.start;
		while (item < value->data.sequence.items.top) {
			yaml_node_t* node = yaml_document_get_node(document, *item);
			if (0
 			||	!strcmp((char*)node->data.scalar.value, "Atomics.waitAsync")
  			||	!strcmp((char*)node->data.scalar.value, "ShadowRealm")
 			||	!strcmp((char*)node->data.scalar.value, "Temporal")
 			||	!strcmp((char*)node->data.scalar.value, "arbitrary-module-namespace-names")
 			||	!strcmp((char*)node->data.scalar.value, "array-grouping")
 			||	!strcmp((char*)node->data.scalar.value, "decorators")
 			||	!strcmp((char*)node->data.scalar.value, "import-assertions")
 			||	!strcmp((char*)node->data.scalar.value, "json-modules")
#ifndef mxRegExpUnicodePropertyEscapes
 			||	!strcmp((char*)node->data.scalar.value, "regexp-unicode-property-escapes")
#endif
			||	!strcmp((char*)node->data.scalar.value, "regexp-v-flag")
			) {
				sloppy = 0;
				strict = 0;
				module = 0;
				pending = 1;
			}
			item++;
		}
	}
bail:	
	xsSet(xsResult, xsID_paths, xsVar(0));
	xsSet(xsResult, xsID_async, xsBoolean(async));
	xsSet(xsResult, xsID_sloppy, xsBoolean(sloppy));
	xsSet(xsResult, xsID_strict, xsBoolean(strict));
	xsSet(xsResult, xsID_module, xsBoolean(module));
	xsSet(xsResult, xsID_pending, xsBoolean(pending));
	if (document)
		yaml_document_delete(document);
	if (parser)
		yaml_parser_delete(parser);
	if (buffer)
		free(buffer);
	if (file)
		fclose(file);
}
