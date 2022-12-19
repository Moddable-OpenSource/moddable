# XS Conformance
Copyright 2016-2022 Moddable Tech, Inc.<BR>
Revised: June 1, 2022

## Caveat

#### Realm

XS supports one realm by virtual machine. Tests that expect `$262.createRealm` to return a new realm fail.

#### Internationalization

XS does not implement ECMA-402, the Internationalization API Specification, so the `intl402` tests are skipped.

#### Annex B

No XS hosts are web browsers, so the `annexB` tests are skipped. However XS implements `Date.prototype.getYear`, `Date.prototype.setYear`, `Object.prototype.__defineGetter__`, `Object.prototype.__defineSetter__`, `Object.prototype.__lookupGetter__`, `Object.prototype.__lookupSetter__`, `Object.prototype.__proto__`, `String.prototype.substr`, `escape` and `unescape`, 

## Runtime models

On microcontrollers, XS uses a runtime model based on a virtual machine prepared by the XS compiler and linker. The prepared virtual machine contains the ECMAScript built-ins, along with the classes, functions and objects of preloaded modules. The prepared virtual machine is in ROM, its contents is shared by the tiny virtual machines that XS quickly creates in RAM to run apps.

Such a runtime model introduces no conformance issues in itself since XS can alias shared classes, functions and objects if apps modify them. However, in order to save ROM and RAM, other restrictions have been introduced:

- Host functions, i.e. functions implemented in C, are primitive values like booleans, numbers, strings, etc. They are promoted to `Function` objects when necessary.
- Scripts evaluation is optional. So some platforms do not support `eval`, `new Function`, etc. But all platforms support `JSON.parse`. 
- Optionnally the XS linker can dead strip ECMAScript built-ins that Moddable apps do not use, and remove `length` and `name` properties from functions.

