<!--
 | Copyright (c) 2016-2023  Moddable Tech, Inc.
 |
 |   This file is part of the Moddable SDK Runtime.
 | 
 |   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 |   it under the terms of the GNU Lesser General Public License as published by
 |   the Free Software Foundation, either version 3 of the License, or
 |   (at your option) any later version.
 | 
 |   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 |   but WITHOUT ANY WARRANTY; without even the implied warranty of
 |   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |   GNU Lesser General Public License for more details.
 | 
 |   You should have received a copy of the GNU Lesser General Public License
 |   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 |
 | This file incorporates work covered by the following copyright and  
 | permission notice:  
 |
 |       Copyright (C) 2010-2016 Marvell International Ltd.
 |       Copyright (C) 2002-2010 Kinoma, Inc.
 |
 |       Licensed under the Apache License, Version 2.0 (the "License");
 |       you may not use this file except in compliance with the License.
 |       You may obtain a copy of the License at
 |
 |        http://www.apache.org/licenses/LICENSE-2.0
 |
 |       Unless required by applicable law or agreed to in writing, software
 |       distributed under the License is distributed on an "AS IS" BASIS,
 |       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 |       See the License for the specific language governing permissions and
 |       limitations under the License.
-->

# XS in C
Revised: February 9, 2023

