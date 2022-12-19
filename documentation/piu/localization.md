# Localization
Copyright 2017 Moddable Tech, Inc.<BR>
Revised: March 3, 2017

## Dictionaries

When using JavaScript, the most obvious format to localize strings is a dictionary. Applications use common keys to access localized strings.

	var en = {
		"I love you": "I love you",
		"Me neither": "Me neither",
	};
	
	var fr = {
		"I love you": "Je t'aime",
		"Me neither": "Moi non plus",
	};

	var language = fr;
	function localize(it) {
		return language[it];
	}
	
It is not always possible to use the English string as the key, because of homonyms, contexts, etc. However, when it is possible, it is recommended: the code is easier to read and obvious redundancies are avoided. 

The trivial examples here above are used to compare solutions here under. Of course real applications use dictionaries with thousands of entries, small and large keys and values, so multiply accordingly.

## JSON Files

Since usually applications need only one language at a time, dictionaries can be JSON files, loaded and unloaded when the user selects a language.

#### en.json

	{"I love you":"I love you","Me neither":"Me neither"}
	
#### fr.json

	{"I love you":"Je t'aime","Me neither":"Moi non plus"}
	
Storing JSON files in ROM is a waste since all dictionaries have to define all keys. For instance here above the keys `I love you` and `Me neither` are repeated in the English and French dictionaries.

ROM|Size
:---|---:
en.json|53 bytes
fr.json|54 bytes

Loading JSON files uses a lot of RAM. Firstly the keys populate the XS symbols table. Secondly a dictionary allocales one slots for itseft, one slot by entry and one chunk by string.

RAM|Size
:---|---:
symbols|64 bytes
en.json|80 bytes
fr.json|84 bytes


## JavaScript Modules

Instead of JSON files, dictionaries could be JavaScript modules that XS can compile, link and preload in ROM.

#### en.js

	export default {"I love you":"I love you","Me neither":"Me neither"}
	
#### fr.js

	export default {"I love you":"Je t'aime","Me neither":"Moi non plus"}

That would avoid redundant keys in ROM and would use no RAM. However the process would still populate the XS symbols table with keys, use six slots by dictionary for the module, export and object, and use one slot by entry.

ROM|Size
:---|---:
symbols|64 bytes
en.js|160 bytes
fr.js|164 bytes

Moreover, since XS has no special case for such objects, the time to lookup a string in ROM would be significant for dictionaries with a lot of entries.

## Strings Tables

The first optimization is to use strings tables instead of JSON files or JavaScript modules. Each table begins with the length of the table followed by the offsets of the strings in the table. All numbers are little-endian 32-bit integers. 

#### locals.en.mhr

	2 12 23 I love you Me neither
	
#### locals.fr.mhr

	2 12 22 Je t'aime Moi non plus

Like most resources in Moddable applications, strings tables are never loaded into RAM. XS allows to use read-only strings, so JavaScript strings can refer to the strings in the strings tables themselves.

ROM|Size
:---|---:
locals.en.mhr|34 bytes
locals.fr.mhr|35 bytes

A tool can generate the strings table from the JSON files. Applications can get strings from the tables thru a host function.

## Strings Indexes

Now something is of course necessary to map keys to indexes into the tables, so `I love you` maps to `0`, `Me neither ` maps to `1`, etc. 

Again a dictionary could be used, at least there would be only one dictionary for all languages.

	var locals = {
		"I love you": 0,
    	"Me neither": 1,
	};
	var en = new StringTable("locals.en.mhr");
	var fr = new StringTable("locals.fr.mhr");
	var language = fr;
	function localize(it) {
		return language.get(locals[it]);
	}
	
But such a dictionary would have the already mentioned drawbacks: populating the XS symbols table and taking time to lookup an index.

## Minimal Perfect Hashing

When all keys and results are known, a perfect hash function can map keys into results without collisions. A practical solution is to use two hash functions and an intermediary table. The first hash function maps keys into seeds. The second hash function uses the seeds to maps keys into results. The seeds table can be sparse, but both the seeds and results tables contain only one entry by result. Finding the seeds take some time but is done by a tool at build time.

Here are some references: 

- [CMPH](http://cmph.sourceforge.net/index.html): a C library with a lot of algorithms and explanations.
- [Steve Hanov's blog](http://stevehanov.ca/blog/index.php?id=119): a detailed presentation of the practical solution in PHP.
- [mixu/perfect](https://github.com/mixu/perfect): a node.js port.

Here the results are indexes into the strings tables. So the results table does not need to be stored, the results table is only used to reorder the strings tables. 

Only the seeds table needs to be stored. Negative seeds signal that the second hash function can be skipped, the index is the absolute of the seed minus one.

#### locals.mhi (release)

	2 -2 -1

The keys themselves do not need to be stored if applications use only valid keys. For the sake of debugging invalid keys, a debug table can also be generated. The debug table contains the keys ordered by the results table. When a key is mapped into an index, the debug table can be used to check if the key matches. The debug table is appended to the seeds table.

#### locals.mhi (debug)

	2 -2 -1 2 24 35 I love you Me neither

With this technique:

- Tables are in ROM, but are smaller than JavaScript modules.
- Localization does not populate the XS symbols table.
- Lookups are significantly faster, independently of the size of the dictionaries.

ROM|Size
:---|---:
locals.mhi (release)|12 bytes
locals.mhi (debug)|46 bytes

## mclocal

**mclocal** is a command line tool that generates strings tables and seeds table from JSON files. 

For instance

	mclocal en.json fr.json
	
will generate the `locals.en.mhr`, `locals.fr.mhr` and `locals.mhi` here above.

**mclocal** unions the keys from all the JSON files and reports missing keys in the JSON files.

By convention, **mcconfig** will generate a make file with a rule to call **mclocal** for all JSON files in a `strings` directory.

### Arguments

	mclocal file+ [-d] [-o directory] [-r name]

- `file+`: one or more JSON files.
- `-d`: to generate the debug table.
- `-o directory`: the output directory. Defaults to the current directory.
- `-r name`: the name of the output file. Defaults to `locals`.


## Locals

Piu defines a class, `Locals`, to get localized strings and to switch languages.

	var locals = new Locals;
	
The constructor takes two arguments, `name` and `language`. The defaults are `locals` and `en`. Resources are accessed by combining `name`, `language` and the `.mhi` or `.mhr` extensions.
	
Applications switch the language with an accessor.
	
	var what = locals.get("I love you"); // what == "I love you"
	locals.language = "fr";
	var quoi = locals.get("I love you");	 // quoi == "Je t'aime"
	
For convenience, applications can define a global function to localize strings.

	global.localize = function(it) {
		return locals.get(it);
	}	
	
	



