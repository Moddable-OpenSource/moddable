# XS Conformance
Copyright 2016-2026 Moddable Tech, Inc.<BR>
Revised: January 31, 2026

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
- Optionally the XS linker can dead strip ECMAScript built-ins that Moddable apps do not use, and remove `length` and `name` properties from functions.

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

The official conformance test suite, [test262](https://github.com/tc39/test262), contains cases for the published specifications, together with cases for proposals at stages 3 and 4, which is great to prepare XS for future editions. The XS harness, `xst` uses adhoc comparisons of the frontmatter `[features]` to skip cases related to not yet implemented proposals. See the skipped features at the end of this document.

Currently, on macOS, XS passes **98.88%** of the language tests and **88.75%** of the built-ins tests.

Mostly because of `Temporal`, the number of skipped cases is significant. For implemented features, XS passes **99.95%** of the language tests and **99.85%** of the built-ins tests.

Details are here under. The numbers of skipped cases are between parentheses. The following section lists the failed tests with some explanations.

### Language

    43291/43780 (470) language
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
         37/37 destructuring
             37/37 binding
                 28/28 syntax
         62/62 directive-prologue
         452/454 eval-code
             336/336 direct
             116/118 indirect
         3/3 export
         20481/20843 (355) expressions
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
             848/850 assignment
                 15/15 destructuring
                 640/640 dstr
             623/627 (4) assignmenttargettype
             110/110 async-arrow-function
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             161/161 async-function
                 10/10 forbidden-ext
                     4/4 b1
                     6/6 b2
             1212/1212 async-generator
                 744/744 dstr
                 10/10 forbidden-ext
                     4/4 b1
                     6/6 b2
             44/44 await
             59/59 bitwise-and
             32/32 bitwise-not
             59/59 bitwise-or
             59/59 bitwise-xor
             170/171 call
             8010/8020 (10) class
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
                 0/8 (8) decorator
                     0/8 (8) syntax
                         0/1 (1) class-valid
                         0/7 (7) valid
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
             103/103 delete
             89/89 division
             75/75 does-not-equals
             1123/1464 (341) dynamic-import
                 53/53 assignment-expression
                 208/272 (64) catch
                 42/42 import-attributes
                 0/4 (4) import-defer
                     0/1 (1) import-defer-async-module
                     0/1 (1) import-defer-transitive-async-module
                     0/1 (1) sync
                     0/1 (1) sync-dependency-of-deferred-async-module
                 116/116 namespace
                 440/713 (273) syntax
                     234/423 (189) invalid
                     206/290 (84) valid
                 216/216 usage
             93/93 equals
             88/88 exponentiation
             484/484 function
                 372/372 dstr
                 8/8 early-errors
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             546/546 generators
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
             2/2 member-expression
             79/79 modulus
             79/79 multiplication
             118/118 new
             28/28 new.target
             2250/2252 object
                 1122/1122 dstr
                 557/557 method-definition
                     20/20 forbidden-ext
                         8/8 b1
                         12/12 b2
             76/76 optional-chaining
             65/65 postfix-decrement
             66/66 postfix-increment
             58/58 prefix-decrement
             57/57 prefix-increment
             42/42 property-accessors
             2/2 relational
             73/73 right-shift
             59/59 strict-does-not-equals
             59/59 strict-equals
             75/75 subtraction
             182/184 super
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
         75/75 global-code
         21/22 identifier-resolution
         535/535 identifiers
         16/98 (82) import
             12/12 import-attributes
             0/82 (82) import-defer
                 0/3 (3) deferred-namespace-object
                 0/12 (12) errors
                     0/1 (1) get-other-while-dep-evaluating
                     0/1 (1) get-other-while-dep-evaluating-async
                     0/1 (1) get-other-while-evaluating
                     0/1 (1) get-other-while-evaluating-async
                     0/1 (1) get-self-while-defer-evaluating
                     0/1 (1) get-self-while-evaluating-async
                     0/3 (3) module-throws
                     0/1 (1) resolution-error
                     0/1 (1) syntax-error
                 0/2 (2) evaluation-sync
                 0/4 (4) evaluation-top-level-await
                     0/1 (1) flattening-order
                     0/1 (1) import-defer-async-module
                     0/1 (1) import-defer-transitive-async-module
                     0/1 (1) sync-dependency-of-deferred-async-module
                 0/52 (52) evaluation-triggers
                 0/9 (9) syntax
         50/50 keywords
         82/82 line-terminators
         1037/1037 literals
             118/118 bigint
                 92/92 numeric-separators
             8/8 boolean
             6/6 null
             301/301 numeric
                 126/126 numeric-separators
             476/476 regexp
                 112/112 named-groups
             128/128 string
         573/594 (17) module-code
             6/9 ambiguous-export-bindings
             13/13 import-attributes
             36/36 namespace
                 34/34 internals
             0/1 (1) source-phase-import
             252/253 top-level-await
                 211/211 syntax
         22/22 punctuators
         53/53 reserved-words
         22/22 rest-parameters
         2/2 source-text
         160/160 statementList
         17769/17790 (16) statements
             133/133 async-function
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             590/590 async-generator
                 372/372 dstr
                 5/5 forbidden-ext
                     2/2 b1
                     3/3 b2
             67/67 await-using
                 65/65 syntax
             40/40 block
                 8/8 early-errors
             40/40 break
             8633/8651 (16) class
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
                 0/12 (12) decorator
                     0/12 (12) syntax
                         0/1 (1) class-valid
                         0/11 (11) valid
                 130/130 definition
                 3840/3840 dstr
                 3049/3052 (3) elements
                     156/156 async-gen-private-method
                     156/156 async-gen-private-method-static
                     24/24 async-private-method
                     24/24 async-private-method-static
                     12/12 evaluation-error
                     20/20 gen-private-method
                     20/20 gen-private-method-static
                     40/40 private-accessor-name
                     10/10 private-methods
                     502/503 (1) syntax
                         444/444 early-errors
                             192/192 delete
                             56/56 invalid-names
                         58/59 (1) valid
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
                 214/217 (1) subclass
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
             2429/2431 for-await-of
             198/198 for-in
                 49/49 dstr
             1442/1442 for-of
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
             69/69 using
                 67/67 syntax
             309/309 variable
                 194/194 dstr
             72/72 while
             182/182 with
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

     36937/41527 (4538) built-ins
         0/8 (8) AbstractModuleSource
             0/3 (3) prototype
         50/50 AggregateError
             12/12 prototype
         6099/6111 Array
             8/8 Symbol.species
             90/90 from
             186/186 fromAsync
             58/58 isArray
             60/60 length
             32/32 of
             5565/5577 prototype
                 2/2 Symbol.iterator
                 8/8 Symbol.unscopables
                 26/26 at
                 135/137 concat
                 78/78 copyWithin
                 24/24 entries
                 433/433 every
                 44/44 fill
                 478/480 filter
                 44/44 find
                 44/44 findIndex
                 46/46 findLast
                 46/46 findLastIndex
                 38/38 flat
                 45/45 flatMap
                 376/376 forEach
                 60/60 includes
                 401/401 indexOf
                 46/46 join
                 24/24 keys
                 395/395 lastIndexOf
                 427/429 map
                 46/46 pop
                 48/48 push
                 517/517 reduce
                 515/517 reduceRight
                 36/36 reverse
                 40/40 shift
                 140/142 slice
                 434/434 some
                 107/107 sort
                 160/162 splice
                 22/22 toLocaleString
                 34/34 toReversed
                 42/42 toSorted
                 60/60 toSpliced
                 22/22 toString
                 42/42 unshift
                 24/24 values
                 42/42 with
         392/392 ArrayBuffer
             8/8 Symbol.species
             34/34 isView
             294/294 prototype
                 20/20 byteLength
                 22/22 detached
                 22/22 maxByteLength
                 20/20 resizable
                 44/44 resize
                 66/66 slice
                 48/48 transfer
                 48/48 transferToFixedLength
         46/46 ArrayIteratorPrototype
             6/6 Symbol.toStringTag
             40/40 next
         104/104 AsyncDisposableStack
             78/78 prototype
                 12/12 adopt
                 12/12 defer
                 12/12 disposeAsync
                 10/10 disposed
                 12/12 move
                 14/14 use
         76/76 AsyncFromSyncIteratorPrototype
             26/26 next
             20/20 return
             30/30 throw
         36/36 AsyncFunction
         46/46 AsyncGeneratorFunction
             12/12 prototype
         96/96 AsyncGeneratorPrototype
             22/22 next
             38/38 return
             32/32 throw
         26/26 AsyncIteratorPrototype
             18/18 Symbol.asyncDispose
             8/8 Symbol.asyncIterator
         742/756 (8) Atomics
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
             100/102 notify
                 16/16 bigint
             30/30 or
                 6/6 bigint
             0/6 (6) pause
             32/32 store
                 6/6 bigint
             30/30 sub
                 6/6 bigint
             148/152 (2) wait
                 48/49 (1) bigint
             200/202 waitAsync
                 88/88 bigint
             30/30 xor
                 6/6 bigint
         150/150 BigInt
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
         1122/1122 DataView
             998/998 prototype
                 22/22 buffer
                 28/28 byteLength
                 26/26 byteOffset
                 42/42 getBigInt64
                 42/42 getBigUint64
                 42/42 getFloat16
                 42/42 getFloat32
                 42/42 getFloat64
                 36/36 getInt16
                 56/56 getInt32
                 34/34 getInt8
                 36/36 getUint16
                 36/36 getUint32
                 34/34 getUint8
                 48/48 setBigInt64
                 6/6 setBigUint64
                 48/48 setFloat16
                 48/48 setFloat32
                 48/48 setFloat64
                 48/48 setInt16
                 48/48 setInt32
                 44/44 setInt8
                 48/48 setUint16
                 48/48 setUint32
                 44/44 setUint8
         1172/1180 (8) Date
             34/34 UTC
             12/12 now
             16/16 parse
             954/962 (8) prototype
                 36/36 Symbol.toPrimitive
                 2/2 constructor
                 16/16 getDate
                 16/16 getDay
                 16/16 getFullYear
                 16/16 getHours
                 16/16 getMilliseconds
                 16/16 getMinutes
                 16/16 getMonth
                 16/16 getSeconds
                 16/16 getTime
                 16/16 getTimezoneOffset
                 16/16 getUTCDate
                 16/16 getUTCDay
                 16/16 getUTCFullYear
                 16/16 getUTCHours
                 16/16 getUTCMilliseconds
                 16/16 getUTCMinutes
                 16/16 getUTCMonth
                 16/16 getUTCSeconds
                 28/28 setDate
                 40/40 setFullYear
                 46/46 setHours
                 28/28 setMilliseconds
                 36/36 setMinutes
                 34/34 setMonth
                 34/34 setSeconds
                 22/22 setTime
                 14/14 setUTCDate
                 12/12 setUTCFullYear
                 22/22 setUTCHours
                 16/16 setUTCMilliseconds
                 16/16 setUTCMinutes
                 18/18 setUTCMonth
                 18/18 setUTCSeconds
                 14/14 toDateString
                 34/34 toISOString
                 26/26 toJSON
                 8/8 toLocaleDateString
                 8/8 toLocaleString
                 8/8 toLocaleTimeString
                 16/16 toString
                 0/8 (8) toTemporalInstant
                 12/12 toTimeString
                 18/18 toUTCString
                 12/12 valueOf
         182/182 DisposableStack
             156/156 prototype
                 22/22 adopt
                 20/20 defer
                 26/26 dispose
                 14/14 disposed
                 26/26 move
                 38/38 use
         106/106 Error
             24/24 isError
             54/54 prototype
                 4/4 constructor
                 2/2 message
                 2/2 name
                 28/28 toString
         94/94 FinalizationRegistry
             62/62 prototype
                 34/34 register
                 20/20 unregister
         891/893 Function
             16/16 internals
                 4/4 Call
                 12/12 Construct
             26/26 length
             600/602 prototype
                 22/22 Symbol.hasInstance
                 88/88 apply
                 2/2 arguments
                 200/200 bind
                 90/90 call
                 2/2 caller
                 2/2 caller-arguments
                 2/2 constructor
                 158/160 toString
         46/46 GeneratorFunction
             12/12 prototype
         122/122 GeneratorPrototype
             28/28 next
             46/46 return
             44/44 throw
         10/10 Infinity
         864/942 (78) Iterator
             64/64 concat
             38/38 from
             746/746 prototype
                 12/12 Symbol.dispose
                 10/10 Symbol.iterator
                 4/4 Symbol.toStringTag
                 4/4 constructor
                 68/68 drop
                 66/66 every
                 74/74 filter
                 64/64 find
                 88/88 flatMap
                 54/54 forEach
                 72/72 map
                 60/60 reduce
                 66/66 some
                 66/66 take
                 36/36 toArray
             0/36 (36) zip
             0/42 (42) zipKeyed
         328/330 JSON
             12/12 isRawJSON
             154/154 parse
             20/20 rawJSON
             130/132 stringify
         405/405 Map
             8/8 Symbol.species
             28/28 groupBy
             309/309 prototype
                 2/2 Symbol.iterator
                 22/22 clear
                 22/22 delete
                 20/20 entries
                 36/36 forEach
                 22/22 get
                 28/28 getOrInsert
                 37/37 getOrInsertComputed
                 22/22 has
                 20/20 keys
                 28/28 set
                 22/22 size
                 20/20 values
         22/22 MapIteratorPrototype
             20/20 next
         654/654 Math
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
             10/10 f16round
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
             20/20 sqrt
             20/20 sumPrecise
             18/18 tan
             10/10 tanh
             24/24 trunc
         10/10 NaN
         184/184 NativeErrors
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
         6802/6802 Object
             76/76 assign
             640/640 create
             1264/1264 defineProperties
             2250/2250 defineProperty
             42/42 entries
             104/104 freeze
             50/50 fromEntries
             620/620 getOwnPropertyDescriptor
             36/36 getOwnPropertyDescriptors
             90/90 getOwnPropertyNames
             24/24 getOwnPropertySymbols
             78/78 getPrototypeOf
             28/28 groupBy
             124/124 hasOwn
             12/12 internals
                 12/12 DefineOwnProperty
             42/42 is
             76/76 isExtensible
             118/118 isFrozen
             66/66 isSealed
             118/118 keys
             78/78 preventExtensions
             494/494 prototype
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
                 82/82 toString
                 40/40 valueOf
             186/186 seal
             24/24 setPrototypeOf
             40/40 values
         1272/1272 Promise
             10/10 Symbol.species
             196/196 all
             208/208 allSettled
             188/188 any
             242/242 prototype
                 28/28 catch
                 56/56 finally
                 146/146 then
             188/188 race
             30/30 reject
             60/60 resolve
             24/24 try
             12/12 withResolvers
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
         3734/3756 RegExp
             24/24 CharacterClassEscapes
             8/8 Symbol.species
             8/8 dotall
             40/40 escape
             34/34 lookBehind
             28/28 match-indices
             72/72 named-groups
             1226/1226 property-escapes
                 938/938 generated
                     56/56 strings
             954/972 prototype
                 106/106 Symbol.match
                 52/52 Symbol.matchAll
                 138/138 Symbol.replace
                 46/46 Symbol.search
                 88/88 Symbol.split
                 14/16 dotAll
                 158/158 exec
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
                 74/76 unicodeSets
             138/140 regexp-modifiers
                 16/16 syntax
                     16/16 valid
             228/228 unicodeSets
                 228/228 generated
         34/34 RegExpStringIteratorPrototype
             30/30 next
         764/764 Set
             8/8 Symbol.species
             712/712 prototype
                 2/2 Symbol.iterator
                 2/2 Symbol.toStringTag
                 42/42 add
                 38/38 clear
                 4/4 constructor
                 40/40 delete
                 56/56 difference
                 34/34 entries
                 64/64 forEach
                 60/60 has
                 56/56 intersection
                 50/50 isDisjointFrom
                 46/46 isSubsetOf
                 50/50 isSupersetOf
                 2/2 keys
                 12/12 size
                 56/56 symmetricDifference
                 58/58 union
                 36/36 values
         22/22 SetIteratorPrototype
             20/20 next
         0/64 (64) ShadowRealm
             0/5 (5) WrappedFunction
             0/51 (51) prototype
                 0/37 (37) evaluate
                 0/12 (12) importValue
         208/208 SharedArrayBuffer
             156/156 prototype
                 18/18 byteLength
                 30/30 grow
                 18/18 growable
                 20/20 maxByteLength
                 64/64 slice
         2419/2421 String
             30/30 fromCharCode
             22/22 fromCodePoint
             2124/2126 prototype
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
                 16/16 isWellFormed
                 48/48 lastIndexOf
                 24/26 localeCompare
                 102/102 match
                 50/50 matchAll
                 28/28 normalize
                 26/26 padEnd
                 26/26 padStart
                 32/32 repeat
                 104/104 replace
                 90/90 replaceAll
                 86/86 search
                 72/72 slice
                 240/240 split
                 42/42 startsWith
                 88/88 substring
                 56/56 toLocaleLowerCase
                 52/52 toLocaleUpperCase
                 56/56 toLowerCase
                 14/14 toString
                 52/52 toUpperCase
                 16/16 toWellFormed
                 258/258 trim
                 46/46 trimEnd
                 46/46 trimStart
                 14/14 valueOf
             60/60 raw
         14/14 StringIteratorPrototype
             10/10 next
         44/44 SuppressedError
             12/12 prototype
         180/184 Symbol
             2/2 asyncDispose
             4/4 asyncIterator
             2/2 dispose
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
         0/4372 (4372) Temporal
             0/514 (514) Duration
                 0/48 (48) compare
                 0/30 (30) from
                 0/408 (408) prototype
                     0/9 (9) abs
                     0/34 (34) add
                     0/3 (3) blank
                     0/3 (3) days
                     0/3 (3) hours
                     0/3 (3) microseconds
                     0/3 (3) milliseconds
                     0/3 (3) minutes
                     0/3 (3) months
                     0/3 (3) nanoseconds
                     0/8 (8) negated
                     0/119 (119) round
                     0/3 (3) seconds
                     0/3 (3) sign
                     0/34 (34) subtract
                     0/12 (12) toJSON
                     0/7 (7) toLocaleString
                     0/44 (44) toString
                     0/1 (1) toStringTag
                     0/73 (73) total
                     0/7 (7) valueOf
                     0/3 (3) weeks
                     0/22 (22) with
                     0/3 (3) years
             0/452 (452) Instant
                 0/28 (28) compare
                 0/30 (30) from
                 0/10 (10) fromEpochMilliseconds
                 0/9 (9) fromEpochNanoseconds
                 0/364 (364) prototype
                     0/28 (28) add
                     0/3 (3) epochMilliseconds
                     0/3 (3) epochNanoseconds
                     0/27 (27) equals
                     0/40 (40) round
                     0/69 (69) since
                     0/27 (27) subtract
                     0/9 (9) toJSON
                     0/7 (7) toLocaleString
                     0/54 (54) toString
                     0/1 (1) toStringTag
                     0/18 (18) toZonedDateTimeISO
                     0/68 (68) until
                     0/7 (7) valueOf
             0/66 (66) Now
                 0/9 (9) instant
                 0/9 (9) plainDateISO
                 0/13 (13) plainDateTimeISO
                 0/10 (10) plainTimeISO
                 0/6 (6) timeZoneId
                 0/2 (2) toStringTag
                 0/15 (15) zonedDateTimeISO
             0/611 (611) PlainDate
                 0/39 (39) compare
                 0/63 (63) from
                 0/490 (490) prototype
                     0/38 (38) add
                     0/2 (2) calendarId
                     0/2 (2) day
                     0/3 (3) dayOfWeek
                     0/3 (3) dayOfYear
                     0/3 (3) daysInMonth
                     0/3 (3) daysInWeek
                     0/3 (3) daysInYear
                     0/37 (37) equals
                     0/2 (2) era
                     0/2 (2) eraYear
                     0/3 (3) inLeapYear
                     0/2 (2) month
                     0/3 (3) monthCode
                     0/3 (3) monthsInYear
                     0/79 (79) since
                     0/37 (37) subtract
                     0/8 (8) toJSON
                     0/7 (7) toLocaleString
                     0/34 (34) toPlainDateTime
                     0/7 (7) toPlainMonthDay
                     0/8 (8) toPlainYearMonth
                     0/18 (18) toString
                     0/1 (1) toStringTag
                     0/45 (45) toZonedDateTime
                     0/79 (79) until
                     0/7 (7) valueOf
                     0/3 (3) weekOfYear
                     0/24 (24) with
                     0/17 (17) withCalendar
                     0/2 (2) year
                     0/3 (3) yearOfWeek
             0/728 (728) PlainDateTime
                 0/38 (38) compare
                 0/64 (64) from
                 0/599 (599) prototype
                     0/41 (41) add
                     0/2 (2) calendarId
                     0/2 (2) day
                     0/3 (3) dayOfWeek
                     0/3 (3) dayOfYear
                     0/3 (3) daysInMonth
                     0/3 (3) daysInWeek
                     0/3 (3) daysInYear
                     0/38 (38) equals
                     0/2 (2) era
                     0/2 (2) eraYear
                     0/2 (2) hour
                     0/3 (3) inLeapYear
                     0/2 (2) microsecond
                     0/2 (2) millisecond
                     0/2 (2) minute
                     0/2 (2) month
                     0/3 (3) monthCode
                     0/3 (3) monthsInYear
                     0/2 (2) nanosecond
                     0/44 (44) round
                     0/2 (2) second
                     0/89 (89) since
                     0/41 (41) subtract
                     0/8 (8) toJSON
                     0/7 (7) toLocaleString
                     0/7 (7) toPlainDate
                     0/7 (7) toPlainTime
                     0/49 (49) toString
                     0/1 (1) toStringTag
                     0/29 (29) toZonedDateTime
                     0/92 (92) until
                     0/7 (7) valueOf
                     0/3 (3) weekOfYear
                     0/30 (30) with
                     0/17 (17) withCalendar
                     0/36 (36) withPlainTime
                     0/2 (2) year
                     0/3 (3) yearOfWeek
             0/188 (188) PlainMonthDay
                 0/53 (53) from
                 0/114 (114) prototype
                     0/2 (2) calendarId
                     0/3 (3) day
                     0/33 (33) equals
                     0/1 (1) month
                     0/3 (3) monthCode
                     0/7 (7) toJSON
                     0/7 (7) toLocaleString
                     0/12 (12) toPlainDate
                     0/16 (16) toString
                     0/1 (1) toStringTag
                     0/7 (7) valueOf
                     0/20 (20) with
             0/477 (477) PlainTime
                 0/30 (30) compare
                 0/49 (49) from
                 0/380 (380) prototype
                     0/32 (32) add
                     0/30 (30) equals
                     0/2 (2) hour
                     0/2 (2) microsecond
                     0/2 (2) millisecond
                     0/2 (2) minute
                     0/2 (2) nanosecond
                     0/40 (40) round
                     0/2 (2) second
                     0/74 (74) since
                     0/32 (32) subtract
                     0/7 (7) toJSON
                     0/7 (7) toLocaleString
                     0/40 (40) toString
                     0/1 (1) toStringTag
                     0/74 (74) until
                     0/7 (7) valueOf
                     0/22 (22) with
             0/482 (482) PlainYearMonth
                 0/36 (36) compare
                 0/58 (58) from
                 0/367 (367) prototype
                     0/37 (37) add
                     0/2 (2) calendarId
                     0/3 (3) daysInMonth
                     0/3 (3) daysInYear
                     0/37 (37) equals
                     0/2 (2) era
                     0/2 (2) eraYear
                     0/3 (3) inLeapYear
                     0/2 (2) month
                     0/3 (3) monthCode
                     0/3 (3) monthsInYear
                     0/78 (78) since
                     0/38 (38) subtract
                     0/8 (8) toJSON
                     0/7 (7) toLocaleString
                     0/12 (12) toPlainDate
                     0/17 (17) toString
                     0/1 (1) toStringTag
                     0/76 (76) until
                     0/7 (7) valueOf
                     0/22 (22) with
                     0/2 (2) year
             0/849 (849) ZonedDateTime
                 0/47 (47) compare
                 0/82 (82) from
                 0/701 (701) prototype
                     0/42 (42) add
                     0/2 (2) calendarId
                     0/3 (3) day
                     0/2 (2) dayOfWeek
                     0/2 (2) dayOfYear
                     0/2 (2) daysInMonth
                     0/2 (2) daysInWeek
                     0/3 (3) daysInYear
                     0/3 (3) epochMilliseconds
                     0/3 (3) epochNanoseconds
                     0/52 (52) equals
                     0/2 (2) era
                     0/2 (2) eraYear
                     0/14 (14) getTimeZoneTransition
                     0/3 (3) hour
                     0/5 (5) hoursInDay
                     0/3 (3) inLeapYear
                     0/4 (4) microsecond
                     0/4 (4) millisecond
                     0/3 (3) minute
                     0/2 (2) month
                     0/3 (3) monthCode
                     0/2 (2) monthsInYear
                     0/3 (3) nanosecond
                     0/3 (3) offset
                     0/2 (2) offsetNanoseconds
                     0/45 (45) round
                     0/3 (3) second
                     0/96 (96) since
                     0/8 (8) startOfDay
                     0/41 (41) subtract
                     0/2 (2) timeZoneId
                     0/10 (10) toInstant
                     0/11 (11) toJSON
                     0/7 (7) toLocaleString
                     0/7 (7) toPlainDate
                     0/10 (10) toPlainDateTime
                     0/9 (9) toPlainTime
                     0/62 (62) toString
                     0/1 (1) toStringTag
                     0/94 (94) until
                     0/7 (7) valueOf
                     0/2 (2) weekOfYear
                     0/41 (41) with
                     0/16 (16) withCalendar
                     0/35 (35) withPlainTime
                     0/16 (16) withTimeZone
                     0/2 (2) year
                     0/3 (3) yearOfWeek
             0/2 (2) toStringTag
         26/28 ThrowTypeError
         2860/2860 TypedArray
             8/8 Symbol.species
             42/42 from
             16/16 of
             2776/2776 prototype
                 2/2 Symbol.iterator
                 36/36 Symbol.toStringTag
                     18/18 BigInt
                 30/30 at
                     2/2 BigInt
                 24/24 buffer
                     4/4 BigInt
                 36/36 byteLength
                     8/8 BigInt
                 32/32 byteOffset
                     8/8 BigInt
                 128/128 copyWithin
                     48/48 BigInt
                 38/38 entries
                     8/8 BigInt
                 88/88 every
                     32/32 BigInt
                 102/102 fill
                     36/36 BigInt
                 168/168 filter
                     72/72 BigInt
                 72/72 find
                     24/24 BigInt
                 72/72 findIndex
                     24/24 BigInt
                 72/72 findLast
                     24/24 BigInt
                 72/72 findLastIndex
                     24/24 BigInt
                 84/84 forEach
                     30/30 BigInt
                 90/90 includes
                     28/28 BigInt
                 86/86 indexOf
                     30/30 BigInt
                 64/64 join
                     18/18 BigInt
                 38/38 keys
                     8/8 BigInt
                 84/84 lastIndexOf
                     28/28 BigInt
                 36/36 length
                     8/8 BigInt
                 168/168 map
                     68/68 BigInt
                 100/100 reduce
                     38/38 BigInt
                 100/100 reduceRight
                     38/38 BigInt
                 42/42 reverse
                     12/12 BigInt
                 218/218 set
                     98/98 BigInt
                 182/182 slice
                     76/76 BigInt
                 88/88 some
                     32/32 BigInt
                 70/70 sort
                     20/20 BigInt
                 134/134 subarray
                     54/54 BigInt
                 78/78 toLocaleString
                     28/28 BigInt
                 18/18 toReversed
                 24/24 toSorted
                 6/6 toString
                     2/2 BigInt
                 42/42 values
                     8/8 BigInt
                 44/44 with
                     2/2 BigInt
         1442/1442 TypedArrayConstructors
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
             232/232 ctors
                 108/108 buffer-arg
                 24/24 length-arg
                 14/14 no-args
                 56/56 object-arg
                 28/28 typedarray-arg
             226/226 ctors-bigint
                 104/104 buffer-arg
                 24/24 length-arg
                 14/14 no-args
                 62/62 object-arg
                 22/22 typedarray-arg
             112/112 from
                 54/54 BigInt
             454/454 internals
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
                 106/106 Set
                     54/54 BigInt
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
         136/136 Uint8Array
             26/26 fromBase64
             18/18 fromHex
             92/92 prototype
                 34/34 setFromBase64
                 24/24 setFromHex
                 20/20 toBase64
                 14/14 toHex
         281/281 WeakMap
             233/233 prototype
                 44/44 delete
                 26/26 get
                 34/34 getOrInsert
                 43/43 getOrInsertComputed
                 40/40 has
                 40/40 set
         58/58 WeakRef
             26/26 prototype
                 18/18 deref
         170/170 WeakSet
             132/132 prototype
                 44/44 add
                 4/4 constructor
                 40/40 delete
                 40/40 has
         110/110 decodeURI
         112/112 decodeURIComponent
         62/62 encodeURI
         62/62 encodeURIComponent
         20/20 eval
         56/56 global
         30/30 isFinite
         30/30 isNaN
         108/108 parseFloat
         110/110 parseInt
         12/12 undefined

## Failures

Here under are the failed tests. The comments are primarily here for the sake of future versions of XS.

### Language

	language/eval-code/indirect/realm.js
	language/expressions/call/eval-realm-indirect.js (sloppy)

One realm.

	language/expressions/assignment/fn-name-lhs-cover.js

Assignments should rename functions only if the left hand side is an identifier. XS also rename functions if the left hand side is a group with only an identifier.

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

	language/identifier-resolution/assign-to-global-undefined.js
	language/module-code/ambiguous-export-bindings/namespace-unambiguous-if-export-star-as-from-and-import-star-as-and-export.js
	language/module-code/ambiguous-export-bindings/namespace-unambiguous-if-export-star-as-from.js
	language/module-code/ambiguous-export-bindings/namespace-unambiguous-if-import-star-as-and-export.js
	language/module-code/top-level-await/pending-async-dep-from-cycle.js

To be investigated.

### Built-ins

	built-ins/Array/prototype/concat/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/filter/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/map/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/slice/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/splice/create-proto-from-ctor-realm-array.js
	built-ins/RegExp/prototype/dotAll/cross-realm.js
	built-ins/RegExp/prototype/global/cross-realm.js
	built-ins/RegExp/prototype/ignoreCase/cross-realm.js
	built-ins/RegExp/prototype/multiline/cross-realm.js
	built-ins/RegExp/prototype/source/cross-realm.js
	built-ins/RegExp/prototype/sticky/cross-realm.js
	built-ins/RegExp/prototype/unicode/cross-realm.js
	built-ins/RegExp/prototype/unicodeSets/cross-realm.js
	built-ins/Symbol/for/cross-realm.js
	built-ins/Symbol/for/cross-realm.js
	built-ins/Symbol/keyFor/cross-realm.js
	built-ins/Symbol/keyFor/cross-realm.js
	built-ins/ThrowTypeError/distinct-cross-realm.js

One realm.

	built-ins/Array/prototype/reduceRight/length-near-integer-limit.js
	built-ins/JSON/stringify/replacer-function-object-deleted-property.js
	built-ins/RegExp/nullable-quantifier.js
	built-ins/RegExp/regexp-modifiers/add-ignoreCase-affects-slash-lower-p.js
	built-ins/String/prototype/localeCompare/15.5.4.9_CE.js

To be investigated.

	built-ins/Atomics/notify/retrieve-length-before-index-coercion.js
	built-ins/Atomics/wait/retrieve-length-before-index-coercion.js
	built-ins/Atomics/waitAsync/retrieve-length-before-index-coercion.js

Tests assume SharedArrayBuffer instances can be grown.

	built-ins/Function/prototype/toString/method-computed-property-name.js

Invalid test.

## Skipped Features

Here under are the skipped features. Tests can be skipped because of several features.

### Language

- arbitrary-module-namespace-names (16)
- decorators (24)
- import-defer (209)
- nonextensible-applies-to-private (3)
- source-phase-imports (216)
- source-phase-imports-module-source (84)

### Built-ins

- Atomics.pause (6)
- ShadowRealm (64)
- Temporal (4380)
- joint-iteration (78)
- source-phase-imports (8)