**See end of document for [copyright and license](#license)**

## About This Document

This document describes XS in C, the C interface to the runtime of the XS JavaScript engine. Information on building XS for target software/hardware platforms is provided in the companion document [XS Platforms.md](./XS%20Platforms.md). 

In accordance with the ECMAScript specifications, the XS runtime implements only generic features that all scripts can use. An application defines the specific features that its own scripts can use through C callbacks. An application that uses the XS runtime is a host in ECMAScript terminology. 

XS in C provides macros to access properties of objects. XS provides two functionally equivalent variations of many of the macros. The macros prefixed with `xs` alone are somewhat more convenient to work with but generate larger binary code whereas the macros prefixed with `xsmc` generate smaller binary code at the expense of being more difficult to use. Including the `xsmc.h` header file makes the `xsmc` versions of some operations available.

## Table of Contents

* [Slots](#slots): Describes how to handle ECMAScript constructs in C callbacks, with examples that show the correspondences between ECMAScript and XS in C.
	* [Slot types](#slot-types) 
	* [Primitives](#primitives)
	* [ArrayBuffer](#arraybuffer)
	* [Instances and Prototypes](#instances-and-prototypes)
	* [Identifiers](#identifiers)
	* [Properties](#properties)
	* [Arguments and Variables](#xsvars)
	* [Garbage Collector](#garbage-collector)
	* [Exceptions](#exceptions)
	* [Errors](#errors)
	* [Debugger](#debugger)
* [Machine](#machine): Introduces the main structure of the XS runtime (its virtual machine) and explains how to use the runtime to build a host and to make C callbacks available to scripts. This section concludes with an example that demonstrates how to use XS in C to implement a JavaScript class with C functions.
	* [Machine Allocation](#machine-allocation)
	* [Context](#context)
	* [Host](#host)
	* [JavaScript `@` language syntax extension](#syntax-extension)
* [Glossary](#glossary): Includes all the terms defined or referenced in this document.
* [License](#license)

<a id="slots"></a>
## Slots

In the XS runtime, the *slot* is a fundamental storage unit. A slot is an opaque structure that is manipulated only through XS in C.

	typedef struct xsSlotRecord xsSlot
	struct xsSlotRecord {
		void* data[4];
	};

<a id="slot-types"></a>
### Slot types

There are eleven types of slots:

	enum {
		xsUndefinedType,
		xsNullType,
		xsBooleanType,
		xsIntegerType,
		xsNumberType,
		xsStringType,
		xsStringXType,
		xsSymbolType,
		xsBigIntType,
		xsBigIntXType,
		xsReferenceType 
	}
	typedef char xsType;
 
The undefined, null, boolean, number, string, symbol, and bigint slots correspond to the ECMAScript primitive types. The reference slot corresponds to the ECMAScript `reference` type. The integer, stringx and bigintx slots are optimizations that are unobservable by scripts. The integer slot is equivalent to the number slot, but allowing floating point operations to be bypassed. The stringx slot is equivalent to the string slot, but uses a string in place (e.g. in ROM) without making a copy. The bigintx slot is equivalent to the bigint slot, but uses a bigint in place (e.g. in ROM) without making a copy.

##### In ECMAScript:

```javascript
undefined
null
false
true
0
0.0
"foo"
```

##### In C:

```
xsUndefined;
xsNull;
xsFalse;
xsTrue;
xsInteger(0);
xsNumber(0.0);
xsString("foo");
```

The `xsTypeOf` macro returns the type of a slot. It is similar to the ECMAScript `typeof` keyword.

**`xsType xsTypeOf(xsSlot theSlot)`**<BR>
**`xsType xsmcTypeOf(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot`|  The slot to test

Returns the type of the slot

> **Note**: The macros in the XS in C API require a reference to the target virtual machine in a variable in the current scope with the name `the` of type `xsMachine *`.

##### In ECMAScript:

```javascript
switch(typeof arguments[0]) {
	case "undefined": break;
	/* Null is an object. */
	case "boolean": break;	
	/* Integers are numbers */	
	case "number": break;
	/* StringX is a string */
	case "string": break;
	case "symbol": break;
	/* BigIntX is a bigint */
	case "bigint": break;
	case "object": break;
	case "function": break;
}	
```

##### In C:

```
switch(xsTypeOf(xsArg(0))) {
	case xsUndefinedType: break;
	case xsNullType: break;
	case xsBooleanType: break;
	case xsIntegerType: break;
	case xsNumberType: break;
	case xsStringType: break;
	case xsSymbolType: break;
	case xsBigIntType: break;
	case xsReferenceType: break;  /* Objects and functions are references. */
}		
```

<a id="primitives"></a>
### Primitives

The undefined, null, boolean, integer, number, string, and symbol slots (collectively known as *direct slots*) correspond to the ECMAScript primitive types, with the integer and stringx slots added as optimizations.

#### Undefined and null

The undefined and null slots contain no value. The `xsUndefined` and `xsNull` macros return slots of those types.

**`xsSlot xsUndefined`**

Returns an undefined slot

***

**`void xsmcSetUndefined(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set to `undefined`

Sets the specified slot value to `undefined`

***

**`xsSlot xsNull`**

Returns a null slot

***

**`void xsmcSetNull(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set to `null`

Sets the specified slot value to `null`

***

#### Booleans, integers, and numbers

These slots contain values of the corresponding type.

	typedef char xsBooleanValue;  
	typedef long xsIntegerValue;  
	typedef double xsNumberValue;  
	
The following macros return slots of each of these types (set to a particular value) or access/set the value in a slot. When accessing the value in a slot, you specify a desired type; the slot is coerced to the requested type if necessary, and the value is returned.

**`xsSlot xsTrue`**

Returns a boolean slot containing `true`

***

**`xsSlot xsFalse`**

Returns a boolean slot containing `false`

***

**`xsSlot xsBoolean(xsBooleanValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theValue` | The value to be contained in the slot

Returns a boolean slot

***

**`xsBooleanValue xsToBoolean(xsSlot theSlot)`**<BR>
**`xsBooleanValue xsmcToBoolean(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to coerce to boolean

Returns the value contained in the slot

***

**`void xsmcSetFalse(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set false

Sets the slot value to `false`

***

**`void xsmcSetTrue(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set true

Sets the slot value to `true`

***

**`void xsmcSetBoolean(xsSlot theSlot, xsBooleanValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set
| `theValue` | The boolean `true` or `false` value to set

Sets the slot value to `true` or `false`

***

**`xsSlot xsInteger(xsIntegerValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theValue` | The value to be contained in the slot

Returns an integer slot

***

**`xsIntegerValue xsToInteger(xsSlot theSlot)`**<BR>
**`xsIntegerValue xsmcToInteger(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to coerce to integer

Returns the value contained in the slot

***

**`void xsmcSetInteger(xsSlot theSlot, xsIntegerValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set
| `theValue` | The integer value to set

Sets the slot value to an integer

***

**`xsSlot xsNumber(xsNumberValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theValue` | The value to be contained in the slot

Returns a number slot

***

**`xsNumberValue xsToNumber(xsSlot theSlot)`**<BR>
**`xsNumberValue xsmcToNumber(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to coerce to number

Returns the value contained in the slot

***

**`void xsmcSetNumber(xsSlot theSlot, xsNumberValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set
| `theValue` | The number value to set

Sets the slot value to a number

***

#### Strings

These slots contain values of the corresponding type.

	typedef char* xsStringValue;
	
A string value is a pointer to a UTF-8 C string. The XS runtime virtual machine and the garbage collector manage UTF-8 C strings used by scripts.

**`xsSlot xsString(xsStringValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theValue` | The value to be contained in the slot

Returns a string slot

C constants, C globals, or C locals can safely be passed to the `xsString` macro, since it duplicates its parameter. 

***

**`xsStringValue xsToString(xsSlot theSlot)`**<BR>
**`xsStringValue xsmcToString(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to coerce to string

Returns the string contained in the slot

For speed, the `xsToString` macro returns the value contained in the slot itself, a pointer to the string in the memory managed by XS. Since the XS runtime can compact memory containing string values, the result of the `xsToString` macro cannot be used across or in other macros of XS in C. The ECMAScript language specification forbids modifying the string in place.

***

**`void xsmcSetString(xsSlot theSlot, xsStringValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set
| `theValue` | The string value to set

Sets the slot value to a string

***

**`xsStringValue xsToStringBuffer(xsSlot theSlot, xsStringValue theBuffer, xsIntegerValue theSize)`**<BR>
**`xsStringValue xsmcToStringBuffer(xsSlot theSlot, xsStringValue theBuffer, xsIntegerValue theSize)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to coerce to string
| `theBuffer` | A buffer to copy the string into
| `theSize` | The size of the buffer

Copies the string value and returns the buffer containing the copy of the string. The buffer provided has to be large enough to hold a copy of the string value. 

***

**`xsSlot xsStringBuffer(void *theData, xsIntegerValue theSize)`**

| Arguments | Description |
| --- | :-- |
| `theData` | A pointer to the data to copy into the string buffer, or `NULL` to leave the string buffer data uninitialized
| `theSize` | The data size to copy in bytes

Copies the string into an allocated buffer, sets a slot value to the string buffer, and returns a reference to the new string buffer instance.

***

**`void xsmcSetStringBuffer(xsSlot theSlot, xsStringValue theValue, xsIntegerValue theSize)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to set
| `theValue` | The string value to set
| `theSize` | The size of the string in bytes

Copies the string into an allocated buffer and sets the slot value to the string buffer.

***

<a id="arraybuffer"></a>
### ArrayBuffer

In ECMAScript an `ArrayBuffer` is commonly used to store fixed length binary data. 

#### Macros

**`xsSlot xsArrayBuffer(void *theData, xsIntegerValue theSize)`**
**`void xsmcSetArrayBuffer(xsSlot theSlot, void *theData, xsIntegerValue theSize)`**

 
| Arguments | Description |
| --- | :-- |
| `theData` | A pointer to the data to copy into the `ArrayBuffer`, or `NULL` to leave the `ArrayBuffer` data uninitialized
| `theSize` | The size of the data in bytes

Creates an `ArrayBuffer` instance and returns a reference to the new `ArrayBuffer` instance 

***

**`void xsGetArrayBufferData(xsSlot theSlot, xsIntegerValue theOffset, void *theData, xsIntegerValue theSize)`**<BR>
**`void xsmcGetArrayBufferData(xsSlot theSlot, xsIntegerValue theOffset, void *theData, xsIntegerValue theSize)`**
 
| Arguments | Description |
| --- | :-- |
| `theSlot` | The `ArrayBuffer` slot
| `theOffset` | The starting byte offset to get the data
| `theData` | The data pointer to get the `ArrayBuffer` data
| `theSize` | The data size to copy in bytes

Copies bytes from the `ArrayBuffer`

***

**`void xsSetArrayBufferData(xsSlot theSlot, xsIntegerValue theOffset, void *theData, xsIntegerValue theSize)`**<BR>
**`void xsmcSetArrayBufferData(xsSlot theSlot, xsIntegerValue theOffset, void *theData, xsIntegerValue theSize)`**
 
| Arguments | Description |
| --- | :-- |
| `theSlot` | The `ArrayBuffer` slot
| `theOffset` | The starting byte offset to set the data
| `theData` | The data pointer to set the `ArrayBuffer` data
| `theSize` | The data size to copy in bytes

Copies bytes into the `ArrayBuffer`

***

**`void xsmcSetArrayBuffer(xsSlot theSlot, void *theData, xsIntegerValue theSize)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The `ArrayBuffer` slot
| `theData` | The data pointer to set the `ArrayBuffer` data
| `theSize` | The data size to copy in bytes

Creates an `ArrayBuffer` instance initialized from the provided data

***

**`xsIntegerValue xsGetArrayBufferLength(xsSlot theSlot)`**<BR>
**`xsIntegerValue xsmcGetArrayBufferLength(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The `ArrayBuffer` slot

Returns the size of the `ArrayBuffer` in bytes

***

**`void xsSetArrayBufferLength(xsSlot theSlot, xsIntegerValue theSize)`**
 
| Arguments | Description |
| --- | :-- |
| `theSlot` | The `ArrayBuffer` slot
| `theSize` | The size of the `ArrayBuffer` data in bytes. If the size of the buffer is increased, the new data is initialized to 0.

Sets the length of the `ArrayBuffer`

***

**`void *xsToArrayBuffer(xsSlot theSlot)`**<BR>
**`void *xsmcToArrayBuffer(xsSlot theSlot)`**
 
| Arguments | Description |
| --- | :-- |
| `theSlot` | The `ArrayBuffer` slot

Returns a pointer to the `ArrayBuffer` data

For speed, the `xsToArrayBuffer ` macro returns the value contained in the slot itself, a pointer to the buffer in the memory managed by XS. Since the XS runtime can compact memory containing string values, the result of the `xsToArrayBuffer ` macro cannot be used across or in other macros of XS in C. 

***

<a id="instances-and-prototypes"></a>
### Instances and Prototypes

In XS in C, as in ECMAScript, an object can inherit properties from another object, which can inherit from another object, and so on; the inheriting object is the *instance*, and the object from which it inherits is the *prototype*.

Reference slots (type `xsReferenceType`) are *indirect* slots: they contain a reference to an instance of an object, function, array, and so on. Instances themselves are made of slots that are the properties of the instance (or, for an array, the items of the instance).

#### Macros

**`xsSlot xsObjectPrototype`**<BR>
**`xsSlot xsFunctionPrototype`**<BR>
**`xsSlot xsArrayPrototype`**<BR>
**`xsSlot xsStringPrototype`**<BR>
**`xsSlot xsBooleanPrototype`**<BR>
**`xsSlot xsNumberPrototype`**<BR>
**`xsSlot xsDatePrototype`**<BR>
**`xsSlot xsRegExpPrototype`**<BR>
**`xsSlot xsHostPrototype`**<BR>
**`xsSlot xsErrorPrototype`**<BR>
**`xsSlot xsEvalErrorPrototype`**<BR>
**`xsSlot xsRangeErrorPrototype`**<BR>
**`xsSlot xsReferenceErrorPrototype`**<BR>
**`xsSlot xsSyntaxErrorPrototype`**<BR>
**`xsSlot xsTypeErrorPrototype`**<BR>
**`xsSlot xsURIErrorPrototype`**<BR>
**`xsSlot xsSymbolPrototype`**<BR>
**`xsSlot xsArrayBufferPrototype`**<BR>
**`xsSlot xsDataViewPrototype`**<BR>
**`xsSlot xsTypedArrayPrototype`**<BR>
**`xsSlot xsMapPrototype`**<BR>
**`xsSlot xsSetPrototype`**<BR>
**`xsSlot xsWeakMapPrototype`**<BR>
**`xsSlot xsWeakSetPrototype`**<BR>
**`xsSlot xsPromisePrototype`**<BR>
**`xsSlot xsProxyPrototype`**<BR>

Returns a reference to the prototype instance created by the XS runtime.

***

**`xsSlot xsNewArray(xsIntegerValue theLength)`**<BR>
**`xsSlot xsmcNewArray(xsIntegerValue theLength)`**

| Arguments | Description |
| --- | :-- |
| `theLength` | The array length property to set

Creates an array instance, and returns a reference to the new array instance 

##### In ECMAScript:

```javascript
new Array(5);
```

##### In C:

```
xsNewArray(5);
```

***

**`xsSlot xsNewObject()`**<BR>
**`xsSlot xsmcNewObject()`**

Creates an object instance, and returns a reference to the new object instance 

##### In ECMAScript:

```javascript
new Object();
```

##### In C:

```
xsNewObject();
```

***

**`void xsmcSetNewObject(theSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The result slot 

The `xsmcSetNewObject` macro is functionally equivalent to the `xsNewObject` macro. The property is returned in the slot provided.

##### In ECMAScript:

```javascript
new Object();
```

##### In C:

```
xsVars(1);
xsmcNewObject(xsVar(0));
```

***

**`xsBooleanValue xsIsInstanceOf(xsSlot theInstance, xsSlot thePrototype)`**<BR>
**`xsBooleanValue xsmcIsInstanceOf(xsSlot theInstance, xsSlot thePrototype)`**

| Arguments | Description |
| --- | :-- |
| `theInstance` | A reference to the instance to test
| `thePrototype` | A reference to the prototype to test

Tests whether an instance has a particular prototype, directly or indirectly (that is, one or more levels up in the prototype hierarchy). Returns `true` if the instance has the prototype, `false` otherwise.

The `xsIsInstanceOf` macro has no equivalent in ECMAScript; scripts test instances through *constructors* rather than directly through prototypes. A constructor is a function that has a `prototype` property that is used to test instances with `isPrototypeOf`.

##### In ECMAScript:

```javascript
if (Object.prototype.isPrototypeOf(this))
	return new Object();
```

##### In C:

```
if (xsIsInstanceOf(xsThis, xsObjectPrototype))
	xsResult = xsNewObject();
```

***

<a id="identifiers"></a>
### Keys, Identifiers and Indexes

In ECMAScript, the properties of an object are identified by number, string or symbol values – a.k.a. the property **key**. In XS in C you can access properties with property keys through the `xsGetAt`, `xsSetAt`, etc macros described below. 

If the number or string value of a property key can be converted into a 32-bit unsigned integer, XS uses the result of the conversion – a.k.a. the property **index** — to identify the property. In XS in C, you can access properties directy with property indexes through the `xsGetIndex`, `xsSetIndex`, etc macros described below. A property index can be used with all instances but is typically used to access items of `Array` instances.

```
typedef uint32_t xsIndex;
```

Otherwise the string or symbol value of a property key is stored into a table and XS uses the resulting table index – a.k.a. the property **identifier** — to identify the property. In XS in C, you can access properties directy with property identifiers through the `xsGet`, `xsSet` etc macros described below.

```
typedef uint16_t xsIdentifier;
```

On 64-bit platforms, the number of available identifiers can be extended to 32-bit and `xsIdentifier` becomes `uint32_t`.

#### Macros

**`xsIdentifier xsID(xsStringValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theValue` | The string to convert

Converts a string value into an identifier and returns the identifier. For a given virtual machine, the same string value is always converted to the same identifier, so you can cache frequently used identifiers by virtual machine.

For performance, XS in C also supports accessing properties by identifiers generated by the XS compiler, e.g. `xsID_property`. The type of the `xsID_*` identifiers is `xsIdentifier`. These identifiers provide an optimization that can be used by the `xsGet`, `xsSet`, etc macros described below. the `xsID_*` properties are functionally equivalent to using the `xsID()` macro and used in all the examples.

In the C examples below, the `xsGet` macro (discussed in the next section) takes as its second argument the identifier of the property or item to get.

##### In ECMAScript:

```javascript
this.foo
```

##### In C:

```
xsGet(xsThis, xsID_foo);
```

***

**`xsBooleanValue xsIsID(xsStringValue theValue)`**

| Arguments | Description |
| --- | :-- |
| `theValue` | The string to test

Tests whether a given string corresponds to an existing property identifier. Returns `true` if the string is a property identifier, `false` otherwise.

***

<a id="properties"></a>
### Properties 

This section describes the macros related to accessing properties of objects (or items of arrays), as summarized in Table 1.

**Table 1.** Property-Related Macros

<table class="normalTable">
  <tbody>
    <tr>
      <th scope="col">Macro</th>
      <th scope="col">Description</th>
    </tr>
    <tr>
      <td><code>xsGlobal</code></td>
      <td>Returns a special instance made of global properties available to scripts</td>
    </tr> 
    <tr>
      <td><code>xsDefine</code></td>
      <td>Defines a new property of an instance with an identifier and attributes</td>
    </tr> 
      <td><code>xsDefineAt</code></td>
      <td>Defines a new property or item of an instance with a key and attributes</td>
    </tr> 
    <tr>
      <td><code>xsHas, xsmcHas</code></td>
      <td>Tests whether an instance has a property corresponding to a particular identifier</td>
    </tr> 
    <tr>
      <td><code>xsHasAt</code></td>
      <td>Tests whether an instance has a property corresponding to a particular key</td>
    </tr> 
     <tr>
      <td><code>xsHasIndex, xsmcHasIndex</code></td>
      <td>Tests whether an instance has a property corresponding to a particular index</td>
    </tr> 
   <tr>
      <td><code>xsGet, xsmcGet</code></td>
      <td>Gets a property of an instance by identifier</td>
    </tr> 
    <tr>
      <td><code>xsGetAt, xsmcGetAt</code></td>
      <td>Gets a property or an instance by key</td>
    </tr> 
    <tr>
      <td><code>xsGetIndex, xsmcGetIndex</code></td>
      <td>Gets a property or an instance by index</td>
    </tr> 
	<tr>
      <td><code>xsSet, xsmcSet</code></td>
      <td>Sets a property of an instance with an identifier</td>
    </tr> 
	<tr>
      <td><code>xsSetAt, xsmcSetAt</code></td>
      <td>Sets a property of an instance with a key </td>
	</tr> 
 	<tr>
      <td><code>xsSetIndex, xsmcSetIndex</code></td>
      <td>Sets an property of an instance with an index</td>
    </tr> 
   <tr>
      <td><code>xsDelete, xsmcDelete</code></td>
      <td>Deletes a property corresponding to a particular identifier </td>
    </tr> 
    <tr>
      <td><code>xsDeleteAt, xsmcDeleteAt</code></td>
      <td>Deletes a property corresponding to a particular key</td>
    </tr> 
    <tr>
      <td><code>xsCall0</code> ... <code>xsCall7, xsmcCall</code></td>
      <td>Calls the function referred to by a property of an instance</td>
    </tr> 
    <tr>
      <td><code>xsNew0</code> ... <code>xsNew7, xsmcNew</code></td>
      <td>Calls the constructor referred to by a property of an instance</td>
    </tr>     
    <tr>
      <td><code>xsTest, xsmcTest</code></td>
      <td>Takes a value of any type and determines whether it is true or false</td>
    </tr>     
    <tr>
      <td><code>xsEnumerate</code></td>
      <td>Enumerates the properties of an instance</td>
    </tr>     
  </tbody>
</table>


> Some of the examples below use variable slots reserved on the stack with the `xsVars` macro; See [`Arguments and Variables`](#xsvars).

#### xsGlobal

Globals available to scripts are properties of a special instance referred to using the `xsGlobal` macro in XS in C.

**`xsSlot xsGlobal`**

Returns a reference to the special instance made of globals

You can use the `xsGet`, `xsSet`, `xsDelete`, `xsCall*`, and `xsNew*` macros with the `xsGlobal` macro as the first parameter. Examples are shown in the sections describing those macros.

***

#### xsDefine

To define a new property of an instance with an identifier and attributes, use the `xsDefine` macro. The attributes of the property are set using one or more of the following constants. 

	enum {
		xsDefault = 0,
		xsDontDelete = 2,
		xsDontEnum = 4,
		xsDontSet = 8,
		xsStatic = 16,
		xsIsGetter = 32,
		xsIsSetter = 64,
		xsChangeAll = 30
	} 
	typedef unsigned char xsAttribute;


**`void xsDefine(xsSlot theThis, xsIdentifier theID, xsSlot theParam, xsAttribute theAttributes)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that will have the property
| `theID` | The identifier of the property to define
| `theParam` | The value of the property to define
| `theAttributes` | A combination of attributes to set.

For `theAttributes`, specify the constants corresponding to the attributes you want to set (the others being cleared).

The `xsDontDelete`, `xsDontEnum`, and `xsDontSet` attributes correspond to the ECMAScript `configurable`, `enumerable`, and `writable` attributes. By default a property can be deleted, enumerated, and set.

When a property is created, if the prototype of the instance has a property with the same name, its attributes are inherited; otherwise, by default, a property can be deleted, enumerated, and set, and can be used by scripts.

##### In ECMAScript:
	
```javascript
Object.defineProperty(this, "foo", 7, { writable: true, enumerable: true, configurable: true });
```

##### In C:

```
xsDefine(xsThis, xsID_foo, xsInteger(7), xsDefault);
```

***

#### xsDefineAt

To define a new property or item of an instance with a key and attributes, use the `xsDefineAt` macro. The `xsDefineAt` macro is functionally equivalent to the `xsDefine` macro, except that a slot is used to identify the property or item to define.

**`void xsDefineAt(xsSlot theThis, xsSlot theSlot, xsSlot theParam, xsAttribute theAttributes)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that will have the property
| `theKey` | The key of the property to define
| `theParam` | The value of the property to define
| `theAttributes` | A combination of attributes to set.

##### In ECMAScript:

```javascript
Object.defineProperty(this, "foo", 7, { writable: true, enumerable: true, configurable: true });
Object.defineProperty(this, 5, 7, { writable: true, enumerable: true, configurable: true });
```

##### In C:

```
xsDefineAt(xsThis, xsString("foo"), xsInteger(7), xsDefault);
xsDefineAt(xsThis, xsInteger(5), xsInteger(7), xsDefault);
```
***

#### xsHas

To test whether an instance has a property corresponding to a particular identifier, use the `xsHas` macro. This macro is similar to the ECMAScript `in` keyword.

**`xsBooleanValue xsHas(xsSlot theThis, xsIdentifier theID)`**<BR>
**`xsBooleanValue xsmcHas(xsSlot theThis, xsIdentifier theID)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance to test
| `theID` | The identifier of the property to test

Returns `true` if the instance has the property, `false` otherwise

##### In ECMAScript:

```javascript
if ("foo" in this)
```

##### In C:

```
if (xsHas(xsThis, xsID_foo));
```

***

#### xsHasAt

To test whether an instance has a property corresponding to a particular key, use the `xsHasAt` macro.

**`xsBooleanValue xsmcHasAt(xsSlot theThis, xsSlot theKey)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance to test
| `theKey` | The key of the property to test

Returns `true` if the instance has the property, `false` otherwise


##### In ECMAScript:

```javascript
if ("foo" in this)
if (5 in this)
```

##### In C:

```
if (xsHasAt(xsThis, xsString("foo"))
if (xsHasAt(xsThis, xsInteger(5))
```

***

#### xsHasIndex

To test whether an instance has a property corresponding to a particular index, use the `xsHasIndex` macro. This macro is similar to the ECMAScript `in` keyword.

**`xsBooleanValue xsHasIndex(xsSlot theThis, xsIndex theIndex)`**<BR>
**`xsBooleanValue xsmcHasIndex(xsSlot theThis, xsIndex theIndex)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance to test
| `theIndex` | The index of the property to test

Returns `true` if the instance has the property, `false` otherwise

##### In ECMAScript:

```javascript
if (7 in this)
```

##### In C:

```
if (xsHasIndex(xsThis, 7));
```

***

#### xsGet

To get a property of an instance by identifer, use the `xsGet` macro.

**`xsSlot xsGet(xsSlot theThis, xsIdentifier theID)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that has the property
| `theID` | The identifier of the property to get

Returns a slot containing what is contained in the property, or `xsUndefined` if the property is not defined by the instance or its prototypes

##### In ECMAScript:

```javascript
foo
this.foo
```

##### In C:

```
xsGet(xsGlobal, xsID_foo);
xsGet(xsThis, xsID_foo);
```

***


#### xsmcGet

The `xsmcGet` macro is functionally equivalent to the `xsGet` macro. The property is returned in the slot provided.

**`void xsmcGet(xsSlot theSlot, xsSlot theThis, xsIdentifier theID)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to contain the property or item
| `theThis` | A reference to the instance that has the property
| `theID` | The identifier of the property to get

##### In ECMAScript:

```javascript
foo
this.foo
```

##### In C:

```
xsVars(1);
xsmcGet(xsVar(0), xsGlobal, xsID_foo);
xsmcGet(xsVar(0), xsThis, xsID_foo);
```

***

#### xsGetAt

To get a property of an instance by key, use the `xsGetAt` macro.

**`xsSlot xsGetAt(xsSlot theThis, xsSlot theKey)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that has the property
| `theKey` | The key of the property to get

Returns a slot containing what is contained in the property, or `xsUndefined` if the property is not defined by the instance or its prototypes

##### In ECMAScript:

```javascript
this["foo"]
this[5]
```

##### In C:

```
xsGetAt(xsThis, xsString("foo"));
xsGetAt(xsVar(0), xsInteger(5));
```

***

#### xsmcGetAt

The `xsmcGetAt` macro is functionally equivalent to the `xsGetAt` macro. The property is returned in the slot provided.

**`void xsmcGetAt(xsSlot theSlot, xsSlot theThis, xsSlot theKey)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to contain the property
| `theThis` | A reference to the instance that has the property
| `theKey` | The key of the property to get

##### In ECMAScript:

```javascript
this["foo"]
this[5]
```

##### In C:

```
xsVars(1);
xsmcGetAt(xsVar(0), xsThis, xsString("foo"));
xsmcGetAt(xsVar(0), xsThis, xsInteger(5));
```
***

#### xsGetIndex

To get a property of an instance by index, use the `xsGetIndex` macro.

**`xsSlot xsGetIndex(xsSlot theThis, xsIndex theIndex)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that has the property
| `theIndex` | The index of the property to get

Returns a slot containing what is contained in the property, or `xsUndefined` if the item is not defined by the instance or its prototypes

##### In ECMAScript:

```javascript
this[0]
```

##### In C:

```
xsGetIndex(xsThis, 0);
```

***

#### xsmcGetIndex

The `xsmcGetIndex` macro is functionally equivalent to the `xsGetIndex` macro. The property is returned in the slot provided.

**`void xsmcGetIndex(xsSlot theSlot, xsSlot theThis, xsIndex theIndex)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to contain the property
| `theThis` | A reference to the instance that has the item
| `theIndex` | The index of the item to get

##### In ECMAScript:

```javascript
this[0]
```

##### In C:

```
xsVars(1);
xsmcGetIndex(xsVar(0), xsThis, 0);
```

***

#### xsSet

To set a property of an instance with an identifier, use the `xsSet` macro. If the property is not defined by the instance, this macro inserts it into the instance.

**`void xsSet(xsSlot theThis, xsIdentifier theID, xsSlot theParam)`**<BR>
**`void xsmcSet(xsSlot theThis, xsIdentifier theID, xsSlot theParam)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that will have the property
| `theID` | The identifier of the property to set
| `theParam` | The value of the property to set

##### In ECMAScript:

```javascript
foo = 0
this.foo = 1
```

##### In C:

```
xsSet(xsGlobal, xsID_foo, xsInteger(0));
xsSet(xsThis, xsID_foo, xsInteger(1));
```

***


#### xsSetAt

To set a property of an instance with a key, use the `xsSetAt` macro. If the property or item is not defined by the instance, this macro inserts it into the instance.

**`void xsSetAt(xsSlot theThis, xsSlot theKey, xsSlot theValue)`**<BR>
**`void xsmcSetAt(xsSlot theThis, xsSlot theKey, xsSlot theValue)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that will have the property
| `theKey` | The key of the property to set
| `theValue` | The value of the property to set

##### In ECMAScript:

```javascript
this["foo"] = 0
this[3] = 1
```

##### In C:

```
xsSetAt(xsThis, xsString("foo"), xsInteger(0));
xsSetAt(xsThis, xsInteger(3), xsInteger(1));
```

***

#### xsSetIndex

To set a property of an instance with an index, use the `xsSetIndex` macro. If the property is not defined by the instance, this macro inserts it into the instance.

**`void xsSetIndex(xsSlot theThis, xsIndex theIndex, xsSlot theParam)`**<BR>
**`void xsmcSetIndex(xsSlot theThis, xsIndex theIndex, xsSlot theParam)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that will have the property
| `theIndex` | The index of the property to set
| `theParam` | The value of the property to set

##### In ECMAScript:

```javascript
this[3] = 1
```

##### In C:

```
xsSetIndex(xsThis, 3, xsInteger(1));
```

***

#### xsDelete

To delete a property of an instance corresponding to a particular identifier, use the `xsDelete` macro. If the property is not defined by the instance, this macro has no effect.

**`void xsDelete(xsSlot theThis, xsIdentifier theID)`**<BR>
**`void xsmcDelete(xsSlot theThis, xsIdentifier theID)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that has the property 
| `theID` | The identifier of the property to delete

##### In ECMAScript:

```javascript
delete foo
delete this.foo
```

##### In C:

```
xsDelete(xsGlobal, xsID_foo);
xsDelete(xsThis, xsID_foo);
```

***

#### xsDeleteAt, xsmcDeleteAt

To delete a property of an instance corresponding to a particular key, use the `xsDeleteAt` macro. If the property is not defined by the instance, this macro has no effect.

**`void xsDeleteAt(xsSlot theThis, xsSlot theKey)`**<BR>
**`void xsmcDeleteAt(xsSlot theThis, xsSlot theKey)`**

| Arguments | Description |
| --- | :-- |
| `theThis ` | A reference to the instance that has the property
| `theKey` | The key of the property to delete

##### In ECMAScript:

```javascript
delete this["foo"]
delete this[3]
```

##### In C:

	xsDeleteAt(xsThis, xsString("foo"));
	xsDeleteAt(xsThis, xsInteger(3));

***

#### xsCall*

When a property or item of an instance is a reference to a function, you can call the function with one of the `xsCall*` macros (where `*` is `0` through `7`, representing the number of parameter slots passed). If the property or item is not defined by the instance or its prototypes or is not a reference to a function, the `xsCall*` macro throws an exception.

**`xsSlot xsCall0(xsSlot theThis, xsIdentifier theID)`**<BR>
**`xsSlot xsCall1(xsSlot theThis, xsIdentifier theID, xsSlot theParam0)`**<BR>
**`xsSlot xsCall2(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1)`**<BR>
**`xsSlot xsCall3(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2)`**<BR>
**`xsSlot xsCall4(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3)`**<BR>
**`xsSlot xsCall5(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4)`**<BR>
**`xsSlot xsCall6(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5)`**<BR>
**`xsSlot xsCall7(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5, xsSlot theParam6)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that will have the property or item
| `theID` | The identifier of the property or item to call
| `theParam0` ... `theParam6` | The parameter slots to pass to the function

Returns the result slot of the function

##### In ECMAScript:

```javascript
foo()
this.foo(1)
this[0](2, 3)
```

##### In C:

	xsCall0(xsGlobal, xsID_foo);
	xsCall1(xsThis, xsID_foo, xsInteger(1));
	xsCall2(xsThis, 0, xsInteger(2), xsInteger(3));

#### xsmcCall

The `xsmcCall` macro is functionally equivalent to the `xsCall*` macros. The result and parameter slots are provided as function parameters.

**`void xsmcCall(xsSlot xsSlot, xsSlot theThis, xsIdentifier theID, ...)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The result slot 
| `theThis` | A reference to the instance that will have the property or item 
| `theID` | The identifier of the property or item to call 
| ... | The variable length parameter slots to pass to the function

##### In ECMAScript:

```javascript
foo(1)
this.foo(1)
this[0](2, 3)
```

##### In C:

	xsVars(3);
	xsmcSetInteger(xsVar(0), 1);
	xsmcSetInteger(xsVar(1), 2);
	xsmcSetInteger(xsVar(2), 3);
	xsmcCall(xsResult, xsGlobal, xsID_foo, xsVar(0));
	xsmcCall(xsResult, xsThis, xsID_foo, xsVar(0));
	xsmcCall(xsResult, xsThis, 0, xsVar(1), xsVar(2));
	
	
***

<a id="xsnew"></a>
#### xsNew*

When a property or item of an instance is a reference to a constructor, you can call the constructor with one of the `xsNew*` macros (where `*` is `0` through `7`, representing the number of parameter slots passed). If the property or item is not defined by the instance or its prototypes or is not a reference to a constructor, the `xsNew*` macro throws an exception.

**`xsSlot xsNew0(xsSlot theThis, xsIdentifier theID)`**<BR>
**`xsSlot xsNew1(xsSlot theThis, xsIdentifier theID, xsSlot theParam0)`**<BR>
**`xsSlot xsNew2(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1)`**<BR>
**`xsSlot xsNew3(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2)`**<BR>
**`xsSlot xsNew4(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3)`**<BR>
**`xsSlot xsNew5(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4)`**<BR>
**`xsSlot xsNew6(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5)`**<BR>
**`xsSlot xsNew7(xsSlot theThis, xsIdentifier theID, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5, xsSlot theParam6)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to the instance that has the property or item 
| `theID` | The identifier of the property or item to call 
| `theParam0` ... `theParam6` | The parameter slots to pass to the constructor

Returns the result slot of the constructor


##### In ECMAScript:

```javascript
new foo()
new this.foo(1)
new this[0](2, 3)
```

##### In C:

	xsNew0(xsGlobal, xsID_foo);
	xsNew1(xsThis, xsID_foo, xsInteger(1));
	xsNew2(xsThis, 0, xsInteger(2), xsInteger(3));

***

#### xsmcNew

The `xsmcNew` macro is functionally equivalent to the `xsNew*` macros. The result and parameter slots are provided as function parameters.

**`void xsmcNew(xsSlot theSlot, xsSlot theThis, xsIdentifier theID, ...)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The result slot of the constructor 
| `theThis` | A reference to the instance that has the property or item 
| `theID` | The identifier of the property or item to call 
| ... | The variable length parameter slots to pass to the function


##### In ECMAScript:

```javascript
new foo(1)
new this.foo(1)
new this[0](2, 3)
```

##### In C:

	xsVars(3);
	xsmcSetInteger(xsVar(0), 1);
	xsmcSetInteger(xsVar(1), 2);
	xsmcSetInteger(xsVar(2), 3);
	xsmcNew(xsResult, xsGlobal, xsID_foo, xsVar(0));
	xsmcNew(xsResult, xsThis, xsID_foo, xsVar(0));
	xsmcNew(xsResult, xsThis, 0, xsVar(1), xsVar(2));

***

#### xsTest

Like an `if` clause in ECMAScript, the `xsTest` macro takes a value of any type and determines whether it is true or false. This macro applies the same rules as in ECMAScript (per the ECMA-262 specification, section 12.5).

**`xsBooleanValue xsTest(xsSlot theValue)`**<BR>
**`xsBooleanValue xsmcTest(xsSlot theValue)`**

| Arguments | Description |
| --- | :-- |
| `theValue` | The value to test

Returns `true` if the value is true, `false` otherwise


##### In ECMAScript:

```javascript
if (foo) {}
```

##### In C:

	if (xsTest(xsGet(xsGlobal, xsID_foo)) {}
	
***

#### xsEnumerate

Use the `xsEnumerate` macro to get an iterator for enumerable instance properties. The iterator provides `next`, `value` and `done` functions to iterate the properties. 

**`xsSlot xsEnumerate(xsSlot theObject)`**

| Arguments | Description |
| --- | :-- |
| `theObject` | A reference to the object that has enumerable properties

Returns a slot containing the iterator

##### In ECMAScript:

```javascript
rectangle = { x:0, y:0, width:200, height:100 };
for (let prop in rectangle)
	trace(`${prop}: ${rectangle[prop]}\n`);
```

##### In C:

	xsVars(5);
	xsVar(0) = xsGet(xsGlobal, xsID_rectangle);
	xsVar(1) = xsEnumerate(xsVar(0));
	for (;;) {
		xsVar(2) = xsCall0(xsVar(1), xsID("next"));
		if (xsTest(xsGet(xsVar(2), xsID("done"))))
			break;
		xsVar(3) = xsGet(xsVar(2), xsID("value"));
		xsVar(4) = xsGetAt(xsVar(0), xsVar(3));
		xsTrace(xsToString(xsVar(3)));xsTrace(": ");
		xsTrace(xsToString(xsVar(4)));xsTrace("\n");
	}

***

<a id="xsvars"></a>
### Arguments and Variables

The XS runtime virtual machine uses a heap and a stack of slots. With XS in C, you can access stack slots directly and heap slots indirectly, through references.

When a C callback is executed, the stack contains its argument slots, its `this` slot, and its result slot, but no variable slots. To use variable slots, you have to reserve them on the stack with the `xsVars` or `xsmcVars` macros. The `xsVars` macro can only be used once at the beginning of the callback execution. The `xsmcVars` macro can be used multiple times within a callback. Using `xsmcVars`, the callback can use a different number of variables in different branches of the code, to reduce the XS stack size.

**`void xsVars(xsIntegerValue theCount)`**<BR>
**`void xsmcVars(xsIntegerValue theCount)`**

| Arguments | Description |
| --- | :-- |
| `theCount` | The number of variable slots to reserve

***

Argument and variable slots are accessed and assigned by index. An exception is thrown if the index is invalid.

Initially:

- The argument slots are the parameter slots passed to the function or constructor.

- If the callback is a function, the `this` slot refers to the instance being called and the result slot is undefined.

- If the callback is a constructor, the `this` and result slots refer to the instance being created.

- The variable slots are undefined.

Scripts can call a constructor as a function or a function as a constructor. To find out whether the C callback is executed as a constructor or as a function, you can check whether the result slot is initially undefined.

***

**`xsSlot xsArgc`<BR>
`int xsmcArgc`**

Returns an integer slot that contains the number of arguments

***

**`xsSlot xsArg(xsIntegerValue theIndex)`**

| Arguments | Description |
| --- | :-- |
| `theIndex` | The index of the argument, from 0 to `xsArgc-1`

Returns the argument slot

***

**`xsSlot xsThis`**

Returns the `this` slot

***

**`xsSlot xsResult`**

Returns the result slot

***

**`xsSlot xsVarc`**

Returns an integer slot that contains the number of variables

***

**`xsSlot xsVar(xsIntegerValue theIndex)`**

| Arguments | Description |
| --- | :-- |
| `theIndex` | The index of the variable, from 0 to `xsVarc-1`

Returns the variable slot

***

#### Example

Usually you access the argument, `this`, result, and variable slots but you assign only the result and variable slots. Whatever is in the result slot at the end of the callback execution is returned to scripts by the function or constructor.

In the C example in this section (and the next one), `xsMachine` is the virtual machine structure, as shown in the section [Machine](#machine).

##### In ECMAScript:

```javascript
function foo() {
	var c, i, s;
	c = arguments.length;
	s = "";
	for (i = 0; i < c; i++)
		s = s.concat(arguments[i]);
	return s;
}
```

##### In C:

	void xs_foo(xsMachine* the) {
		xsIntegerValue c, i;
		xsVars(1);
		c = xsToInteger(xsArgc));
		xsVar(0) = xsString("");
		for (i = 0; i < c; i++)
			xsVar(0) = xsCall1(xsVar(0), xsID_concat, xsArg(i));
		xsResult = xsVar(0);
	}

<a id="garbage-collector"></a>
### Garbage Collector

When the XS runtime needs to allocate slots and there is not enough memory, it automatically deletes unused slots. The runtime garbage collector uses a mark and sweep algorithm.  To force the runtime to delete unused  slots, you can use the `xsCollectGarbage` macro.

**`void xsCollectGarbage()`**

***

If you store slots in memory that is no managed by the garbage collector, such as a C global or a C allocated structure, use the `xsRemember` and `xsForget` macros to inform the runtime.

**`void xsRemember(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to remember


**`void xsForget(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to forget

`xsRemember` links and `xsForget` unlinks a slot to and from a chain of slots which the garbage collector scans to mark the slots that the C global or the C allocated structure references.

***

Use `xsAccess` to get the value of a slot previously linked to the chain of slots.

**`xsSlot xsAccess(xsSlot theSlot)`**

| Arguments | Description |
| --- | :-- |
| `theSlot` | The slot to access

Returns the value of the slot

##### In C:

	xsSlot gFooSlot;
	void xsSetupFoo(xsMachine* the) {
		gFooSlot = xsThis;
		xsRemember(gFooSlot);
	}
	void xsInvokeFoo(xsMachine* the) {
		xsVars(2);
		xsVar(0) = xsAccess(gFooSlot);
		xsVar(1) = xsString("message");
		xsCall1(xsVar(0), xsID_invoke, xsVar(1));
	}
	void xsCleanupFoo(xsMachine* the) {
		xsForget(gFooSlot);
	}

***

The garbage collector is enabled by default. Use `xsEnableGarbageCollection` to enable or disable the garbage collector.
 
**`xsSlot xsEnableGarbageCollection(xsBooleanValue enable)`**

| Arguments | Description |
| --- | :-- |
| `enable` | Set `true` to enable garbage collection or `false` to disable garbage collection

***

<a id="exceptions"></a>
### Exceptions

To handle exceptions in C, the XS runtime uses `setjmp`, `longjmp`, and a chain of `jmp_buf` buffers, defined as follows:

	typedef struct xsJumpRecord xsJump
	struct xsJumpRecord {
		jmp_buf buffer;
		xsJump* nextJump;
		xsSlot* stack;
		xsSlot* frame;
	};

However, you do not need to use this directly, because XS in C defines macros for throwing and catching exceptions.

To throw an exception, use the `xsThrow` macro.

**`void xsThrow(xsSlot theException)`**

| Arguments | Description |
| --- | :-- |
| `theException` | The exception slot

Assigns the current exception

***

**`xsSlot xsException`**

Accesses the current exception and returns the exception slot

***

As shown in the following example, the `xsTry` and `xsCatch` macros are used together to catch exceptions. If you catch an exception in your C callback and you want to propagate the exception to the script that calls your function or constructor, throw the exception again.

##### In ECMAScript:

```javascript
{
	try {
		/* Exception thrown here ... */
	}
	catch(e) {
		/* ... is caught here. */	
		throw e
 	}
}
```

##### In C:

	 {
		xsTry {
			/* Exception thrown here ... */
		}
		xsCatch {
			/* ... is caught here. */
			xsThrow(xsException)
		}
	} 

<a id="errors"></a>
### Errors

Exceptions may be thrown by C callbacks. C callbacks are often provide the interface between scripts and systems. Many system calls can fail, and they have a way to return an error to the application which can be propagated as an exception.

For specific errors, the XS runtime provides error types and prototypes.

	enum {
		XS_NO_ERROR = 0,
		XS_UNKNOWN_ERROR,
		XS_EVAL_ERROR,
		XS_RANGE_ERROR,
		XS_REFERENCE_ERROR,
		XS_SYNTAX_ERROR,
		XS_TYPE_ERROR,
		XS_URI_ERROR,
		XS_ERROR_COUNT
	};
	
XS in C defines the following macros to throw specific exceptions.

**`void xsUnknownError(...)`<BR>
`void xsEvalError(...)`<BR>
`void xsRangeError(...)`<BR>
`void xsReferenceError(...)`<BR>
`void xsSyntaxError(...)`<BR>
`void xsTypeError(...)`<BR>
`void xsURIError(...)`**

| Arguments | Description |
| --- | :-- |
| ... | The message and optional arguments to display when throwing the exception

##### In C:

	xpt2046 xpt = calloc(1, sizeof(xpt2046Record));
	if (!xpt) xsUnknownError("out of memory");
	
	if (strlen(string) > MAXNAMESIZE)
		xsRangeError("name too long: %s", string);

	char *slash = strrchr(path, '/');
	if (!slash)
		xsURIError("No path");

***

The `xsErrorPrintf` macro is a shortcut for `xsUnknownError` when only a message parameter is required.

**`xsErrorPrintf(xsStringValue theMessage)`<BR>
`xsUnknownError("%s", theMessage)`**

| Arguments | Description |
| --- | :-- |
| theMessage | The message to display when throwing the exception

##### In C:

	if (rotation != requestedRotation)
		xsErrorPrintf("not configured for requested rotation");

***

<a id="debugger"></a>
### Debugger

XS in C provides two macros to help you debug your C callbacks.

The `xsDebugger` macro is equivalent to the ECMAScript `debugger` keyword.

**`void xsDebugger()`**

***

The `xsTrace` macro is equivalent to the global `trace` function.

**`void xsTrace(xsStringValue theMessage)`**

| Arguments | Description |
| --- | :-- |
| `theMessage` | The message to log in the debugger

##### In ECMAScript:

```javascript
debugger;
trace("Hello xsbug!\n");
```

##### In C:

```
xsDebugger();
xsTrace("Hello xsbug!\n");
```

<a id="machine"></a>
## Machine

The main structure of the XS runtime is its virtual machine, which is what parses, compiles, links, and executes scripts. A virtual machine is an opaque structure though some members of the structure are available to optimize the macros of XS in C; you never need to use them directly.

```
typedef struct xsMachineRecord xsMachine
struct xsMachineRecord {
	xsSlot* stack;
	xsSlot* scope;
	xsSlot* frame;
	xsByte* code;
	xsSlot* stackBottom;
	xsSlot* stackTop;
	xsSlot* stackPrototypes;
	xsJump* firstJump;
};
```

A single machine does not support multiple threads. To work with multiple threads, create one XS runtime machine for each thread, with the host optionally providing a way for the machines to communicate. 

<a id="machine-allocation"></a>
### Machine Allocation

To use the XS runtime you have to create a machine with the `xsCreateMachine` macro, allocating memory for it as required. Its parameters are:

-  A structure with members that are essentially parameters specifying what to allocate for the machine. Pass `NULL` if you want to use the defaults. 

```
typedef struct {
	xsIntegerValue initialChunkSize;
	xsIntegerValue incrementalChunkSize;
	xsIntegerValue initialHeapCount;
	xsIntegerValue incrementalHeapCount;
	xsIntegerValue stackCount;
	xsIntegerValue initialKeyCount;
	xsIntegerValue incrementalKeyCount;
	xsIntegerValue nameModulo;
	xsIntegerValue symbolModulo;
	xsIntegerValue parserBufferSize;
	xsIntegerValue parserTableModulo;
	xsIntegerValue staticSize;
} xsCreation;
```

- The name of the machine

- A context you can set and get in your callbacks (as discussed in the next section). Pass `NULL` if you do not want an initial context.

**`xsMachine* xsCreateMachine(xsCreation* theCreation, xsStringValue theName, void* theContext)`**

| Arguments | Description |
| --- | :-- |
| `theCreation` | The parameters of the machine
| `theName` | The name of the machine as a string
| `theContext` | The initial context of the machine, or `NULL`

Returns a machine if successful, otherwise `NULL`




Regarding the parameters of the machine that are specified in the `xsCreation` structure:

- A machine uses chunks to store strings, bytecodes, array buffers, big int values, and others. The `initialChunkSize` is the initial size of the memory allocated to chunks. The `incrementalChunkSize` tells the runtime how to expand the memory allocated to chunks. 

- A machine uses a heap and a stack of slots. The `initialHeapCount` is the initial number of slots allocated to the heap. The `incrementalHeapCount` tells the runtime how to increase the number of slots allocated to the heap. The `stackCount` is the number of slots allocated to the stack. Note that these values are all slots, not bytes.

- A symbol binds a string value and an identifier; see [`xsID`](#xs-id). The `initialKeyCount` is the number of symbols the machine will allocate at initialization. When the keys are exhausted `incrementalKeyCount` keys are added; if `incrementalKeyCount` is 0, the VM aborts when the keys are exhausted. `symbolModulo` is the size of the hash table the machine will use for symbols.  The `nameModulo` is the size of the hash table the machine will use for symbol names. 

- Some XS hosts attempt to grow the slot and chunk heaps without limit at runtime to accommodate the memory needs of the hosted scripts; others limit the maximum memory that may be allocated to the machine. For the latter, the `staticSize` defines the total number of bytes that may be allocated for the combination of chunks and slots, which includes the stack. In general, only hosts running on resource constrained devices implement `staticSize`.

***

When you are done with a machine, you free it with the `xsDeleteMachine` macro. The destructors of all the host objects are executed, and all the memory allocated by the machine is freed.

**`void xsDeleteMachine(xsMachine* the)`**

| Arguments | Description |
| --- | :-- |
| `the` | A machine

The `xsDeleteMachine` macro is one of a number of macros described in this document that have an explicit machine parameter named `the`, for which the value returned by `xsCreateMachine` is passed. (The other such macros are `xsGetContext`, `xsSetContext`, `xsBeginHost`, and `xsEndHost`.) Only those macros have an explicit `the` parameter because they are the only ones that can be used outside a callback and cannot throw exceptions. Callbacks must name their machine parameter `the` because all other macros have an implicit parameter named `the`; the primary reason for this convention is terseness, but it also emphasizes the fact that these other macros can be used only inside a callback and can throw exceptions.

#### Example

The following example illustrates the use of `xsCreateMachine` and `xsDeleteMachine`. The `xsMainContext` function called in the example is defined in the next section.

```
int main(int argc, char* argv[]) 
{
	xsCreation aCreation = {
		128 * 1024 * 1024,	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		4 * 1024 * 1024, 	/* initialHeapCount */
		1 * 1024 * 1024,	/* incrementalHeapCount */
		1024,			/* stack count */
		2048+1024,		/* key count */
		1993,			/* name modulo */
		127			/* symbol modulo */
	};
	xsMachine* aMachine;

	aMachine = xsCreateMachine(&aCreation, "machine", NULL);
	if (aMachine) {
		xsMainContext(aMachine, argc, argv);
		xsDeleteMachine(aMachine);
	}
	else
		fprintf(stderr, "### Cannot allocate machine\n");
	return 0;
}
```

<a id="context"></a>
### Context

The machine will call your C code primarily through callbacks. In your callbacks, you can set and get a *context*: a pointer to an area where you can store and retrieve information for the machine.

**`void xsSetContext(xsMachine* the, void* theContext)`**

| Arguments | Description |
| --- | :-- |
| `the` | A machine
| `theContext` | A context

Sets a context

***

**`void* xsGetContext(xsMachine* the)`**

| Arguments | Description |
| --- | :-- |
| `the` | A machine

Returns a context

***

#### Example 
The following code shows a context being set in the `xsMainContext` function, which was called in the preceding section's example.

```
typedef struct {
	int argc;
	char** argv;
} xsContext;

void xsMainContext(xsMachine* theMachine, int argc, char* argv[])
{
	xsContext* aContext;

	aContext = malloc(sizeof(xsContext));
	if (aContext) {
		aContext->argc = argc;
		aContext->argv = argv;
		xsSetContext(theMachine, aContext);
		xsSetContext(theMachine, NULL);
		free(aContext);
	}
	else
		fprintf(stderr, "### Cannot allocate context\n");
}
```

<a id="host"></a>
### Host

This section describes the host-related macros of XS in C (see Table 2). An annotated example that uses the host-related macros follows.

**Table 2.** Host-Related Macros

<table class="normalTable">
  <tbody>
    <tr>
      <th scope="col">Macro</th>
      <th scope="col">Description</th>
    </tr>
    <tr>
      <td>
        <p><code>xsNewHostFunction</code></p>
        <p><code>xsNewHostConstructor</code></p>
      </td>
      <td>Creates a host function or host constructor</td>
    </tr> 
    <tr>
      <td><code>xsNewHostObject</code></td>
      <td>Creates a host object</td>
    </tr> 
    <tr>
      <td><code>xsNewHostInstance, xsmcNewHostInstance</code></td>
      <td>Creates a host object instance</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsGetHostData, xsmcGetHostData</code></p>
        <p><code>xsSetHostData, xsmcSetHostData</code></p>
      </td>
      <td>Gets or sets the data in a host object</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsGetHostChunk, xsmcGetHostChunk</code></p>
        <p><code>xsSetHostChunk, xsmcSetHostChunk</code></p>
      </td>
      <td>Gets or sets the data as a chunk in a host object</td>
    </tr> 
    <tr>
      <td><code>xsSetHostDestructor</code></td>
      <td>Sets the destructor for a host object</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsBeginHost</code></p>
        <p><code>xsEndHost</code></p>
      </td>
      <td>Used together to set up and clean up a stack frame, so that you can use all the macros of XS in C in between</td>
    </tr> 
  </tbody>
</table>

#### xsNewHostFunction and xsNewHostConstructor

A *host function* is a special kind of function, one whose implementation is in C rather than ECMAScript. For a script, a host function is just like a function; however, when a script invokes a host function, a C callback is executed. The same is true for *host constructors*, which are constructors implemented in C.

```
typedef void (*xsCallback)(xsMachine* the);
```

To create a host function, use the `xsNewHostFunction` macro.

**`xsSlot xsNewHostFunction(xsCallback theCallback, xsIntegerValue theLength)`**

| Arguments | Description |
| --- | :-- |
| `theCallback` | The callback to execute
| `theLength` | The number of parameters expected by the callback

Creates a host function, and returns a reference to the new host function

***

**`xsSlot xsNewHostConstructor(xsCallback theCallback, xsIntegerValue theLength, xsSlot thePrototype)`**

| Arguments | Description |
| --- | :-- |
| `theCallback` | The callback to execute
| `theLength` | The number of parameters expected by the callback
| `thePrototype` | A reference to the prototype of the instance to create

Creates a host constructor, and returns a reference to the new host constructor

***

#### xsNewHostObject

A *host object* is a special kind of object with data that can be directly accessed only in C. The data in a host object is invisible to scripts. 

When the garbage collector is about to get rid of a host object, it executes the host object's destructor, if any. No reference to the host object is passed to the destructor: a destructor can only destroy data.

```
typedef void (xsDestructor)(void* theData);
```

To create a host object, use the `xsNewHostObject` macro.

**`xsSlot xsNewHostObject(xsDestructor theDestructor)`**

| Arguments | Description |
| --- | :-- |
| `theDestructor` | The destructor to be executed by the garbage collector. Pass the host object's destructor, or `NULL` if it does not need a destructor.

Creates a host object, and returns a reference to the new host object

***

#### xsNewHostInstance

Use the `xsNewHostInstance` macro to create an instance of a host object.

**`xsSlot xsNewHostInstance(xsSlot thePrototype)`**
**`xsSlot xsmcNewHostInstance(xsSlot thePrototype)`**

| Arguments | Description |
| --- | :-- |
| `thePrototype` | A reference to the prototype of the instance to create. This argument must be host object.

Creates a host object instance, and returns a reference to the new host object instance


***

#### xsGetHostData and xsSetHostData

To get and set the data of a host object, use the `xsGetHostData` and `xsSetHostData` macros. Both throw an exception if the `theThis` parameter does not refer to a host object.

**`void* xsGetHostData(xsSlot theThis)`**<BR>
**`void* xsmcGetHostData(xsSlot theThis)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to a host object

Returns the data

***

**`void xsSetHostData(xsSlot theThis, void* theData)`<BR>
`void xsmcSetHostData(xsSlot theThis, void* theData)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to a host object 
| `theData` | The data to set

***

#### xsGetHostChunk and xsSetHostChunk

To get and set the data of a host object as a chunk, use the `xsGetHostChunk` and `xsSetHostChunk` macros. Both throw an exception if the `theThis` parameter does not refer to a host object. Like the memory used by ArrayBuffer and String, chunk memory is allocated and managed by the XS runtime; see the [handle](./handle.md) document for details. 

**`void* xsGetHostChunk(xsSlot theThis)`<BR>
`void* xsmcGetHostChunk(xsSlot theThis)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to a host object

Returns the data

***

**`void xsSetHostChunk(xsSlot theThis, void* theData, xsIntegerValue theSize)`<BR>
`void xsmcSetHostChunk(xsSlot theThis, void* theData, xsIntegerValue theSize)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to a host object
| `theData` | The data to set or `NULL` to leave the chunk data uninitialized 
| `theSize` | The size of the data in bytes

> Note that an object has either host data or a host chunk but never both. 

***

#### xsSetHostDestructor

To set the destructor of a host object (or to clear the destructor, by passing `NULL`), use the `xsSetHostDestructor` macro. This macro throws an exception if the `theThis` parameter does not refer to a host object.

**`void xsSetHostDestructor(xsSlot theThis, xsDestructor theDestructor)`<BR>
`void xsmcSetHostDestructor(xsSlot theThis, xsDestructor theDestructor)`**

| Arguments | Description |
| --- | :-- |
| `theThis` | A reference to a host object 
| `theDestructor` | The destructor to be executed by the garbage collector, or `NULL` to clear the destructor

***

#### xsBeginHost and xsEndHost

Use the `xsBeginHost` macro to establish a new stack frame and the `xsEndHost` macro to remove it. 

**`void xsBeginHost(xsMachine* the)`**<BR>
**`void xsEndHost(xsMachine* the)`**

| Arguments | Description |
| --- | :-- |
| `the` | A machine

The `xsBeginHost` macro sets up the stack, and the `xsEndHost` macro cleans up the stack, so that you can use all the macros of XS in C in the block between `xsBeginHost` and `xsEndHost`.

Uncaught exceptions that occur between the calls the `xsBeginHost` and `xsEndHost `do not propagate beyond `xsEndHost`.

##### Example

This example creates a `File` class using the host macros of XS in C. This is a low-level technique that provides the most flexibility. Most projects do not create classes directly using XS in C, but instead use the [`@` syntax extension](#syntax-extension) to declare classes because it is simpler.

This code uses the `File` class from JavaScript to open and close a file:

```
const f = new File("/Users/user/test.js", "rb");
f.close();
```

The following code builds the `File` class. The XS in C host macro calls appear in the block between `xsBeginHost` and `xsEndHost`. Two variable slots are used to store the `File` host object and constructor. The `File` object includes a single host function, `close`, that has no arguments.

The `prototype` is a host object which includes the native destructor `xs_file_destructor` to be invoked when the object is garbage collected. This prototype is provided to `xsNewHostConstructor` along with the native constructor `xs_file_constructor`.

This example adds the `close` function to the prototype after creating the constructor. It may be added before instead.

This example also adds a getter accessor function for the property `isOpen`. 

```
#define kPrototype (0)
#define kConstructor (1)

xsBeginHost(the);
	xsVars(2);
	xsVar(kPrototype) = xsNewHostObject(xs_file_destructor);
	xsVar(kConstructor) = xsNewHostConstructor(xs_file_constructor, 0, xsVar(kPrototype));
	xsSet(xsGlobal, xsID("File"), xsVar(kConstructor));
	xsDefine(xsVar(kPrototype), xsID("close"),	xsNewHostFunction(xs_file_close, xsDefault));
	xsDefine(xsVar(kPrototype), xsID("isOpen"),	xsNewHostFunction(xs_file_get_isOpen, xsIsGetter));
xsEndHost(the);
```

The `xs_file_constructor` function implements the host constructor. The constructor instantiates an instance of the `File` object prototype, opens the requested file, and stores the associated `xsFileRecord`, containing the stdio `FILE` pointer, as host data.

Note that the implementation of a constructor created by calling `xsNewHostConstructor` is slightly different from one created using the `@` syntax. Specifically, the constructor created by `xsNewHostConstructor` must create the instance whereas XS creates the instance for constructors declared with the `@` syntax. Here the constructor uses `xsNewHostInstance` to create the instance and assign it to the the return value `xsResult`. The prototype passed to `xsNewHostInstance` is taken from the prototype of the constructor, accessed through `xsTarget`. The `xsTarget` value is the XS in C equivalent to the [`new.target`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/new.target) pseudo-property in JavaScript.

```
typedef struct {
	FILE *fd;
} xsFileRecord, *xsFile;

static void xs_file_constructor(xsMachine *the)
{
	xsFileRecord f;
	FILE *fd;
	xsResult = xsGet(xsTarget, xsID("prototype"));
	xsResult = xsNewHostInstance(xsResult);
	fd = fopen(xsToString(xsArg(0)), xsToString(xsArg(1)));
	if (!fd)
		xsUnknownError("can't open");
	f = malloc(sizeof(xsFileRecord));
	if (!f) {
		fclose(fd);
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	}
	f->fd = fd;
	xsSetHostData(xsResult, f);
}
```

The `xs_file_destructor` function implements the host object's destructor. The destructor closes the file and frees the host data:

```
static void xs_file_destructor(void *data)
{
	xsFile f = data;
	if (f) {
		fclose(f->fd);
		free(f);
	}
}
```

> Note: The destructor function is called by XS when the `File` instance is garbage collected.

The `xs_file_close` function closes the file immediately, rather than waiting for the instance to be freed by the garbage collector. The function retrieves the associated `xsFileRecord` record from the object instance host data and calls the host object destructor to close the file.

```
static void xs_file_close(xsMachine *the)
{
	xsFile f = xsGetHostData(xsThis);
	if (!f) return;
	xs_file_destructor(f);
	xsSetHostData(xsThis, NULL);
}
```

The `xs_file_get_isOpen` getter accessor function sets the result to `true` or `false` depending on whether the file is open.

```
static void xs_file_get_isOpen(xsMachine *the)
{
	xsFile f = xsGetHostData(xsThis);
	xsResult = f ? xsTrue : xsFalse;
}
```

***

<a id="syntax-extension"></a>
### JavaScript `@` language syntax extension

XS provides the `@` language syntax extension to implement JavaScript functions in C. The language extension is only recognized by the XS compiler. This section introduces the language extension with a JavaScript class that implements methods with C functions.

```javascript
class Rectangle @ "xs_rectangle_destructor" {
	constructor(...params) @ "xs_rectangle";

	get x() @ "xs_rectangle_get_x";
	set x() @ "xs_rectangle_set_x";
	get y() @ "xs_rectangle_get_y";
	set y() @ "xs_rectangle_set_y";
	get w() @ "xs_rectangle_get_w";
	set w() @ "xs_rectangle_set_w";
	get h() @ "xs_rectangle_get_h";
	set h() @ "xs_rectangle_set_h";

	contains(x, y) @ "xs_rectangle_contains";

	union(r) @ "xs_rectangle_union";
};

export default Rectangle;
```
The `Rectangle` class is completely implemented in C using callbacks specified by `@` functions. For example, the `contains` method is implemented by the `xs_rectangle_contains` C function. The C functions use XS in C macros to access properties, host data, and return results. JavaScript applications import the `Rectangle` class and access the methods.

```javascript
import Rectangle from "rectangle";

let r1 = new Rectangle(0, 0, 200, 100);
let r2 = new Rectangle(20, 40, 300, 50);
let r3 = new Rectangle();
r3.union(r1, r2);
```

The `Rectangle` constructor `xs_rectangle` function stores the parameters in a host chunk. The constructor accepts either a single `Rectangle` instance or the individual `x`, `y`, `w` and `h` values. The function uses the `xsmcArgc` macro to count the function parameters and the `xsmcIsInstanceOf` macro to determine if the first parameter is an object.

```
typedef struct {
	int x;
	int y;
	int w;
	int h;
} xsRectangleRecord, *xsRectangle;

void xs_rectangle(xsMachine *the)
{
	xsRectangleRecord r;
	if (xsmcArgc == 0) {
		r.x = r.y = r.w = r.h = 0;
	}
	else if (xsmcIsInstanceOf(xsArg(0), xsObjectPrototype)) {
		xsRectangle r1 = xsmcGetHostChunk(xsArg(0));
		r = *r1;
	}
	else {
		r.x = xsmcToInteger(xsArg(0));
		r.y = xsmcToInteger(xsArg(1));
		r.w = xsmcToInteger(xsArg(2));
		r.h = xsmcToInteger(xsArg(3));
	}
	xsmcSetHostChunk(xsThis, &r, sizeof(r));
}
```
The destructor function `xs_rectangle_destructor` is called when the object instance is deleted or garbage collected. Any memory or resources allocated by the instance should be freed in the destructor. Because the XS runtime manages host chunk memory, the destructor doesn't need to dispose the chunk.

```
void xs_rectangle_destructor(void *data)
{
}
```
The `Rectangle` class provides getters and setters for class properties.

```javascript
	get x() @ "xs_rectangle_get_x";
	set x() @ "xs_rectangle_set_x";
```

The `get` functions read the corresponding field from the host chunk and return the property to the caller by setting `xsResult`. The `set` functions store the value provided into the host chunk.

```
void xs_rectangle_get_x(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, r->x);
}

void xs_rectangle_set_x(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	r->x = xsmcToInteger(xsArg(0));
}
```

The `union` method returns the union of all the rectangles passed to the function. The `xs_rectangle_union` function uses the `xsmcArgc` macro to count the number or `Rectangle` instances. The union result is stored back into calling instance's host chunk. JavaScript applications read the result rectangle properties using the `get *()` methods.

```
void xs_rectangle_union(xsMachine *the)
{
	xsIntegerValue i, argc;
	xsRectangle r, r0 = xsmcGetHostChunk(xsThis);
	xsRectangleRecord rUnion;
	r = r0;
	for (i = 0; i < argc; ++i) {
		Union(&rUnion, r, xsmcGetHostChunk(xsArg(i)));
		r = &rUnion;
	}
	*r0 = rUnion;
}
```

Standalone functions -- functions that are not part of a class -- can also be implemented in C. The `@` syntax extension is used where the function body normally appears. 

```
	function restart() @ "xs_restart";
```

The value of `xsThis` in the implementation of `xs_restart` matches the receiver, which is `xsGlobal` in the following invocation.

```javascript
	restart();
```

<a id="glossary"></a>
## Glossary

| Term | Definition |
| :--- | :-- |
| constructor | In ECMAScript, a function that has a `prototype` property and that the `new` operator invokes to build an instance. The value of the `prototype` property becomes the prototype of the instances that the constructor builds.
| context | A pointer to an area where you can store and retrieve information for the XS runtime virtual machine in your callbacks.
| direct slot | One of the slot types that correspond to the ECMAScript primitive types (undefined, null, boolean, number, string, and symbol), plus an integer and stringx slot provided as an optimization.
| ECMAScript | An object-oriented, prototype-based language for implementing application logic and control.
| host | In ECMAScript terminology, an application that uses the XS runtime.
| host constructor | In XS, a constructor whose implementation is in C rather than ECMAScript.
| host function | In XS, a function whose implementation is in C rather than ECMAScript.
| host object | In XS, an object with data that can be directly accessed only in C.
| indirect slot | A type of slot that contains a reference to an instance of an object, function, array, and so on; corresponds to the ECMAScript `reference` type.
| instance | An object that inherits properties from another object, which is called its *prototype*.
| property | In ECMAScript, a value accessed by name within an object (in contrast to items accessed by index within an array); in XS in C, a slot accessed by index within an object (just as an item is accessed by index within an array).
| prototype | An object from which another object (called an instance) inherits properties.
| sandbox | An environment that is restricted to prevent untrusted code from harming the device on which the code is running. The sandbox for XS application scripts includes the standard features defined in the ECMAScript specification plus additional features as defined and permitted by the XS modules.
| slot | An opaque structure in which everything in the XS runtime is stored, and which is manipulated only through XS in C.
| XS | A toolkit, consisting of a runtime library and a command-line tool, that is designed for developing standards-based, networked, interactive multimedia applications (GUI-based runtimes) or command-line tools for various devices. See also xsruntime and xsc.
| XS runtime | The runtime library part of XS.
| XS in C | The C interface of the XS runtime.
| xsbug | The XS debugger, used to debug applications, modules, and scripts.
| xsc | The command-line tool part of XS. It compiles JavaScript files into XS binary files containing symbols and bytecodes, which is executed by the XS virtual machine that is contained within the XS runtime.

<!-- TBD:
	- Document xsCall*_noResult, xsmcCall_noResult
	- Document xsNewHostConstructorObject, xsNewHostFunctionObject
	- Document: xsReference
-->

<a id="license"></a>
## License
    Copyright (c) 2016-2021  Moddable Tech, Inc.
 
    This file is part of the Moddable SDK Runtime.
  
    The Moddable SDK Runtime is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
  
    The Moddable SDK Runtime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
  
    You should have received a copy of the GNU Lesser General Public License
    along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.

    This file incorporates work covered by the following copyright and  
    permission notice:  

        Copyright (C) 2010-2016 Marvell International Ltd.
        Copyright (C) 2002-2010 Kinoma, Inc.
 
        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at
 
         http://www.apache.org/licenses/LICENSE-2.0
 
        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.
