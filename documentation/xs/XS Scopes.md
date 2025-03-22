# XS Scopes
Revised: June 25, 2024

## About

The best way to understand what XS does is to follow the execution of byte codes and see what happens to the stack and the heap. So that is mostly what this document does.

The overview is adapted from [XS7 @ TC-39](https://moddable.com/XS7-TC-39). That is the text of the presentation given in 2017 to introduce XS to TC39. Then there are four examples that explore some details about how XS handles scopes at runtime. 

<a id="overview"></a>	
## Overview

The overview mostly defines the vocabulary used when talking about XS, and presents its essential structures

### Machine

The main XS runtime structure is the machine. Each machine is its own ECMAScript realm and has its own heap, stack, keys, names table, symbols table, connection to the debugger, etc. 

XS can run several machines concurrently in several threads but one machine can only run in one thread at a time. XS provides a C programming interface for machines to communicate.

### Slot

The most ubiquitous XS runtime structure is the slot. Slots store booleans, numbers, references, etc. 

The size of a slot is four times the size of a pointer, so 16 bytes on 32-bit processors. 

- The first field of a slot is a pointer to the `next` slot. Slots are mostly used as linked lists. For example, objects are linked lists of properties. 
- The second field of a slot is its `id`, an index into the keys of the machine that owns the slot. For example, properties and variables have an `id`.
- The third field of a slot is its `flag`, with for instance bits for configurable, enumerable and writable. The garbage collector also uses the flag to mark the slot.
- The fourth field of a slot is its `kind`. It defines what is in the fifth field, the `value` of the slot. For example, if the `kind` is number, the value contains a double. 
- The fifth field of a slot is its `value`.

In the machine heap, there is a linked list of free slots, using the `next` field of the slots. The slot allocator remove slots from that list. The garbage collector sweeps unreachable slots by adding them to that list.

### Chunk

What does not fit into a slot goes into a chunk. Chunks store byte codes, strings, arrays, array buffers, bigints, etc. Chunks are always accessed thru slots. The kind field of a slot that points to the chunk defines what the chunk contains.

The size of chunks varies. The first four bytes of a chunk is the size of its data. The garbage collector uses the high-order bit of the size to mark the chunk. 

In the machine heap, chunks are allocated inside a memory block. In order to compact the memory block, the garbage collector sweeps unreachable chunks by relocating reachable chunks, then update slots that point to them with their relocated addresses.

XS allows frameworks and applications to use chunks for their own data using a specific kind of slot that references a chunk and hooks for the garbage collector. Such a slot works like a Handle, for those who remember the original Macintosh operating system.

### Byte Code

At the core of the XS runtime, there is a huge C function that is a byte code interpreter. The C function uses computed gotos to dispatch byte code, which is always faster than a loop and a switch, especially because of branch prediction. For details, see for example Eli Bendersky, [Computed goto for efficient dispatch tables](http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables)

The primary objective of byte code is of course to encode modules and scripts into something that is fast enough to decode. XS compiles modules and scripts into byte codes to handle their relative complexity once and only once. Then XS only runs byte code. Byte code is the only information exchanged between compile and run time.

The secondary objective of byte code is compression. I always try to find a balance between the vocabulary (the number of byte codes available to encode ECMAScript constructs) and the grammar (the number of byte codes necessary to encode ECMAScript constructs). The design of the byte code evolves with each version of XS.

In fact most byte codes do not look at all like assembly instructions. Most byte codes directly refer to ECMAScript expressions and statements. For example, there are byte codes to get and set properties, byte codes for unary and binary operators, byte codes for primitive values, etc.

A lot of byte codes have no values and take just 1 byte. Most byte codes with values have variations depending on the size of their values. A few examples:

- the integer byte code has variations for 1, 2 and 4 bytes values, 
- the branches byte codes have also variations for 1, 2 and 4 bytes values, depending on how far the interpreter has to jump. 
- since ECMAScript functions with more than 255 arguments and variables are rare, related byte codes have variations too, so most accesses and assignments take only two bytes.

Each machine has its current code pointer, which can be null if the machine is not executing byte code.

### Stack

The byte code interpreter is stack based. Byte codes push, pop or transform references and values to, from or on the machine stack. The machine stack is a stack of slots, separate from the C stack.

Each machine has its current stack pointer.

### Operators

#### Unary Operator

What happens to the stack for `typeof null`?

- **null**: Push null

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|-|-|Null|-|

- **typeof**: Apply the unary operator

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|-|-|String|"object"|

#### Binary Operator

What happens to the stack for `1 + 1`?

- **integer 1**: Push integer `1`

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|-|-|Integer|1|

- **integer 1**: Push integer `1`

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|-|-|Integer|1|
|-|-|-|Integer|1|

- **add**: Apply the binary operator

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|-|-|Integer|2|

### Function

When calling a function, XS does not use the C stack. The byte code interpreter uses a frame to remember where to go back and jumps to the first byte code of the function.

Each machine has a current frame pointer.

Firstly the calling function pushes `this` and a reference to the called function

|Next|ID|Flag|Kind |Value||
|---:|---|---|:---|:---|:---|
|-|-|-|Undefined|-|*this*
|-|-|-|Reference|Instance reference|*function*

Then executes byte codes like:

- **call**: Push slots to hold the target, result, frame and argument count
- **number NaN**: Push number `NaN` as first argument
- **run 1**: Set the argument count to 1 and store the current frame, code and scope pointers into the frame

|Next|ID|Flag|Kind |Value||
|---:|---|---|:---|:---|:---|
|-|-|-|Undefined|-|*this*
|-|-|-|Reference|Instance reference|*function*
|-|-|-|Undefined|-|*target*|
|-|-|-|Undefined|-|*result*|
|➔ Frame|-|-|Frame|Code pointer + Scope|
|-|-|-|Integer|1|*argument count*|
|-|-|-|Number|NaN|*first argument*|

With such a stack, XS jumps to the first byte code of the called function

The last byte code of a function is:

- **end**: Retrieve the byte code pointer and scope from the frame, reset the frame, scope and stack then push the result

|Next|ID|Flag|Kind |Value||
|---:|---|---|:---|:---|:---|
|-|-|-|Undefined|-|*result*|

With such a stack, XS jumps to the next byte code of the calling function

### C Function

XS implements built-in functions in C and modules can implement their functions in C too. So the byte code interpreter is re-entrant, in order to handle calls from and to such host functions.

### Script, Module and Function Scopes

At compile time, XS parses modules and scripts into syntax trees, then hoists definitions and variables, then scopes identifiers, then generates byte code. 

The first objective of scoping identifiers is to access and assign variables by index, on the stack, directly in locals, or indirectly in closures, instead of having to lookup identifiers. That is for performance.

The second objective of scoping identifiers is to create closures with only the necessary variables, since enclosing functions know what their enclosed functions use. That is to minimize memory use.

In strict mode, when there is a direct call to `eval`, XS creates closures for everything. For the evaluated string, XS generates different byte codes to lookup identifiers.

In non-strict mode, when there is a direct call to `eval` or a `with` statement, it is similar, except that XS also generates different byte codes to lookup identifiers outside the direct call to `eval` or inside the `with` statement.

### Block Scopes

Each function reserves slots on the stack for its variables. The number of reserved slots is the maximum number of variables used by the function and its blocks. 

Several byte codes move the current scope pointer down and up when entering and leaving blocks. Parallel blocks use the same indices.

#### Source code

```js
{
	let x = 0;
	{
		let y = 1;
	}
	{
		let z = 2;
	}
}
```

#### Byte Codes

- **reserve 2**: Reserve 2 slots on the stack
- **[0] new_local x**: Create local 0 with ID **x** and uninitialized value
- **integer 0**: Push integer `0`
- **let_local [0]**: Initialize local 0 (**x**) with the stack value as a `let`
- **pop**: Pop the stack
- **[1] new_local y**: Create local 1 with ID **y** and uninitialized value
- **integer 1**: Push integer `11`
- **let_local [1]**: Initialize local 1 (**y**) with the stack value as a `let`
- **pop**: Pop the stack
- **unwind 1**: Delete local 1
- **[1] new_local z**: Create local 1 with ID **z** and uninitialized value
- **integer 2**: Push integer `2`
- **let_local [1]**: Initialize local 1 (**z**) with the stack value as a `let`
- **pop**: Pop the stack
- **unwind 2**: Delete local 1 and 0

Initially, the current scope and stack pointers are the same. **reserve** moves the stack down but not the scope, **new_local** move the scope down, **unwind** moves the scope up and sets the slots to have no IDs, no flags and `undefined` values.

<a id="example1"></a>	
## Example 1

Let us start with something simple.

#### Source code

```js
let x = 0;
let y = 1;
const f = function(z) {
  x += z;
  y += z;
}
```

For each significant step, I will comment byte codes and present stack and heap slots at the end of the step. I omitted debugger related byte codes.

### Step 1: Closures and Locals Creation

Locals are stack slots only. Closures are stack slots plus heap slots for their values.

#### Byte Codes

- **reserve 3**: Reserve 3 slots on the stack
- **[0] new_closure x**: Create closure 0 with ID **x** and uninitialized value
- **[1] new_closure y**: Create closure 1 with ID **y** and uninitialized value
- **[2] new_local f**: Create local 2 with ID **f** and uninitialized value

> indices **[0, 1, 2]** are implicit: **reserve** moves the stack but not the scope, **new_closure** and **new_local** move the scope.

#### Stack Slots

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|x|-| Closure |Value reference ➔ 0|
|-|y|-| Closure |Value reference ➔ 1|
|-|f|-| Uninitialized |-|

#### Heap Slots

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|-|-|-| Uninitialized |-|
|1|-|-|-| Uninitialized |-|

### Step 2: Closures and Locals Initialization

Closures and locals can be initialized as `const`, `let` or `var`. All of them are always accessed by index.

#### Byte Codes

- **integer 0**: Push integer `0`
- **let_closure [0]**: Initialize closure 0 (**x**) with the stack value as a `let`
- **pop**: Pop the stack
- **integer 1**: Push integer `1`
- **let_closure [1]**: Initialize closure 1 (**y**) with the stack value as a `let`
- **pop**: Pop the stack

#### Stack Slots

No changes

#### Heap Slots

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|-|-|-| Integer |0|
|1|-|-|-| Integer |1|


### Step 3: Function and Environment Creation

Function instances have a byte code pointer and, if they have closures, an environment reference. Environment instances have properties which are closures that reference values.

#### Byte Codes

- **function f**: Push a new function named **f**...
- **code n**: ... with n byte codes
	- See step 4
- **environment**: Push a new environment and assign it to the function on the stack
- **store [0]**: Store ID and value reference of closure 0 (**x**) into the environment
- **store [1]**: Store ID and value reference of closure 1 (**y**) into the environment
- **pop**: Pop the stack
- **const_local [2]**: Initialize local 2 (**f**) with the stack value as a `const`
- **pop**: Pop the stack


#### Stack Slots

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|x|-| Closure |Value reference ➔ 0|
|-|y|-| Closure |Value reference ➔ 1|
|-|f|const| Reference |Instance reference ➔ 2|

#### Heap Slots

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|-|-|-| Integer |0|
|1|-|-|-| Integer |1|
|2|➔ 3|-|-|Instance|-|
|3|➔ 4|-| internal |Code|Byte code pointer + Environment reference ➔ 6 |
|4|➔ 5|length|-|Integer|1|
|5|-|name|-|String|"f"|
|6|➔ 7|-|-|Instance|-|
|7|➔ 8|-| internal |Environment|-|
|8|➔ 9|x|-|Closure |Value reference ➔ 0|
|9|-|y|-|Closure |Value reference ➔ 1|

### Step 4: Function execution

Let us call `f(2)`. 

#### Byte Codes

- **undefined**: Push `undefined` as `this`
- **get_local [2]**: Push the value of local 2 (**f**)
- **call**: Push slots to hold the target, result, frame and argument count
- **integer 2**: Push integer `2` as first argument
- **run 1**: Set the argument count to 1 and store the byte code pointer and scope into the frame

> XS jumps to the first byte code of the called function

- **begin_strict 1**:
- **reserve 3**: Reserve 3 slots on the stack
- **[0,1] retrieve**: Initialize closures 0 and 1 with IDs and values from the environment (**x** and **y**)
- **[2] new_local z**: Create local 2 with ID **z** and uninitialized value.

> Again indices **[0, 1, 2]** are implicit, **retrieve** move the scope for each property of the environment.

- **argument 0**: Push the value of the first argument if any, or `undefined`
- **var_local [2]**: Initialize local 2 (**z**) with the stack value as a `var`
- **pop**: Pop the stack
- **get_closure [0]**: Push the value of closure 0 (**x**)
- **get_local [2]**: Push the value of local 2 (**z**)
- **add**: Apply the binary operator
- **pull_closure [0]**: Pull the stack value into closure 0 (**x**)
- **get_closure [1]**: Push the value of closure 1 (**y**)
- **get_local [2]**: Push the value of local 2 (**z**)
- **add**: Apply the binary operator
- **pull_closure [1]**: Pull the stack value into closure 1 (**y**)
- **end**: Retrieve the byte code pointer and scope from the frame, reset the frame, scope and stack then push the result

> XS jumps to the next byte code of the calling function

#### Heap Slots

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|-|-|-| Integer |2|
|1|-|-|-| Integer |3|
|2|➔ 3|-|-|Instance|-|
|3|➔ 4|-| internal |Code|Byte code pointer + Environment reference ➔ 6 |
|4|➔ 5|length|-|Integer|1|
|5|-|name|-|String|"f"|
|6|➔ 7|-|-|Instance|-|
|7|➔ 8|-| internal |Environment|-|
|8|➔ 9|x|-|Closure |Value reference ➔ 0|
|9|-|y|-|Closure |Value reference ➔ 1|

<a id="example2"></a>	
## Example 2

Here is a typical example of closure.

#### Source code
```js
let counter = 0;
const decrement = function() {
  counter--;
}
const increment = function() {
  counter++;
}
```
#### Byte Codes

- **reserve 3**: Reserve 3 slots on the stack
- **[0] new_closure counter**: Create closure 0 with ID **counter** and uninitialized value
- **[1] new_local decrement**: Create local 1 with ID **decrement** and uninitialized value
- **[2] new_local increment**: Create local 2 with ID **increment** and uninitialized value
- **integer 0**: Push integer `0`
- **let_closure [0]**: Initialize closure 0 (**counter**) with the stack value as a `let`
- **pop**: Pop the stack
- **function decrement**: Push a new function named **decrement**...
- **code n**: ... with n byte codes
	- **begin_strict 0**:
	- **reserve 2**: Reserve 2 slots on the stack
	- **[0] retrieve**: Initialize closure 0 with ID and value from the environment (**counter**)
	- **get_closure [0]**: Push the value of closure 0 (**counter**)
	- **decrement**: Apply the postfix operator
	- **pull_closure [0]**: Pull the stack value into closure 0 (**counter**)
	- **end**:
- **environment**: Push a new environment and assign it to the function on the stack
- **store [0]**: Store ID and value reference of closure 0 (**counter**) into the environment
- **pop**: Pop the stack
- **const_local [1]**:: Initialize local 1 (**decrement**) with the stack value as a `const`
- **pop**: Pop the stack
- **function increment**: Push a new function named **increment**...
- **code n**: ... with n byte codes
	- **begin_strict 0**:
	- **reserve 2**: Reserve 2 slots on the stack
	- **[0] retrieve**: Initialize closure 0 with ID and value from the environment (**counter**)
	- **get_closure [0]**: Push the value of closure 0 (**counter**)
	- **increment**: Apply the postfix operator
	- **pull_closure [0]**: Pull the stack value into closure 0 (**counter**)
	- **end**:
- **environment**: Push a new environment and assign it to the function on the stack
- **store [0]**: Store ID and value reference of closure 0 (**counter**) into the environment
- **pop**: Pop the stack
- **const_local [2]**: Initialize local 2 (**increment**) with the stack value as a `const`
- **pop**: Pop the stack

#### Stack Slots

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|counter|-| Closure |Value reference ➔ 0|
|-|decrement|const| Reference | Instance reference ➔ 1 |
|-|increment|const| Reference | Instance reference ➔ 8 |

#### Heap Slots

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|-|-|-|Number |0|
|1|➔ 2|-|-|Instance|-|
|2|➔ 3|-| internal |Code|Byte code pointer + Environment reference ➔ 5 |
|3|➔ 4|length|-|Integer|0|
|4|-|name|-|String|"decrement"|
|5|➔ 6|-|-|Instance|-|
|6|➔ 7|-| internal |Environment|-|
|7|-|counter|-|Closure |Value reference ➔ 0|
|8|➔ 9|-|-|Instance|-|
|9|➔ 10|-| internal |Code|Byte code pointer + Environment reference ➔ 12 |
|10|➔ 11|length|-|Integer|0|
|11|-|name|-|String|"increment"|
|12|➔ 13|-|-|Instance|-|
|13|➔ 14|-| internal |Environment|-|
|14|-|counter|-|Closure |Value reference ➔ 0|

Both `decrement` and `increment` functions have environments with closures that reference the same value.

<a id="example3"></a>	
## Example 3

Here is the example in the document quoted here above.

#### Source code

```js
function f(x) {
	const xx = x**2;
	return function(y) {
		const yy = y**2;
		return (xx + yy)**0.5;
	}
}
```
#### Byte codes

- **function f**: Push a new function named **f**...
- **code n**: ...with n bytes of byte code
	- **begin_strict 1**:
	- **reserve 2**: Reserve 2 slots on the stack for locals and closures
	- **[0] new_local x**: Create  local 0 with ID **x** and uninitialized value
	- **argument 0**: Push the value of the first argument if any, else `undefined`
	- **var_local [0]**: Initialize local 0 (**x**) with the stack value as a `var` 
	- **pop**: Pop the stack
	- **[1] new_closure xx**: Create closure 1 with ID **xx** and uninitialized value
	- **get_local [0]**: Push the value of local 0 (**x**)
	- **integer 2**: Push integer `2`
	- **exponentiation**: Apply the binary operator
	- **const_closure [1]**: Initialize the closure 1 (**xx**) with the stack value as a `const`
	- **pop**: Pop the stack
	- **function ?**: Push a new anonymous function...
	- **code n**: ...with n bytes of byte code
		- **begin_strict 1**:
		- **reserve 3**: Reserve 3 slots on the stack for locals and closures
		- **[0] retrieve**: Initialize closure  0 with ID and value from the environment (**xx**)
		- **[1] new_local y**: Create local 1 with ID **y** and uninitialized value
		- **argument 0**: Push the value of the first argument if any, or `undefined`
		- **var_local [1]**: Initialize local 1 (**y**) with the stack value as a `var` 
		- **pop**: Pop the stack
		- **[2] new_local yy**: Create a new local 2 with ID **yy** and uninitialized value
		- **get_local [1]**: Push the value of local 1 (**y**)
		- **integer 2**: Push integer `2`
		- **exponentiation**: Apply the binary operator
		- **const_local [2]**: Initialize local 2 (**yy**) with the stack value as a `const`
		- **pop**: Pop the stack
		- **get_closure [0]**: Push the value of closure 0 (**xx**) 
		- **get_local [2]**: Push the value of local 2 (**yy**)
		- **add**: Apply the binary operator
		- **number 0.5**: Push number `0.5` on the stack
		- **exponentiation**: Apply the binary operator
		- **set_result**: Pull the stack value into into the result
		- **end**:
	- **environment**: Push a new environment and assign it to the function on the stack
	- **store [1]**: Store ID and value of closure 1 (**xx**) into the environment
	- **pop**: Pop the stack
	- **set_result**: Pull the stack value into into the result 
	- **end**:


#### Heap Slots

Here are slots created by defining function `f`.

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|➔ 1|-|-|Instance|-|
|1|➔ 2|-| internal |Code|Byte code pointer|
|2|➔ 3|length|-|Integer|1|
|3|-|name|-|String|"f"|

Here are slots created by calling `f(3)`

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|4|-|-|-|Number |9|
|5|➔ 6|-|-|Instance|-|
|6|➔ 7|-| internal |Code|Byte code pointer + Environment reference ➔ 10 |
|7|➔ 8|length|-|Integer|1|
|8|-|name|-|String|""|
|9|➔ 10|-|-|Instance|-|
|10|➔ 11|-| internal |Environment|-|
|11|-|xx|-|Closure |Value reference ➔ 4|

<a id="example4"></a>	
## Example 4

Generators, like async function and async generators, do not change the way XS handles scopes at runtime.  

When suspending and resuming the execution of a generator, XS stores and retrieves relevant stack slots with code and scope offsets into and from the generator instance.

Closures in such stack slots will be alive as long as the generator instance itself is alive.

#### Source code
```js
let index = 0;
const infinite = function*() {
	for (;;) {
		yield index;
		index += 1;
	}
}
const generator = infinite();
generator.next();
```

### Step 1

Create the `index` closure and the `infinite` generator function.

#### Byte codes

- **reserve 3**: Reserve 3 slots on the stack for locals and closures
- **[0] new_closure index**: Create closure 0 with ID **index** and uninitialized value
- **[1] new_local infinite**: Create local 1 with ID **infinite** and uninitialized value
- **[2] new_local generator**: Create local 2 with ID **generator** and uninitialized value
- **integer 0**: Push integer `0`
- **let_closure [0]**: Initialize closure 0 (**index**) with the stack value as a `let`
- **pop**: Pop the stack
- **generator infinite**: Push a new generator function named **infinite**...
- **code n**: ... with n byte codes
	- See steps 2 and 3
- **environment**: Push a new environment and assign it to the function on the stack
- **store [0]**: Store ID and value reference of closure 0 (**index**) into the environment
- **pop**: Pop the stack
- **const_local [1]**:: Initialize local 1 (**infinite**) with the stack value as a `const`
- **pop**: Pop the stack

#### Stack Slots

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-|index|-| Closure |Value reference ➔ 0|
|-|infinite|const| Reference |Instance reference ➔ 1|
|-|generator|-| Uninitialized |-|

#### Heap Slots

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|-|-|-| Integer |0|
|1|➔ 2|-|-|Instance|-|
|2|➔ 3|-|internal|Code|Byte code pointer + Environment reference ➔ 5|
|3|➔ 4|length|-|Integer|0|
|4|-|name|-|String|"infinite"|
|5|➔ 6|-|-|Instance|-|
|6|➔ 7|-| internal |Environment|-|
|7|-|index|-|Closure |Value reference ➔ 0|

### Step 2

Call the `infinite` generator function to create the generator instance.

#### Byte codes

- **undefined**: Push `undefined`
- **get_local [1]**: Push the value of local 1 (**infinite**)
- **call**: Push slots to hold the target, result, frame and argument count
- **run 0**: Set the argument count to 0 and store the byte code pointer and scope into the frame

> XS jumps to the first byte code of the called generator function

- **begin_strict 0**:
- **reserve 1**: Reserve 1 slot on the stack for the closure
- **[0] retrieve**: Initialize closure 0  with ID and value from the environment (**index**)
- **start_generator**: Set the result to a new generator instance and exit like **end**

**start_generator** also stores stack slots with code and scope offsets into the generator instance.

> XS jumps to the next byte code of the calling function

- **const_local [2]**: Initialize local 2 (**generator**) with the stack value as a `const`
- **pop**: Pop the stack

#### Stack Slots

|Next|ID|Flag|Kind |Value|
|---:|---|---|:---|:---|
|-| index |-| Closure |Value reference ➔ 0|
|-|infinite|const| Reference |Instance reference ➔ 1|
|-|generator|const| Reference |Instance reference ➔ 8|

#### Heap Slots

|#|Next|ID|Flag|Kind|Value|
|---:|---:|---|---|:---|:---|
|0|-|-|-| Integer |0|
|1|➔ 2|-|-|Instance|-|
|2|➔ 3|-|internal|Code|Byte code pointer + Environment reference ➔ 5|
|3|➔ 4|length|-|Integer|0|
|4|-|name|-|String|"infinite"|
|5|➔ 6|-|-|Instance|-|
|6|➔ 7|-| internal |Environment|-|
|7|-| index |-|Closure |Value reference ➔ 0|
|8|➔ 9|-|-|Instance|-|
|9|-|-|internal|Stack|Chunk pointer |

The last slot contains a chunk pointer. The chunk contains stack slots plus code and scope offsets.

#### Heap Chunk

|Next|ID|Flag|Kind |Value||
|---:|---|---|:---|:---|:---|
|-|-|-|Undefined|-|*this*
|-|-|-|Reference|Instance reference|*function*
|-|-|-|Undefined|-|*target*|
|-|-|-|Undefined|-|*result*|
|-|-|-|Frame|Code offset + Scope offset |
|-|index|-| Closure |Value reference ➔ 0|

### Step 3

Call the `next` method of the generator instance

#### Byte codes

- **get_local [2]**: Push the value of local 2 (**generator**)
- **dub**: Re-push the stack value
- **get_property next**: Get property with ID **next** and replace the stack value with its value
- **call**: Push slots to hold the target, result, frame and argument count
- **run 0**: Set the argument count to 0 and store the byte code pointer and scope into the frame

Internally, the `next` method retrieves stack slots with code and scope offsets from the generator instance. So the stack is the same as after **start_generator**

> XS jumps to the next byte code of the called generator function

- **object**: Push a new ordinary object
- **dub**: Re-push the stack value 
- **get_closure [0]**:
- **new_property value**: Create a new property with ID `value` and pull the stack value into the property
- **dub**: Re-push the stack value 
- **false**: Push boolean `false`
- **new_property done**: Create a new property with ID `done` and pull the stack value into the property
- **yield**:  Pull the stack value into the result and exit like **end**

**yield** also stores stack slots with code and scope offsets into the generator instance.

> XS jumps to the next byte code of the calling function

#### Stack Slots

No changes

#### Heap Slots

No changes

#### Heap Chunk

Only the code offset changed.