Here the conformance is tested on macOS with a traditional runtime model and without any restrictions. For each case, XS creates a virtual machine, then parses and runs the script. The XS harness, `xst`, uses [LibYAML](http://pyyaml.org/wiki/LibYAML) to load the frontmatter, which contains, among other information, the harness scripts to parse and run before the case script itself.

To build `xst`:

#### Linux

	cd $MODDABLE/xs/makefiles/lin
	make
	
#### macOS

	cd $MODDABLE/xs/makefiles/mac
	make
	
#### Windows

	cd %MODDABLE%\xs\makefiles\win
	build
	
To pass some tests, clone [test262](https://github.com/tc39/test262.git) and change the directory to the `test` directory inside the `test262` directory. Then you can run `xst` with files or directories. For instance:

	cd ~/test262/test
	$MODDABLE/build/bin/mac/release/xst language/block-scope
	$MODDABLE/build/bin/mac/release/xst built-ins/TypedArray*

## Results

After the 6th edition, TC39 adopted a [process](https://tc39.github.io/process-document/) based on [proposals](https://github.com/tc39/proposals). Each proposal has a maturity stage. At stage 4, proposals are finished and will be published in the following edition of the specifications.

The official conformance test suite, [test262](https://github.com/tc39/test262), contains cases for the published specifications, together with cases for proposals at stages 3 and 4, which is great to prepare XS for future editions. The XS harness, `xst` uses adhoc comparisons of the frontmatter `[features]` to skip cases related to not yet implemented proposals. See the skipped cases at the end of this document.

Currently, on macOS, XS passes **99.64%** of the language tests and **87.13%** of the built-ins tests. Mostly because of `Temporal`, XS skips **12.75%** of the built-ins tests.

Details are here under. The numbers of skipped cases are between parentheses. The following section lists the failed tests with some explanations.

### Language

     42595/42746 (118) language
         460/460 arguments-object
             43/43 mapped
             8/8 unmapped
         204/204 asi
         287/287 block-scope
             30/30 leave
             4/4 return-from
             30/30 shadowing
             223/223 syntax
                 16/16 for-in
                 12/12 function-declarations
                 189/189 redeclaration
                 6/6 redeclaration-global
         81/81 comments
             35/35 hashbang
         96/96 computed-property-names
             6/6 basics
             58/58 class
                 8/8 accessor
                 22/22 method
                 28/28 static
             24/24 object
                 12/12 accessor
                 10/10 method
                 2/2 property
             8/8 to-name-side-effects
         34/34 destructuring
             34/34 binding
                 28/28 syntax
         62/62 directive-prologue
         452/454 eval-code
             336/336 direct
             116/118 indirect
         3/3 export
         20244/20339 (70) expressions
             95/95 addition
             104/104 array
             643/643 arrow-function
                 10/10 arrow
                 454/454 dstr
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
                 77/77 syntax
                     43/43 early-errors
             833/839 assignment
                 6/6 destructuring
                 640/640 dstr
             615/615 assignmenttargettype
             108/108 async-arrow-function
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             158/161 async-function
                 10/10 forbidden-ext
                     4/4 b1
                     6/6 b2
             1209/1212 async-generator
                 744/744 dstr
                 10/10 forbidden-ext
                     4/4 b1
                     6/6 b2
             42/42 await
             59/59 bitwise-and
             32/32 bitwise-not
             59/59 bitwise-or
             59/59 bitwise-xor
             170/171 call
             8010/8017 (7) class
                 42/42 accessor-name-inst
                 42/42 accessor-name-static
                 191/191 async-gen-method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 191/191 async-gen-method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 59/59 async-method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 59/59 async-method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 0/5 (5) decorator
                     0/5 (5) syntax
                         0/1 (1) class-valid
                         0/4 (4) valid
                 3840/3840 dstr
                 2839/2841 (2) elements
                     156/156 async-gen-private-method
                     156/156 async-gen-private-method-static
                     24/24 async-private-method
                     24/24 async-private-method-static
                     12/12 evaluation-error
                     20/20 gen-private-method
                     20/20 gen-private-method-static
                     40/40 private-accessor-name
                     10/10 private-methods
                     498/499 (1) syntax
                         444/444 early-errors
                             192/192 delete
                             56/56 invalid-names
                         54/55 (1) valid
                 55/55 gen-method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 55/55 gen-method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 35/35 method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 35/35 method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 72/72 subclass-builtins
             46/46 coalesce
             11/11 comma
             786/786 compound-assignment
             10/10 concatenation
             42/42 conditional
             99/99 delete
             89/89 division
             75/75 does-not-equals
             959/1022 (63) dynamic-import
                 53/53 assignment-expression
                 208/208 catch
                 116/116 namespace
                 320/362 (42) syntax
                     194/194 invalid
                     126/168 (42) valid
                 216/216 usage
             93/93 equals
             88/88 exponentiation
             480/484 function
                 372/372 dstr
                 8/8 early-errors
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             542/546 generators
                 372/372 dstr
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             97/97 greater-than
             85/85 greater-than-or-equal
             16/16 grouping
             27/27 import.meta
                 23/23 syntax
             69/69 in
             85/85 instanceof
             89/89 left-shift
             89/89 less-than
             93/93 less-than-or-equal
             34/34 logical-and
             132/132 logical-assignment
             38/38 logical-not
             34/34 logical-or
             79/79 modulus
             79/79 multiplication
             118/118 new
             28/28 new.target
             2248/2250 object
                 1122/1122 dstr
                 557/557 method-definition
                     20/20 forbidden-ext
                         8/8 b1
                         12/12 b2
             76/76 optional-chaining
             63/63 postfix-decrement
             64/64 postfix-increment
             56/56 prefix-decrement
             55/55 prefix-increment
             42/42 property-accessors
             2/2 relational
             73/73 right-shift
             59/59 strict-does-not-equals
             59/59 strict-equals
             75/75 subtraction
             166/168 super
             48/48 tagged-template
             114/114 template-literal
             11/11 this
             32/32 typeof
             28/28 unary-minus
             34/34 unary-plus
             89/89 unsigned-right-shift
             18/18 void
             123/123 yield
         281/281 function-code
         85/85 future-reserved-words
         73/73 global-code
         21/22 identifier-resolution
         471/471 identifiers
         4/16 (12) import
         50/50 keywords
         82/82 line-terminators
         865/865 literals
             118/118 bigint
                 92/92 numeric-separators
             4/4 boolean
             4/4 null
             301/301 numeric
                 126/126 numeric-separators
             310/310 regexp
                 112/112 named-groups
             128/128 string
         552/581 (29) module-code
             36/36 namespace
                 34/34 internals
             245/245 top-level-await
                 211/211 syntax
         22/22 punctuators
         53/53 reserved-words
         22/22 rest-parameters
         2/2 source-text
         160/160 statementList
         17584/17596 (7) statements
             133/133 async-function
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             588/588 async-generator
                 372/372 dstr
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             40/40 block
                 8/8 early-errors
             40/40 break
             8630/8639 (7) class
                 42/42 accessor-name-inst
                 42/42 accessor-name-static
                 4/4 arguments
                 191/191 async-gen-method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 191/191 async-gen-method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 59/59 async-method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 59/59 async-method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 0/5 (5) decorator
                     0/5 (5) syntax
                         0/1 (1) class-valid
                         0/4 (4) valid
                 130/130 definition
                 3840/3840 dstr
                 3046/3048 (2) elements
                     156/156 async-gen-private-method
                     156/156 async-gen-private-method-static
                     24/24 async-private-method
                     24/24 async-private-method-static
                     12/12 evaluation-error
                     20/20 gen-private-method
                     20/20 gen-private-method-static
                     40/40 private-accessor-name
                     10/10 private-methods
                     498/499 (1) syntax
                         444/444 early-errors
                             192/192 delete
                             56/56 invalid-names
                         54/55 (1) valid
                 55/55 gen-method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 55/55 gen-method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 35/35 method
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 35/35 method-static
                     5/5 forbidden-ext
                         2/2 b1
                         3/3 b2
                 12/12 name-binding
                 4/4 strict-mode
                 214/216 subclass
                     140/140 builtin-objects
                         10/10 Array
                         4/4 ArrayBuffer
                         4/4 Boolean
                         4/4 DataView
                         4/4 Date
                         6/6 Error
                         8/8 Function
                         10/10 GeneratorFunction
                         4/4 Map
                         36/36 NativeError
                         4/4 Number
                         8/8 Object
                         4/4 Promise
                         2/2 Proxy
                         6/6 RegExp
                         4/4 Set
                         6/6 String
                         4/4 Symbol
                         4/4 TypedArray
                         4/4 WeakMap
                         4/4 WeakSet
                 72/72 subclass-builtins
                 16/16 super
                 26/26 syntax
                     4/4 early-errors
             271/271 const
                 186/186 dstr
                 50/50 syntax
             48/48 continue
             4/4 debugger
             70/70 do-while
             4/4 empty
             6/6 expression
             758/758 for
                 570/570 dstr
             2425/2427 for-await-of
             196/196 for-in
                 49/49 dstr
             1416/1416 for-of
                 1095/1095 dstr
             783/783 function
                 372/372 dstr
                 8/8 early-errors
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             510/510 generators
                 372/372 dstr
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             125/125 if
             37/37 labeled
             287/287 let
                 186/186 dstr
                 60/60 syntax
             31/31 return
             216/216 switch
                 127/127 syntax
                     127/127 redeclaration
             28/28 throw
             387/388 try
                 186/186 dstr
             309/309 variable
                 194/194 dstr
             72/72 while
             170/170 with
         211/211 types
             10/10 boolean
             6/6 list
             8/8 null
             39/39 number
             36/36 object
             49/49 reference
             48/48 string
             15/15 undefined
         134/134 white-space

### Built-ins

     33011/37884 (4833) built-ins
         5583/5596 (1) Array
             8/8 Symbol.species
             90/90 from
             58/58 isArray
             58/58 length
             32/32 of
             5237/5250 (1) prototype
                 2/2 Symbol.iterator
                 6/7 (1) Symbol.unscopables
                 22/22 at
                 135/137 concat
                 76/76 copyWithin
                 18/18 entries
                 427/427 every
                 40/40 fill
                 472/474 filter
                 38/38 find
                 38/38 findIndex
                 38/38 findLast
                 38/38 findLastIndex
                 38/38 flat
                 45/45 flatMap
                 370/370 forEach
                 54/54 includes
                 393/393 indexOf
                 40/40 join
                 18/18 keys
                 389/389 lastIndexOf
                 421/423 map
                 46/46 pop
                 48/48 push
                 511/511 reduce
                 509/511 reduceRight
                 34/34 reverse
                 40/40 shift
                 134/136 slice
                 428/428 some
                 99/99 sort
                 160/162 splice
                 14/14 toLocaleString
                 22/22 toString
                 42/42 unshift
                 18/18 values
         300/300 ArrayBuffer
             8/8 Symbol.species
             34/34 isView
             208/208 prototype
                 20/20 byteLength
                 22/22 maxByteLength
                 20/20 resizable
                 40/40 resize
                 64/64 slice
                 38/38 transfer
         46/46 ArrayIteratorPrototype
             6/6 Symbol.toStringTag
             40/40 next
         2/2 AsyncArrowFunction
         52/52 AsyncFromSyncIteratorPrototype
             14/14 next
             20/20 return
             18/18 throw
         36/36 AsyncFunction
         46/46 AsyncGeneratorFunction
             12/12 prototype
         96/96 AsyncGeneratorPrototype
             22/22 next
             38/38 return
             32/32 throw
         8/8 AsyncIteratorPrototype
             8/8 Symbol.asyncIterator
         526/633 (107) Atomics
             30/30 add
                 6/6 bigint
             30/30 and
                 6/6 bigint
             32/32 compareExchange
                 6/6 bigint
             32/32 exchange
                 6/6 bigint
             14/14 isLockFree
                 2/2 bigint
             28/28 load
                 6/6 bigint
             94/94 notify
                 16/16 bigint
             30/30 or
                 6/6 bigint
             32/32 store
                 6/6 bigint
             30/30 sub
                 6/6 bigint
             138/145 (7) wait
                 46/48 (2) bigint
             0/100 (100) waitAsync
                 0/44 (44) bigint
             30/30 xor
                 6/6 bigint
         148/148 BigInt
             28/28 asIntN
             28/28 asUintN
             2/2 parseInt
             48/48 prototype
                 2/2 toLocaleString
                 22/22 toString
                 16/16 valueOf
         101/101 Boolean
             51/51 prototype
                 2/2 constructor
                 20/20 toString
                 20/20 valueOf
         1010/1010 DataView
             888/888 prototype
                 22/22 buffer
                 28/28 byteLength
                 26/26 byteOffset
                 42/42 getBigInt64
                 42/42 getBigUint64
                 42/42 getFloat32
                 42/42 getFloat64
                 36/36 getInt16
                 56/56 getInt32
                 34/34 getInt8
                 36/36 getUint16
                 36/36 getUint32
                 34/34 getUint8
                 46/46 setBigInt64
                 4/4 setBigUint64
                 46/46 setFloat32
                 46/46 setFloat64
                 46/46 setInt16
                 46/46 setInt32
                 42/42 setInt8
                 46/46 setUint16
                 46/46 setUint32
                 42/42 setUint8
         1540/1540 Date
             44/44 UTC
             12/12 now
             26/26 parse
             1304/1304 prototype
                 36/36 Symbol.toPrimitive
                 14/14 constructor
                 26/26 getDate
                 26/26 getDay
                 26/26 getFullYear
                 26/26 getHours
                 26/26 getMilliseconds
                 26/26 getMinutes
                 26/26 getMonth
                 26/26 getSeconds
                 26/26 getTime
                 26/26 getTimezoneOffset
                 26/26 getUTCDate
                 26/26 getUTCDay
                 26/26 getUTCFullYear
                 26/26 getUTCHours
                 26/26 getUTCMilliseconds
                 26/26 getUTCMinutes
                 26/26 getUTCMonth
                 26/26 getUTCSeconds
                 34/34 setDate
                 46/46 setFullYear
                 52/52 setHours
                 34/34 setMilliseconds
                 42/42 setMinutes
                 40/40 setMonth
                 40/40 setSeconds
                 32/32 setTime
                 20/20 setUTCDate
                 18/18 setUTCFullYear
                 20/20 setUTCHours
                 20/20 setUTCMilliseconds
                 20/20 setUTCMinutes
                 20/20 setUTCMonth
                 20/20 setUTCSeconds
                 24/24 toDateString
                 34/34 toISOString
                 26/26 toJSON
                 18/18 toLocaleDateString
                 18/18 toLocaleString
                 18/18 toLocaleTimeString
                 26/26 toString
                 22/22 toTimeString
                 28/28 toUTCString
                 22/22 valueOf
         82/82 Error
             54/54 prototype
                 4/4 constructor
                 2/2 message
                 2/2 name
                 28/28 toString
         122/122 FinalizationRegistry
             88/88 prototype
                 28/28 cleanupSome
                 32/32 register
                 20/20 unregister
         887/889 Function
             16/16 internals
                 4/4 Call
                 12/12 Construct
             26/26 length
             596/598 prototype
                 22/22 Symbol.hasInstance
                 86/86 apply
                 200/200 bind
                 90/90 call
                 2/2 constructor
                 158/160 toString
         46/46 GeneratorFunction
             12/12 prototype
         120/120 GeneratorPrototype
             28/28 next
             44/44 return
             44/44 throw
         10/10 Infinity
         8/8 IteratorPrototype
             8/8 Symbol.iterator
         286/288 JSON
             144/144 parse
             130/132 stringify
         312/312 Map
             8/8 Symbol.species
             244/244 prototype
                 2/2 Symbol.iterator
                 22/22 clear
                 22/22 delete
                 20/20 entries
                 36/36 forEach
                 22/22 get
                 22/22 has
                 20/20 keys
                 28/28 set
                 22/22 size
                 20/20 values
         22/22 MapIteratorPrototype
             20/20 next
         622/622 Math
             4/4 E
             4/4 LN10
             4/4 LN2
             4/4 LOG10E
             4/4 LOG2E
             4/4 PI
             4/4 SQRT1_2
             4/4 SQRT2
             16/16 abs
             16/16 acos
             14/14 acosh
             18/18 asin
             10/10 asinh
             14/14 atan
             22/22 atan2
             10/10 atanh
             10/10 cbrt
             22/22 ceil
             20/20 clz32
             18/18 cos
             10/10 cosh
             18/18 exp
             10/10 expm1
             22/22 floor
             18/18 fround
             24/24 hypot
             10/10 imul
             18/18 log
             10/10 log10
             10/10 log1p
             10/10 log2
             20/20 max
             20/20 min
             56/56 pow
             10/10 random
             22/22 round
             10/10 sign
             16/16 sin
             10/10 sinh
             18/18 sqrt
             18/18 tan
             10/10 tanh
             24/24 trunc
         10/10 NaN
         234/234 NativeErrors
             50/50 AggregateError
                 12/12 prototype
             30/30 EvalError
                 10/10 prototype
             30/30 RangeError
                 10/10 prototype
             30/30 ReferenceError
                 10/10 prototype
             30/30 SyntaxError
                 10/10 prototype
             30/30 TypeError
                 10/10 prototype
             30/30 URIError
                 10/10 prototype
         670/670 Number
             6/6 MAX_VALUE
             6/6 MIN_VALUE
             8/8 NEGATIVE_INFINITY
             8/8 POSITIVE_INFINITY
             16/16 isFinite
             18/18 isInteger
             14/14 isNaN
             20/20 isSafeInteger
             2/2 parseFloat
             2/2 parseInt
             330/330 prototype
                 30/30 toExponential
                 26/26 toFixed
                 8/8 toLocaleString
                 34/34 toPrecision
                 180/180 toString
                 22/22 valueOf
         6742/6742 Object
             74/74 assign
             640/640 create
             1262/1262 defineProperties
             2244/2244 defineProperty
             42/42 entries
             102/102 freeze
             50/50 fromEntries
             620/620 getOwnPropertyDescriptor
             36/36 getOwnPropertyDescriptors
             90/90 getOwnPropertyNames
             24/24 getOwnPropertySymbols
             78/78 getPrototypeOf
             124/124 hasOwn
             12/12 internals
                 12/12 DefineOwnProperty
             42/42 is
             76/76 isExtensible
             118/118 isFrozen
             66/66 isSealed
             118/118 keys
             78/78 preventExtensions
             474/474 prototype
                 22/22 __defineGetter__
                 22/22 __defineSetter__
                 32/32 __lookupGetter__
                 32/32 __lookupSetter__
                 30/30 __proto__
                 4/4 constructor
                 126/126 hasOwnProperty
                 20/20 isPrototypeOf
                 32/32 propertyIsEnumerable
                 22/22 toLocaleString
                 66/66 toString
                 40/40 valueOf
             186/186 seal
             24/24 setPrototypeOf
             40/40 values
         1220/1220 Promise
             10/10 Symbol.species
             192/192 all
             204/204 allSettled
             184/184 any
             242/242 prototype
                 28/28 catch
                 56/56 finally
                 146/146 then
             184/184 race
             30/30 reject
             60/60 resolve
         607/607 Proxy
             28/28 apply
             58/58 construct
             48/48 defineProperty
             30/30 deleteProperty
             2/2 enumerate
             38/38 get
             42/42 getOwnPropertyDescriptor
             38/38 getPrototypeOf
             43/43 has
             24/24 isExtensible
             54/54 ownKeys
             23/23 preventExtensions
             35/35 revocable
             54/54 set
             34/34 setPrototypeOf
         306/306 Reflect
             18/18 apply
             20/20 construct
             24/24 defineProperty
             22/22 deleteProperty
             2/2 enumerate
             22/22 get
             26/26 getOwnPropertyDescriptor
             20/20 getPrototypeOf
             20/20 has
             16/16 isExtensible
             26/26 ownKeys
             20/20 preventExtensions
             36/36 set
             28/28 setPrototypeOf
         2970/3018 (32) RegExp
             48/48 CharacterClassEscapes
             8/8 Symbol.species
             8/8 dotall
             34/34 lookBehind
             28/28 match-indices
             52/52 named-groups
             1112/1144 (32) property-escapes
                 826/858 (32) generated
                     0/32 (32) strings
             866/882 prototype
                 102/102 Symbol.match
                 52/52 Symbol.matchAll
                 134/134 Symbol.replace
                 46/46 Symbol.search
                 88/88 Symbol.split
                 14/16 dotAll
                 152/152 exec
                 32/32 flags
                 18/20 global
                 14/16 hasIndices
                 18/20 ignoreCase
                 18/20 multiline
                 22/24 source
                 14/16 sticky
                 90/90 test
                 18/18 toString
                 14/16 unicode
         34/34 RegExpStringIteratorPrototype
             30/30 next
         392/392 Set
             8/8 Symbol.species
             340/340 prototype
                 2/2 Symbol.iterator
                 2/2 Symbol.toStringTag
                 42/42 add
                 38/38 clear
                 4/4 constructor
                 40/40 delete
                 34/34 entries
                 64/64 forEach
                 60/60 has
                 2/2 keys
                 12/12 size
                 36/36 values
         22/22 SetIteratorPrototype
             20/20 next
         0/64 (64) ShadowRealm
             0/5 (5) WrappedFunction
             0/51 (51) prototype
                 0/37 (37) evaluate
                 0/12 (12) importValue
         202/202 SharedArrayBuffer
             156/156 prototype
                 18/18 byteLength
                 30/30 grow
                 18/18 growable
                 20/20 maxByteLength
                 64/64 slice
         2323/2325 String
             30/30 fromCharCode
             22/22 fromCodePoint
             2028/2030 prototype
                 12/12 Symbol.iterator
                 22/22 at
                 60/60 charAt
                 50/50 charCodeAt
                 32/32 codePointAt
                 44/44 concat
                 4/4 constructor
                 54/54 endsWith
                 54/54 includes
                 94/94 indexOf
                 48/48 lastIndexOf
                 24/26 localeCompare
                 88/88 match
                 40/40 matchAll
                 28/28 normalize
                 26/26 padEnd
                 26/26 padStart
                 32/32 repeat
                 92/92 replace
                 82/82 replaceAll
                 74/74 search
                 72/72 slice
                 232/232 split
                 42/42 startsWith
                 88/88 substring
                 56/56 toLocaleLowerCase
                 52/52 toLocaleUpperCase
                 56/56 toLowerCase
                 14/14 toString
                 52/52 toUpperCase
                 258/258 trim
                 46/46 trimEnd
                 46/46 trimStart
                 14/14 valueOf
             60/60 raw
         14/14 StringIteratorPrototype
             10/10 next
         176/180 Symbol
             4/4 asyncIterator
             16/18 for
             4/4 hasInstance
             4/4 isConcatSpreadable
             4/4 iterator
             14/16 keyFor
             4/4 match
             4/4 matchAll
             68/68 prototype
                 18/18 Symbol.toPrimitive
                 14/14 description
                 14/14 toString
                 16/16 valueOf
             4/4 replace
             4/4 search
             8/8 species
             4/4 split
             4/4 toPrimitive
             4/4 toStringTag
             4/4 unscopables
         0/4629 (4629) Temporal
             0/571 (571) Calendar
                 0/15 (15) from
                 0/550 (550) prototype
                     0/46 (46) dateAdd
                     0/19 (19) dateFromFields
                     0/37 (37) dateUntil
                     0/32 (32) day
                     0/31 (31) dayOfWeek
                     0/31 (31) dayOfYear
                     0/31 (31) daysInMonth
                     0/31 (31) daysInWeek
                     0/31 (31) daysInYear
                     0/13 (13) fields
                     0/2 (2) id
                     0/28 (28) inLeapYear
                     0/11 (11) mergeFields
                     0/33 (33) month
                     0/33 (33) monthCode
                     0/18 (18) monthDayFromFields
                     0/28 (28) monthsInYear
                     0/6 (6) toJSON
                     0/6 (6) toString
                     0/1 (1) toStringTag
                     0/31 (31) weekOfYear
                     0/32 (32) year
                     0/18 (18) yearMonthFromFields
             0/441 (441) Duration
                 0/44 (44) compare
                 0/19 (19) from
                 0/357 (357) prototype
                     0/9 (9) abs
                     0/58 (58) add
                     0/3 (3) blank
                     0/2 (2) days
                     0/2 (2) hours
                     0/2 (2) microseconds
                     0/2 (2) milliseconds
                     0/2 (2) minutes
                     0/2 (2) months
                     0/2 (2) nanoseconds
                     0/8 (8) negated
                     0/68 (68) round
                     0/2 (2) seconds
                     0/2 (2) sign
                     0/58 (58) subtract
                     0/9 (9) toJSON
                     0/6 (6) toLocaleString
                     0/34 (34) toString
                     0/1 (1) toStringTag
                     0/51 (51) total
                     0/7 (7) valueOf
                     0/2 (2) weeks
                     0/22 (22) with
                     0/2 (2) years
             0/318 (318) Instant
                 0/14 (14) compare
                 0/18 (18) from
                 0/7 (7) fromEpochMicroseconds
                 0/7 (7) fromEpochMilliseconds
                 0/7 (7) fromEpochNanoseconds
                 0/7 (7) fromEpochSeconds
                 0/251 (251) prototype
                     0/17 (17) add
                     0/3 (3) epochMicroseconds
                     0/3 (3) epochMilliseconds
                     0/3 (3) epochNanoseconds
                     0/3 (3) epochSeconds
                     0/15 (15) equals
                     0/22 (22) round
                     0/35 (35) since
                     0/17 (17) subtract
                     0/10 (10) toJSON
                     0/7 (7) toLocaleString
                     0/46 (46) toString
                     0/1 (1) toStringTag
                     0/16 (16) toZonedDateTime
                     0/11 (11) toZonedDateTimeISO
                     0/34 (34) until
                     0/6 (6) valueOf
             0/167 (167) Now
                 0/9 (9) instant
                 0/16 (16) plainDate
                 0/11 (11) plainDateISO
                 0/37 (37) plainDateTime
                 0/26 (26) plainDateTimeISO
                 0/13 (13) plainTimeISO
                 0/7 (7) timeZone
                 0/2 (2) toStringTag
                 0/27 (27) zonedDateTime
                 0/17 (17) zonedDateTimeISO
             0/500 (500) PlainDate
                 0/30 (30) compare
                 0/40 (40) from
                 0/416 (416) prototype
                     0/29 (29) add
                     0/2 (2) calendar
                     0/3 (3) day
                     0/3 (3) dayOfWeek
                     0/3 (3) dayOfYear
                     0/2 (2) daysInMonth
                     0/3 (3) daysInWeek
                     0/2 (2) daysInYear
                     0/34 (34) equals
                     0/9 (9) getISOFields
                     0/2 (2) inLeapYear
                     0/3 (3) month
                     0/2 (2) monthCode
                     0/3 (3) monthsInYear
                     0/64 (64) since
                     0/29 (29) subtract
                     0/7 (7) toJSON
                     0/7 (7) toLocaleString
                     0/27 (27) toPlainDateTime
                     0/9 (9) toPlainMonthDay
                     0/10 (10) toPlainYearMonth
                     0/16 (16) toString
                     0/1 (1) toStringTag
                     0/35 (35) toZonedDateTime
                     0/63 (63) until
                     0/7 (7) valueOf
                     0/3 (3) weekOfYear
                     0/21 (21) with
                     0/13 (13) withCalendar
                     0/3 (3) year
             0/670 (670) PlainDateTime
                 0/30 (30) compare
                 0/49 (49) from
                 0/569 (569) prototype
                     0/30 (30) add
                     0/2 (2) calendar
                     0/3 (3) day
                     0/3 (3) dayOfWeek
                     0/3 (3) dayOfYear
                     0/3 (3) daysInMonth
                     0/3 (3) daysInWeek
                     0/3 (3) daysInYear
                     0/31 (31) equals
                     0/9 (9) getISOFields
                     0/2 (2) hour
                     0/2 (2) inLeapYear
                     0/2 (2) microsecond
                     0/2 (2) millisecond
                     0/2 (2) minute
                     0/3 (3) month
                     0/2 (2) monthCode
                     0/3 (3) monthsInYear
                     0/2 (2) nanosecond
                     0/37 (37) round
                     0/2 (2) second
                     0/73 (73) since
                     0/30 (30) subtract
                     0/8 (8) toJSON
                     0/7 (7) toLocaleString
                     0/7 (7) toPlainDate
                     0/9 (9) toPlainMonthDay
                     0/7 (7) toPlainTime
                     0/9 (9) toPlainYearMonth
                     0/39 (39) toString
                     0/1 (1) toStringTag
                     0/28 (28) toZonedDateTime
                     0/76 (76) until
                     0/7 (7) valueOf
                     0/3 (3) weekOfYear
                     0/31 (31) with
                     0/14 (14) withCalendar
                     0/38 (38) withPlainDate
                     0/29 (29) withPlainTime
                     0/3 (3) year
             0/163 (163) PlainMonthDay
                 0/34 (34) from
                 0/113 (113) prototype
                     0/2 (2) calendar
                     0/4 (4) day
                     0/22 (22) equals
                     0/9 (9) getISOFields
                     0/1 (1) month
                     0/3 (3) monthCode
                     0/8 (8) toJSON
                     0/6 (6) toLocaleString
                     0/13 (13) toPlainDate
                     0/15 (15) toString
                     0/1 (1) toStringTag
                     0/7 (7) valueOf
                     0/21 (21) with
             0/469 (469) PlainTime
                 0/24 (24) compare
                 0/42 (42) from
                 0/388 (388) prototype
                     0/21 (21) add
                     0/3 (3) calendar
                     0/25 (25) equals
                     0/9 (9) getISOFields
                     0/2 (2) hour
                     0/2 (2) microsecond
                     0/2 (2) millisecond
                     0/2 (2) minute
                     0/2 (2) nanosecond
                     0/34 (34) round
                     0/2 (2) second
                     0/59 (59) since
                     0/21 (21) subtract
                     0/7 (7) toJSON
                     0/7 (7) toLocaleString
                     0/29 (29) toPlainDateTime
                     0/32 (32) toString
                     0/1 (1) toStringTag
                     0/42 (42) toZonedDateTime
                     0/59 (59) until
                     0/7 (7) valueOf
                     0/19 (19) with
             0/379 (379) PlainYearMonth
                 0/23 (23) compare
                 0/36 (36) from
                 0/303 (303) prototype
                     0/32 (32) add
                     0/2 (2) calendar
                     0/3 (3) daysInMonth
                     0/3 (3) daysInYear
                     0/25 (25) equals
                     0/9 (9) getISOFields
                     0/2 (2) inLeapYear
                     0/3 (3) month
                     0/2 (2) monthCode
                     0/3 (3) monthsInYear
                     0/56 (56) since
                     0/32 (32) subtract
                     0/7 (7) toJSON
                     0/6 (6) toLocaleString
                     0/13 (13) toPlainDate
                     0/15 (15) toString
                     0/1 (1) toStringTag
                     0/55 (55) until
                     0/7 (7) valueOf
                     0/23 (23) with
                     0/3 (3) year
             0/189 (189) TimeZone
                 0/15 (15) from
                 0/167 (167) prototype
                     0/35 (35) getInstantFor
                     0/11 (11) getNextTransition
                     0/12 (12) getOffsetNanosecondsFor
                     0/18 (18) getOffsetStringFor
                     0/31 (31) getPlainDateTimeFor
                     0/28 (28) getPossibleInstantsFor
                     0/11 (11) getPreviousTransition
                     0/4 (4) id
                     0/9 (9) toJSON
                     0/6 (6) toString
                     0/1 (1) toStringTag
             0/757 (757) ZonedDateTime
                 0/29 (29) compare
                 0/45 (45) from
                 0/669 (669) prototype
                     0/20 (20) add
                     0/2 (2) calendar
                     0/8 (8) day
                     0/6 (6) dayOfWeek
                     0/6 (6) dayOfYear
                     0/6 (6) daysInMonth
                     0/6 (6) daysInWeek
                     0/6 (6) daysInYear
                     0/3 (3) epochMicroseconds
                     0/3 (3) epochMilliseconds
                     0/3 (3) epochNanoseconds
                     0/3 (3) epochSeconds
                     0/32 (32) equals
                     0/16 (16) getISOFields
                     0/7 (7) hour
                     0/7 (7) hoursInDay
                     0/6 (6) inLeapYear
                     0/8 (8) microsecond
                     0/8 (8) millisecond
                     0/7 (7) minute
                     0/7 (7) month
                     0/6 (6) monthCode
                     0/6 (6) monthsInYear
                     0/7 (7) nanosecond
                     0/7 (7) offset
                     0/6 (6) offsetNanoseconds
                     0/31 (31) round
                     0/7 (7) second
                     0/58 (58) since
                     0/12 (12) startOfDay
                     0/20 (20) subtract
                     0/2 (2) timeZone
                     0/6 (6) toInstant
                     0/15 (15) toJSON
                     0/7 (7) toLocaleString
                     0/10 (10) toPlainDate
                     0/13 (13) toPlainDateTime
                     0/14 (14) toPlainMonthDay
                     0/12 (12) toPlainTime
                     0/14 (14) toPlainYearMonth
                     0/48 (48) toString
                     0/1 (1) toStringTag
                     0/57 (57) until
                     0/6 (6) valueOf
                     0/6 (6) weekOfYear
                     0/32 (32) with
                     0/12 (12) withCalendar
                     0/34 (34) withPlainDate
                     0/30 (30) withPlainTime
                     0/12 (12) withTimeZone
                     0/7 (7) year
             0/2 (2) toStringTag
         28/28 ThrowTypeError
         2506/2508 TypedArray
             8/8 Symbol.species
             28/28 from
             14/14 of
             2448/2450 prototype
                 2/2 Symbol.iterator
                 36/36 Symbol.toStringTag
                     18/18 BigInt
                 26/26 at
                     2/2 BigInt
                 24/24 buffer
                     4/4 BigInt
                 30/30 byteLength
                     8/8 BigInt
                 30/30 byteOffset
                     8/8 BigInt
                 122/122 copyWithin
                     48/48 BigInt
                 32/32 entries
                     8/8 BigInt
                 82/82 every
                     32/32 BigInt
                 96/96 fill
                     36/36 BigInt
                 154/154 filter
                     68/68 BigInt
                 66/66 find
                     24/24 BigInt
                 66/66 findIndex
                     24/24 BigInt
                 66/66 findLast
                     24/24 BigInt
                 66/66 findLastIndex
                     24/24 BigInt
                 78/78 forEach
                     30/30 BigInt
                 76/76 includes
                     28/28 BigInt
                 78/78 indexOf
                     30/30 BigInt
                 56/56 join
                     18/18 BigInt
                 32/32 keys
                     8/8 BigInt
                 74/74 lastIndexOf
                     28/28 BigInt
                 30/30 length
                     8/8 BigInt
                 148/150 map
                     64/64 BigInt
                 94/94 reduce
                     38/38 BigInt
                 94/94 reduceRight
                     38/38 BigInt
                 40/40 reverse
                     12/12 BigInt
                 204/204 set
                     98/98 BigInt
                 162/162 slice
                     72/72 BigInt
                 82/82 some
                     32/32 BigInt
                 62/62 sort
                     20/20 BigInt
                 124/124 subarray
                     54/54 BigInt
                 72/72 toLocaleString
                     28/28 BigInt
                 6/6 toString
                     2/2 BigInt
                 32/32 values
                     8/8 BigInt
         1406/1406 TypedArrayConstructors
             24/24 BigInt64Array
                 8/8 prototype
             24/24 BigUint64Array
                 8/8 prototype
             22/22 Float32Array
                 8/8 prototype
             22/22 Float64Array
                 8/8 prototype
             22/22 Int16Array
                 8/8 prototype
             22/22 Int32Array
                 8/8 prototype
             22/22 Int8Array
                 8/8 prototype
             22/22 Uint16Array
                 8/8 prototype
             22/22 Uint32Array
                 8/8 prototype
             22/22 Uint8Array
                 8/8 prototype
             22/22 Uint8ClampedArray
                 8/8 prototype
             222/222 ctors
                 106/106 buffer-arg
                 24/24 length-arg
                 14/14 no-args
                 52/52 object-arg
                 24/24 typedarray-arg
             226/226 ctors-bigint
                 104/104 buffer-arg
                 24/24 length-arg
                 14/14 no-args
                 62/62 object-arg
                 22/22 typedarray-arg
             112/112 from
                 54/54 BigInt
             428/428 internals
                 108/108 DefineOwnProperty
                     52/52 BigInt
                 54/54 Delete
                     26/26 BigInt
                 56/56 Get
                     28/28 BigInt
                 48/48 GetOwnProperty
                     24/24 BigInt
                 62/62 HasProperty
                     29/29 BigInt
                 20/20 OwnPropertyKeys
                     8/8 BigInt
                 80/80 Set
                     46/46 BigInt
             52/52 of
                 24/24 BigInt
             120/120 prototype
                 4/4 Symbol.toStringTag
                 4/4 buffer
                 4/4 byteLength
                 4/4 byteOffset
                 4/4 copyWithin
                 4/4 entries
                 4/4 every
                 4/4 fill
                 4/4 filter
                 4/4 find
                 4/4 findIndex
                 4/4 forEach
                 4/4 indexOf
                 4/4 join
                 4/4 keys
                 4/4 lastIndexOf
                 4/4 length
                 4/4 map
                 4/4 reduce
                 4/4 reduceRight
                 4/4 reverse
                 4/4 set
                 4/4 slice
                 4/4 some
                 4/4 sort
                 4/4 subarray
                 4/4 toLocaleString
                 4/4 toString
                 4/4 values
         186/186 WeakMap
             140/140 prototype
                 38/38 delete
                 22/22 get
                 36/36 has
                 38/38 set
         56/56 WeakRef
             26/26 prototype
                 18/18 deref
         158/158 WeakSet
             120/120 prototype
                 38/38 add
                 4/4 constructor
                 38/38 delete
                 36/36 has
         110/110 decodeURI
         110/110 decodeURIComponent
         62/62 encodeURI
         62/62 encodeURIComponent
         20/20 eval
         56/56 global
         34/34 isFinite
         34/34 isNaN
         118/118 parseFloat
         120/120 parseInt
         12/12 undefined

## Failures

Here under are the failed tests. The comments are primarily here for the sake of future versions of XS. 

### Language

	language/eval-code/indirect/realm.js
	language/expressions/call/eval-realm-indirect.js (sloppy)

One realm.

	language/expressions/assignment/fn-name-lhs-cover.js

Assignments should rename functions only if the left hand side is an identifier. XS also rename functions if the left hand side is a group with only an identifier.

	language/expressions/async-function/named-reassign-fn-name-in-body-in-arrow.js (sloppy)
	language/expressions/async-function/named-reassign-fn-name-in-body-in-eval.js (sloppy)
	language/expressions/async-function/named-reassign-fn-name-in-body.js (sloppy)
	language/expressions/async-generator/named-no-strict-reassign-fn-name-in-body-in-arrow.js (sloppy)
	language/expressions/async-generator/named-no-strict-reassign-fn-name-in-body-in-eval.js (sloppy)
	language/expressions/async-generator/named-no-strict-reassign-fn-name-in-body.js (sloppy)
	language/expressions/function/named-no-strict-reassign-fn-name-in-body-in-arrow.js (sloppy)
	language/expressions/function/named-no-strict-reassign-fn-name-in-body-in-eval.js (sloppy)
	language/expressions/function/named-no-strict-reassign-fn-name-in-body.js (sloppy)
	language/expressions/function/scope-name-var-open-non-strict.js (sloppy)
	language/expressions/generators/named-no-strict-reassign-fn-name-in-body-in-arrow.js (sloppy)
	language/expressions/generators/named-no-strict-reassign-fn-name-in-body-in-eval.js (sloppy)
	language/expressions/generators/named-no-strict-reassign-fn-name-in-body.js (sloppy)
	language/expressions/generators/scope-name-var-open-non-strict.js (sloppy)

The name of a function expression always defines a constant variable that reference the current function. In sloppy mode it should define a variable that can be assigned but does not change!

	language/expressions/object/literal-property-name-bigint.js
	
XS does not support bigint as property name.

	language/expressions/super/call-proto-not-ctor.js 
	
XS checks if super is a constructor before evaluating arguments.

	language/statements/class/subclass/default-constructor-spread-override.js
	
The default derived constructor should not use `%Array.prototype%  @@iterator`
	
	language/statements/for-await-of/head-lhs-async.js
	
`for (async of x)` is a syntax error but `for await (async of x)` should not be!

	language/statements/try/tco-catch.js (strict)

XS does not tail call optimize `return` inside `catch`

	language/expressions/assignment/target-member-computed-reference-null.js
	language/expressions/assignment/target-member-computed-reference-undefined.js
	language/identifier-resolution/assign-to-global-undefined.js

To be investigated.		

### Built-ins

	built-ins/Array/prototype/concat/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/filter/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/map/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/slice/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/splice/create-proto-from-ctor-realm-array.j	built-ins/RegExp/prototype/dotAll/cross-realm.js
	built-ins/RegExp/prototype/global/cross-realm.js
	built-ins/RegExp/prototype/ignoreCase/cross-realm.js
	built-ins/RegExp/prototype/multiline/cross-realm.js
	built-ins/RegExp/prototype/source/cross-realm.js 
	built-ins/RegExp/prototype/sticky/cross-realm.js
	built-ins/RegExp/prototype/unicode/cross-realm.js
	built-ins/Symbol/for/cross-realm.js
	built-ins/Symbol/for/cross-realm.js
	built-ins/Symbol/keyFor/cross-realm.js
	built-ins/Symbol/keyFor/cross-realm.js
	
One realm.
	
	built-ins/Array/prototype/reduceRight/length-near-integer-limit.js
	built-ins/String/prototype/localeCompare/15.5.4.9_CE.js
	built-ins/JSON/stringify/replacer-function-object-deleted-property.js
	
To be investigated.

	built-ins/Function/prototype/toString/method-computed-property-name.js
	built-ins/TypedArray/prototype/map/callbackfn-resize.js

Invalid tests.
	
### Skipped cases

`xst` skips cases with the following features:

- `Atomics.waitAsync`
- `ShadowRealm`
- `Temporal`
- `arbitrary-module-namespace-names`
- `array-grouping`
- `decorators`
- `import-assertions`
- `json-modules`

