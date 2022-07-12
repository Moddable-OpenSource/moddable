# ROM Colors
Copyright 2020 Moddable Tech, Inc.<BR>
Revised: August 12, 2020

## Opportunity

In XS, instances are mostly linked list of properties with a key and a value. To get a value by key, the runtime iterates the linked list to find a property with a matching key.

Of course XS already implements several optimizations to access properties by index: in array instances, in closures, in global and local scopes. 

The XS linker prepares most classes, objects, prototypes, functions to be accessed straightly from ROM. Such instances and their properties never change. That is an opportunity to access properties by index, especially since iterating a linked list in ROM is not optimal.

Intuitively indexes could be chosen for keys, then instances could be rearranged to have matching properties at that index. But if indexes are chosen too naively, instances and properties could take a lot of memory. So a technique is necessary to choose indexes while keeping instances and properties as dense as possible.

## Graph Coloring

The technique is based on graph coloring. Similar techniques are applied to object programming languages for decades. See for instance:

- André and Royer, [Optimizing method search with lookup caches and incremental coloring](https://dl.acm.org/doi/abs/10.1145/141936.141947), OOPSLA '92 proceedings.

- Driesen, [Selector table indexing & sparse arrays](https://dl.acm.org/doi/abs/10.1145/165854.165902), OOPSLA '93 proceedings.

When intrinsics and all modules are preloaded, the XS linker traverses all instances and their properties to build a conflict graph. The nodes are the keys, the edges are the conflicts. Two keys conflict if there is an instance that has properties for both keys. Choosing indexes for keys is equivalent to coloring nodes in the graph.

Once the graph is colored, each key has a color that can be used to access a property by index. 

	property = instance[key.color]

Since non conflicting keys can reuse the same color, the runtime has to check if the property matches the key.

	if (property.key == key)
		return property
	
Optimal graph coloring could be complex, but coloring from the most to the least conflicted key usually gives good enough results: the number of colors is significantly smaller than the number of keys. 
	
## Memory Layout

What remains to be done is to reorganize the ROM so properties are where the color of their key wants them to be.

Instances and properties are slots. Initially the XS linker allocates slots in the order instances and properties are created.

For instance, here are part of the `Math` object with a few properties, followed by the `JSON` object.

|Slot|Next|Kind|Key|   |
|----|----|----|---|---|
|0|-> 1|Instance||*Math*
|1|-> 2|Property|imul|
|2|-> 3|Property|max|
|3|-> 4|Property|min|
|4|NULL|Property|sign|
|5|-> 6|Instance||*JSON*
|6|-> 7|Property|parse|
|7| NULL |Property|stringify|

Firstly keys are colored. Since keys can conflict in several instances, their colors are never sequential.

|Key|Color|
|---|-----|
|imul|3
|max|1
|min|4
|parse|4
|sign|5
|stringify|5

Secondly the properties are moved to the color of their key:

|Slot|Next|Kind|Key|   |
|----|----|----|---|---|
|0|-> 3|Instance||*Math*
|1|-> 4|Property|max|
|2| NULL |||
|3|-> 1|Property|imul|
|4|-> 5|Property|min|
|5| NULL |Property|sign|
|6|-> 10|Instance||*JSON*
|7| NULL |||
|8| NULL |||
|9| NULL |||
|10|-> 11|Property|parse|
|11| NULL |Property|stringify|

That is enough to access properties by index but there are holes! Here above slots `2`, `7`, `8`, and `9` are unused. Graph coloring minimizes the number of colors, but because of conflicts, some instances can be sparse.

Eventually instances are moved to fill the holes.

|Slot|Next|Kind|Key|   |
|----|----|----|---|---|
|0|-> 3|Instance||*Math*
|1|-> 4|Property|max|
|2|-> 6|Instance||*JSON*
|3|-> 1|Property|imul|
|4|-> 5|Property|min|
|5| NULL |Property|sign|
|6|-> 7|Property|parse|
|7| NULL |Property|stringify|

Now the two objects are intertwined to reduce the memory footprint. There are usually enough instances without properties or with a few properties to fill all holes.

> Despite all movements, the order of the linked lists is maintained. That is required by several object traversal functions. 

## Results

Here is a simple test. 

	const math = Math;
	const now = Date.now();
	for (let i = 0; i < 100000; i++) {
		math.imul(i, 1)
		math.max(i, 0)
		math.min(i, 0)
		math.sign(i)
	}
	trace((Date.now() - now) + "\n");



The `Math` object is in ROM and its `imul`, `max`, `min` and `sign` properties are accessed 100000 times. The `Math` object is cached into a `const` to avoid the interference of global scope optimizations, the properties are selected to avoid floating point operations. 

Here are results without and with the optimization:

|Device|Without|With|Gain|
|------|-------|----|----|
|Moddable One|15066|11403|24%|
|Moddable Two|2906|2353|19%|
|simulator (macOS)|285|260|9%|

That is interesting: less the device is performant, more the gain is significant.
