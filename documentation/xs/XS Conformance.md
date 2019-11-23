# XS Conformance

Copyright 2016-2019 Moddable Tech, Inc.

Revised: November 20, 2019

## Caveat

#### Realm

XS supports one realm by virtual machine. Tests that expect `$262.createRealm` to return a new realm fail.

#### String

Strings are UTF-8 encoded, their length is the number of code points they contain and they are indexed by code points. The *Unicode Escape Sequence* parser and `String.fromCharCode` combine surrogate pairs into one code point. But some tests related to code units fail.

XS depends on the platform for `String.prototype.normalize`, which succeeds only on iOS, macOS and Windows. For `String.prototype.toLowerCase` and `String.prototype.toUpperCase`, XS relies on the platform when possible, or uses compact tables that do do not completely conform to the specifications. 

XS does not implement the tagged template cache. So related tests fail.

XS does not implement ECMA-402, the Internationalization API Specification, so the `intl402` tests are skipped.

#### Annex B

No XS hosts are web browsers, so the `annexB` tests are skipped. However XS implements `Date.prototype.getYear`, `Date.prototype.setYear`, `Object.prototype.__proto__`, `String.prototype.substr`, `escape` and `unescape`, 

## Runtime models

On microcontrollers, XS uses a runtime model based on a virtual machine prepared by the XS compiler and linker. The prepared virtual machine contains the ECMAScript built-ins, along with the classes, functions and objects of preloaded modules. The prepared virtual machine is in ROM, its contents is shared by the the tiny virtual machines that XS quickly creates in RAM to run apps.

Such a runtime model introduces no conformance issues in itself since XS can alias shared classes, functions and objects if apps modify them. However, in order to save ROM and RAM, other restrictions have been introduced:

- Functions have no `length` property.
- Functions have no `name` property.
- Host functions, i.e. functions implemented in C, are primitive values like booleans, numbers, strings, etc. They are promoted to `Function` objects when necessary.
- Scripts evaluation is optional. So some platforms do not support `eval`, `new Function`, `new AsyncFunction` or `new Generator`. But all platforms support `JSON.parse`. 
- The XS linker can dead strip ECMAScript built-ins that Moddable apps do not use.

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
	$MODDABLE/build/bin/mac/debug/xst language/block-scope
	$MODDABLE/build/bin/mac/debug/xst built-ins/TypedArrays/buffer-arg-*

## Results

After the 6th edition, TC39 adopted a [process](https://tc39.github.io/process-document/) based on [proposals](https://github.com/tc39/proposals). Each proposal has a maturity stage. At stage 4, proposals are finished and will be published in the following edition of the specifications.

The official conformance test suite, [test262](https://github.com/tc39/test262), contains cases for the published specifications, together with cases for proposals at stages 3 and 4, which is great to prepare XS for future editions. The XS harness, `xst` uses adhoc comparisons of the frontmatter `[features]` to skip cases related to not yet implemented proposals. See the skipped cases at the end of this document.

Currently, on macOS, XS passes **99.8%** of the language tests (`38847/38911`) and **99.8%** of the built-ins tests (`29925/29998`). Details are here under. The numbers of skipped cases are between parentheses. The following section lists the failed tests with some explanations.

### Language

     99% 38847/38911 (58) language
        100% 457/457 arguments-object
            100% 40/40 mapped
            100% 8/8 unmapped
        100% 204/204 asi
        100% 287/287 block-scope
            100% 30/30 leave
            100% 4/4 return-from
            100% 30/30 shadowing
            100% 223/223 syntax
                100% 16/16 for-in
                100% 12/12 function-declarations
                100% 189/189 redeclaration
                100% 6/6 redeclaration-global
        100% 81/81 comments
            100% 35/35 hashbang
        100% 90/90 computed-property-names
            100% 6/6 basics
            100% 52/52 class
                100% 8/8 accessor
                100% 22/22 method
                100% 22/22 static
            100% 24/24 object
                100% 12/12 accessor
                100% 10/10 method
                100% 2/2 property
            100% 8/8 to-name-side-effects
        100% 30/30 destructuring
            100% 30/30 binding
                100% 24/24 syntax
        100% 62/62 directive-prologue
         99% 253/255 eval-code
            100% 139/139 direct
             98% 114/116 indirect
        100% 3/3 export
         99% 18100/18141 (49) expressions
            100% 95/95 addition
            100% 104/104 array
            100% 617/617 arrow-function
                100% 10/10 arrow
                100% 442/442 dstr
                100% 77/77 syntax
                    100% 43/43 early-errors
             99% 801/806 (4) assignment
                100% 6/6 destructuring
                100% 618/618 (4) dstr
            100% 100/100 async-arrow-function
            100% 127/127 async-function
            100% 1158/1158 async-generator
                100% 720/720 dstr
            100% 42/42 await
            100% 59/59 bitwise-and
            100% 32/32 bitwise-not
            100% 59/59 bitwise-or
            100% 59/59 bitwise-xor
             98% 176/179 call
             99% 7276/7284 (1) class
                100% 42/42 accessor-name-inst
                100% 42/42 accessor-name-static
                100% 186/186 async-gen-method
                100% 186/186 async-gen-method-static
                100% 42/42 async-method
                100% 42/42 async-method-static
                100% 3720/3720 dstr
                 99% 2613/2621 (1) elements
                    100% 156/156 async-gen-private-method
                    100% 156/156 async-gen-private-method-static
                    100% 12/12 async-private-method
                    100% 12/12 async-private-method-static
                    100% 12/12 evaluation-error
                    100% 20/20 gen-private-method
                    100% 20/20 gen-private-method-static
                    100% 40/40 private-accessor-name
                    100% 10/10 private-methods
                    100% 484/484 (1) syntax
                        100% 440/440 (1) early-errors
                            100% 192/192 delete
                            100% 56/56 invalid-names
                        100% 44/44 valid
                100% 50/50 gen-method
                100% 50/50 gen-method-static
                100% 30/30 method
                100% 30/30 method-static
              0% 0/0 (24) coalesce
            100% 11/11 comma
             99% 699/701 compound-assignment
            100% 10/10 concatenation
            100% 40/40 (1) conditional
            100% 85/85 delete
            100% 89/89 division
            100% 75/75 does-not-equals
             99% 994/999 dynamic-import
                100% 53/53 assignment-expression
                100% 208/208 catch
                100% 116/116 namespace
                100% 360/360 syntax
                    100% 234/234 invalid
                    100% 126/126 valid
                100% 216/216 usage
            100% 93/93 equals
            100% 88/88 exponentiation
             99% 452/453 function
                100% 360/360 dstr
                100% 8/8 early-errors
             99% 515/516 generators
                100% 360/360 dstr
            100% 97/97 greater-than
            100% 85/85 greater-than-or-equal
            100% 16/16 grouping
            100% 27/27 import.meta
                100% 23/23 syntax
            100% 27/27 in
            100% 85/85 instanceof
            100% 89/89 left-shift
            100% 89/89 less-than
            100% 93/93 less-than-or-equal
            100% 34/34 logical-and
            100% 38/38 logical-not
            100% 34/34 logical-or
             97% 77/79 modulus
            100% 79/79 multiplication
            100% 110/110 new
            100% 28/28 new.target
             99% 2044/2046 object
                100% 1080/1080 dstr
                100% 496/496 method-definition
              0% 0/0 (19) optional-chaining
            100% 64/64 postfix-decrement
            100% 65/65 postfix-increment
            100% 57/57 prefix-decrement
            100% 56/56 prefix-increment
            100% 42/42 property-accessors
            100% 2/2 relational
            100% 73/73 right-shift
            100% 59/59 strict-does-not-equals
            100% 59/59 strict-equals
            100% 75/75 subtraction
             98% 164/166 super
             79% 38/48 tagged-template
            100% 108/108 template-literal
            100% 11/11 this
            100% 30/30 typeof
            100% 28/28 unary-minus
            100% 34/34 unary-plus
            100% 89/89 unsigned-right-shift
            100% 18/18 void
            100% 119/119 yield
         99% 285/287 function-code
        100% 85/85 future-reserved-words
        100% 73/73 global-code
        100% 19/19 identifier-resolution
        100% 289/289 identifiers
        100% 4/4 import
        100% 50/50 keywords
        100% 82/82 line-terminators
         99% 844/845 literals
            100% 118/118 bigint
                100% 92/92 numeric-separators
            100% 4/4 boolean
            100% 4/4 null
            100% 297/297 numeric
                100% 124/124 numeric-separators
            100% 312/312 regexp
                100% 116/116 named-groups
             99% 109/110 string
        100% 533/533 module-code
            100% 36/36 namespace
                100% 34/34 internals
            100% 240/240 top-level-await
                100% 211/211 syntax
        100% 22/22 punctuators
        100% 53/53 reserved-words
        100% 22/22 rest-parameters
          0% 0/2 source-text
        100% 160/160 statementList
         99% 16464/16480 (9) statements
            100% 113/113 async-function
            100% 566/566 async-generator
                100% 360/360 dstr
            100% 38/38 block
                100% 8/8 early-errors
            100% 38/38 break
             99% 7782/7794 (1) class
                100% 42/42 accessor-name-inst
                100% 42/42 accessor-name-static
                100% 4/4 arguments
                100% 186/186 async-gen-method
                100% 186/186 async-gen-method-static
                100% 42/42 async-method
                100% 42/42 async-method-static
                100% 124/124 definition
                100% 3720/3720 dstr
                 99% 2780/2790 (1) elements
                    100% 156/156 async-gen-private-method
                    100% 156/156 async-gen-private-method-static
                    100% 12/12 async-private-method
                    100% 12/12 async-private-method-static
                    100% 12/12 evaluation-error
                    100% 20/20 gen-private-method
                    100% 20/20 gen-private-method-static
                    100% 40/40 private-accessor-name
                    100% 10/10 private-methods
                    100% 484/484 (1) syntax
                        100% 440/440 (1) early-errors
                            100% 192/192 delete
                            100% 56/56 invalid-names
                        100% 44/44 valid
                100% 50/50 gen-method
                100% 50/50 gen-method-static
                100% 30/30 method
                100% 30/30 method-static
                100% 12/12 name-binding
                100% 4/4 strict-mode
                 98% 190/192 subclass
                    100% 140/140 builtin-objects
                        100% 10/10 Array
                        100% 4/4 ArrayBuffer
                        100% 4/4 Boolean
                        100% 4/4 DataView
                        100% 4/4 Date
                        100% 6/6 Error
                        100% 8/8 Function
                        100% 10/10 GeneratorFunction
                        100% 4/4 Map
                        100% 36/36 NativeError
                        100% 4/4 Number
                        100% 8/8 Object
                        100% 4/4 Promise
                        100% 2/2 Proxy
                        100% 6/6 RegExp
                        100% 4/4 Set
                        100% 6/6 String
                        100% 4/4 Symbol
                        100% 4/4 TypedArray
                        100% 4/4 WeakMap
                        100% 4/4 WeakSet
                100% 16/16 super
                100% 26/26 syntax
                    100% 4/4 early-errors
            100% 261/261 const
                100% 180/180 dstr
                100% 50/50 syntax
            100% 44/44 continue
            100% 4/4 debugger
            100% 70/70 do-while
            100% 4/4 empty
            100% 6/6 expression
            100% 738/738 for
                100% 552/552 dstr
            100% 2415/2415 for-await-of
            100% 178/178 (4) for-in
                100% 41/41 (4) dstr
             99% 1356/1360 (4) for-of
                100% 1055/1055 (4) dstr
            100% 758/758 function
                100% 360/360 dstr
                100% 8/8 early-errors
            100% 490/490 generators
                100% 360/360 dstr
            100% 125/125 if
            100% 35/35 labeled
            100% 277/277 let
                100% 180/180 dstr
                100% 60/60 syntax
            100% 31/31 return
            100% 204/204 switch
                100% 127/127 syntax
                    100% 127/127 redeclaration
            100% 28/28 throw
            100% 370/370 try
                100% 180/180 dstr
            100% 291/291 variable
                100% 180/180 dstr
            100% 72/72 while
            100% 170/170 with
        100% 211/211 types
            100% 10/10 boolean
            100% 6/6 list
            100% 8/8 null
            100% 39/39 number
            100% 36/36 object
            100% 49/49 reference
            100% 48/48 string
            100% 15/15 undefined
        100% 84/84 white-space

### Built-ins

     99% 29956/30034 (88) built-ins
          0% 0/0 (29) AggregateError
              0% 0/0 (14) prototype
                  0% 0/0 (9) errors
         99% 5197/5209 Array
            100% 8/8 Symbol.species
            100% 80/80 from
            100% 56/56 isArray
            100% 50/50 length
            100% 30/30 of
             99% 4875/4887 prototype
                100% 4/4 Symbol.unscopables
                 96% 113/117 concat
                100% 66/66 copyWithin
                100% 16/16 entries
                100% 421/421 every
                100% 34/34 fill
                 99% 462/464 filter
                100% 32/32 find
                100% 32/32 findIndex
                100% 28/28 flat
                100% 35/35 flatMap
                100% 364/364 forEach
                100% 50/50 includes
                100% 385/385 indexOf
                100% 38/38 join
                100% 16/16 keys
                100% 381/381 lastIndexOf
                 99% 411/413 map
                100% 34/34 pop
                100% 34/34 push
                100% 505/505 reduce
                100% 503/503 reduceRight
                100% 30/30 reverse
                100% 28/28 shift
                 98% 128/130 slice
                100% 422/422 some
                100% 59/59 sort
                 98% 154/156 splice
                100% 14/14 toLocaleString
                100% 18/18 toString
                100% 30/30 unshift
                100% 16/16 values
        100% 160/160 ArrayBuffer
            100% 8/8 Symbol.species
            100% 32/32 isView
            100% 86/86 prototype
                100% 20/20 byteLength
                100% 62/62 slice
        100% 46/46 ArrayIteratorPrototype
            100% 6/6 Symbol.toStringTag
            100% 40/40 next
        100% 44/44 AsyncFromSyncIteratorPrototype
            100% 12/12 next
            100% 16/16 return
            100% 16/16 throw
        100% 32/32 AsyncFunction
        100% 40/40 AsyncGeneratorFunction
            100% 10/10 prototype
        100% 90/90 AsyncGeneratorPrototype
            100% 22/22 next
            100% 32/32 return
            100% 32/32 throw
        100% 8/8 AsyncIteratorPrototype
            100% 8/8 Symbol.asyncIterator
         99% 470/472 (7) Atomics
            100% 26/26 add
                100% 6/6 bigint
            100% 26/26 and
                100% 6/6 bigint
            100% 28/28 compareExchange
                100% 6/6 bigint
            100% 26/26 exchange
                100% 6/6 bigint
            100% 12/12 isLockFree
                100% 2/2 bigint
            100% 24/24 load
                100% 6/6 bigint
            100% 82/82 notify
                100% 10/10 bigint
            100% 26/26 or
                100% 6/6 bigint
            100% 26/26 store
                100% 6/6 bigint
            100% 26/26 sub
                100% 6/6 bigint
             98% 136/138 (7) wait
                 97% 45/46 (2) bigint
            100% 26/26 xor
                100% 6/6 bigint
        100% 134/134 BigInt
            100% 26/26 asIntN
            100% 26/26 asUintN
            100% 2/2 parseInt
            100% 40/40 prototype
                100% 18/18 toString
                100% 14/14 valueOf
        100% 98/98 Boolean
            100% 50/50 prototype
                100% 2/2 constructor
                100% 18/18 toString
                100% 18/18 valueOf
        100% 910/910 DataView
            100% 798/798 prototype
                100% 22/22 buffer
                100% 22/22 byteLength
                100% 22/22 byteOffset
                100% 38/38 getBigInt64
                100% 38/38 getBigUint64
                100% 38/38 getFloat32
                100% 38/38 getFloat64
                100% 32/32 getInt16
                100% 52/52 getInt32
                100% 30/30 getInt8
                100% 32/32 getUint16
                100% 32/32 getUint32
                100% 30/30 getUint8
                100% 42/42 setBigInt64
                100% 42/42 setFloat32
                100% 42/42 setFloat64
                100% 42/42 setInt16
                100% 42/42 setInt32
                100% 38/38 setInt8
                100% 42/42 setUint16
                100% 42/42 setUint32
                100% 38/38 setUint8
        100% 1406/1406 Date
            100% 40/40 UTC
            100% 10/10 now
            100% 20/20 parse
            100% 1188/1188 prototype
                100% 34/34 Symbol.toPrimitive
                100% 14/14 constructor
                100% 24/24 getDate
                100% 24/24 getDay
                100% 24/24 getFullYear
                100% 24/24 getHours
                100% 24/24 getMilliseconds
                100% 24/24 getMinutes
                100% 24/24 getMonth
                100% 24/24 getSeconds
                100% 24/24 getTime
                100% 24/24 getTimezoneOffset
                100% 24/24 getUTCDate
                100% 24/24 getUTCDay
                100% 24/24 getUTCFullYear
                100% 24/24 getUTCHours
                100% 24/24 getUTCMilliseconds
                100% 24/24 getUTCMinutes
                100% 24/24 getUTCMonth
                100% 24/24 getUTCSeconds
                100% 30/30 setDate
                100% 44/44 setFullYear
                100% 48/48 setHours
                100% 30/30 setMilliseconds
                100% 38/38 setMinutes
                100% 36/36 setMonth
                100% 36/36 setSeconds
                100% 30/30 setTime
                100% 16/16 setUTCDate
                100% 16/16 setUTCFullYear
                100% 16/16 setUTCHours
                100% 16/16 setUTCMilliseconds
                100% 16/16 setUTCMinutes
                100% 16/16 setUTCMonth
                100% 16/16 setUTCSeconds
                100% 22/22 toDateString
                100% 32/32 toISOString
                100% 22/22 toJSON
                100% 16/16 toLocaleDateString
                100% 16/16 toLocaleString
                100% 16/16 toLocaleTimeString
                100% 24/24 toString
                100% 20/20 toTimeString
                100% 26/26 toUTCString
                100% 20/20 valueOf
        100% 76/76 Error
            100% 52/52 prototype
                100% 4/4 constructor
                100% 6/6 message
                100% 6/6 name
                100% 20/20 toString
         95% 128/134 FinalizationGroup
            100% 16/16 FinalizationGroupCleanupIteratorPrototype
             95% 82/86 prototype
                100% 32/32 cleanupSome
                 86% 26/30 register
                100% 16/16 unregister
         99% 893/895 Function
            100% 16/16 internals
                100% 4/4 Call
                100% 12/12 Construct
            100% 26/26 length
             99% 610/612 prototype
                100% 22/22 Symbol.hasInstance
                100% 88/88 apply
                100% 212/212 bind
                100% 92/92 call
                100% 2/2 constructor
                 98% 160/162 toString
        100% 40/40 GeneratorFunction
            100% 10/10 prototype
        100% 114/114 GeneratorPrototype
            100% 26/26 next
            100% 42/42 return
            100% 42/42 throw
        100% 10/10 Infinity
        100% 8/8 IteratorPrototype
            100% 8/8 Symbol.iterator
        100% 276/276 JSON
            100% 138/138 parse
            100% 126/126 stringify
        100% 286/286 Map
            100% 8/8 Symbol.species
            100% 224/224 prototype
                100% 20/20 clear
                100% 20/20 delete
                100% 18/18 entries
                100% 34/34 forEach
                100% 20/20 get
                100% 20/20 has
                100% 18/18 keys
                100% 26/26 set
                100% 22/22 size
                100% 18/18 values
        100% 22/22 MapIteratorPrototype
            100% 20/20 next
        100% 546/546 Math
            100% 4/4 E
            100% 4/4 LN10
            100% 4/4 LN2
            100% 4/4 LOG10E
            100% 4/4 LOG2E
            100% 4/4 PI
            100% 4/4 SQRT1_2
            100% 4/4 SQRT2
            100% 14/14 abs
            100% 14/14 acos
            100% 12/12 acosh
            100% 16/16 asin
            100% 8/8 asinh
            100% 12/12 atan
            100% 20/20 atan2
            100% 8/8 atanh
            100% 8/8 cbrt
            100% 20/20 ceil
            100% 18/18 clz32
            100% 16/16 cos
            100% 8/8 cosh
            100% 16/16 exp
            100% 8/8 expm1
            100% 20/20 floor
            100% 16/16 fround
            100% 20/20 hypot
            100% 8/8 imul
            100% 16/16 log
            100% 8/8 log10
            100% 8/8 log1p
            100% 8/8 log2
            100% 16/16 max
            100% 16/16 min
            100% 54/54 pow
            100% 8/8 random
            100% 20/20 round
            100% 8/8 sign
            100% 14/14 sin
            100% 8/8 sinh
            100% 16/16 sqrt
            100% 16/16 tan
            100% 8/8 tanh
            100% 22/22 trunc
        100% 10/10 NaN
        100% 170/170 NativeErrors
            100% 28/28 EvalError
                100% 10/10 prototype
            100% 28/28 RangeError
                100% 10/10 prototype
            100% 28/28 ReferenceError
                100% 10/10 prototype
            100% 28/28 SyntaxError
                100% 10/10 prototype
            100% 28/28 TypeError
                100% 10/10 prototype
            100% 28/28 URIError
                100% 10/10 prototype
        100% 562/562 Number
            100% 6/6 MAX_VALUE
            100% 6/6 MIN_VALUE
            100% 8/8 NEGATIVE_INFINITY
            100% 8/8 POSITIVE_INFINITY
            100% 14/14 isFinite
            100% 16/16 isInteger
            100% 12/12 isNaN
            100% 18/18 isSafeInteger
            100% 236/236 prototype
                100% 28/28 toExponential
                100% 24/24 toFixed
                100% 6/6 toLocaleString
                100% 30/30 toPrecision
                100% 98/98 toString
                100% 20/20 valueOf
        100% 6234/6234 Object
            100% 52/52 assign
            100% 638/638 create
            100% 1242/1242 defineProperties
            100% 2224/2224 defineProperty
            100% 34/34 entries
            100% 92/92 freeze
            100% 48/48 fromEntries
            100% 614/614 getOwnPropertyDescriptor
            100% 30/30 getOwnPropertyDescriptors
            100% 82/82 getOwnPropertyNames
            100% 16/16 getOwnPropertySymbols
            100% 76/76 getPrototypeOf
            100% 12/12 internals
                100% 12/12 DefineOwnProperty
            100% 40/40 is
            100% 74/74 isExtensible
            100% 114/114 isFrozen
            100% 62/62 isSealed
            100% 110/110 keys
            100% 72/72 preventExtensions
            100% 338/338 prototype
                100% 4/4 constructor
                100% 130/130 hasOwnProperty
                100% 20/20 isPrototypeOf
                100% 32/32 propertyIsEnumerable
                100% 22/22 toLocaleString
                100% 64/64 toString
                100% 40/40 valueOf
            100% 90/90 seal
            100% 22/22 setPrototypeOf
            100% 34/34 values
         99% 938/942 Promise
            100% 10/10 Symbol.species
             98% 176/178 all
             98% 180/182 allSettled
            100% 230/230 prototype
                100% 26/26 catch
                100% 48/48 finally
                100% 144/144 then
            100% 156/156 race
            100% 28/28 reject
            100% 58/58 resolve
        100% 509/509 Proxy
            100% 22/22 apply
            100% 52/52 construct
            100% 42/42 defineProperty
            100% 24/24 deleteProperty
            100% 2/2 enumerate
            100% 32/32 get
            100% 36/36 getOwnPropertyDescriptor
            100% 32/32 getPrototypeOf
            100% 37/37 has
            100% 18/18 isExtensible
            100% 48/48 ownKeys
            100% 18/18 preventExtensions
            100% 22/22 revocable
            100% 44/44 set
            100% 28/28 setPrototypeOf
        100% 274/274 Reflect
            100% 14/14 apply
            100% 18/18 construct
            100% 22/22 defineProperty
            100% 20/20 deleteProperty
            100% 2/2 enumerate
            100% 20/20 get
            100% 24/24 getOwnPropertyDescriptor
            100% 18/18 getPrototypeOf
            100% 18/18 has
            100% 14/14 isExtensible
            100% 22/22 ownKeys
            100% 18/18 preventExtensions
            100% 34/34 set
            100% 26/26 setPrototypeOf
         99% 2812/2838 (13) RegExp
            100% 48/48 CharacterClassEscapes
            100% 8/8 Symbol.species
             50% 4/8 dotall
            100% 34/34 lookBehind
              0% 0/0 (13) match-indices
            100% 44/44 named-groups
            100% 1076/1076 property-escapes
                100% 790/790 generated
             97% 790/812 prototype
                 98% 98/100 Symbol.match
                100% 50/50 Symbol.matchAll
                 98% 102/104 Symbol.replace
                100% 40/40 Symbol.search
                100% 86/86 Symbol.split
                 87% 14/16 dotAll
                 97% 144/148 exec
                100% 30/30 flags
                 90% 18/20 global
                 90% 18/20 ignoreCase
                 90% 18/20 multiline
                 91% 22/24 source
                 87% 14/16 sticky
                100% 88/88 test
                100% 14/14 toString
                 87% 14/16 unicode
        100% 34/34 RegExpStringIteratorPrototype
            100% 30/30 next
        100% 374/374 Set
            100% 8/8 Symbol.species
            100% 326/326 prototype
                100% 2/2 Symbol.toStringTag
                100% 40/40 add
                100% 36/36 clear
                100% 4/4 constructor
                100% 38/38 delete
                100% 32/32 entries
                100% 62/62 forEach
                100% 58/58 has
                100% 2/2 keys
                100% 12/12 size
                100% 34/34 values
        100% 22/22 SetIteratorPrototype
            100% 20/20 next
        100% 114/114 SharedArrayBuffer
            100% 84/84 prototype
                100% 18/18 byteLength
                100% 62/62 slice
         99% 2115/2131 (39) String
            100% 28/28 fromCharCode
            100% 20/20 fromCodePoint
             99% 1830/1844 (39) prototype
                100% 10/10 Symbol.iterator
                100% 58/58 charAt
                100% 48/48 charCodeAt
                 93% 28/30 codePointAt
                100% 42/42 concat
                100% 4/4 constructor
                100% 52/52 endsWith
                100% 52/52 includes
                100% 92/92 indexOf
                100% 46/46 lastIndexOf
                100% 24/24 localeCompare
                100% 84/84 match
                 89% 34/38 matchAll
                100% 26/26 normalize
                 91% 22/24 padEnd
                 91% 22/24 padStart
                100% 30/30 repeat
                100% 90/90 replace
                  0% 0/0 (39) replaceAll
                100% 70/70 search
                100% 70/70 slice
                100% 214/214 split
                100% 40/40 startsWith
                100% 86/86 substring
                 96% 52/54 toLocaleLowerCase
                100% 50/50 toLocaleUpperCase
                 96% 52/54 toLowerCase
                100% 12/12 toString
                100% 50/50 toUpperCase
                100% 256/256 trim
                100% 44/44 trimEnd
                100% 44/44 trimStart
                100% 12/12 valueOf
            100% 58/58 raw
         85% 12/14 StringIteratorPrototype
             80% 8/10 next
         97% 160/164 Symbol
            100% 4/4 asyncIterator
             85% 12/14 for
            100% 4/4 hasInstance
            100% 4/4 isConcatSpreadable
            100% 4/4 iterator
             85% 12/14 keyFor
            100% 4/4 match
            100% 4/4 matchAll
            100% 60/60 prototype
                100% 14/14 Symbol.toPrimitive
                100% 14/14 description
                100% 12/12 toString
                100% 14/14 valueOf
            100% 4/4 replace
            100% 4/4 search
            100% 8/8 species
            100% 4/4 split
            100% 4/4 toPrimitive
            100% 4/4 toStringTag
            100% 4/4 unscopables
        100% 26/26 ThrowTypeError
         99% 2096/2100 TypedArray
            100% 8/8 Symbol.species
            100% 26/26 from
            100% 12/12 of
             99% 2042/2046 prototype
                100% 36/36 Symbol.toStringTag
                    100% 18/18 BigInt
                100% 22/22 buffer
                    100% 4/4 BigInt
                100% 22/22 byteLength
                    100% 4/4 BigInt
                100% 22/22 byteOffset
                    100% 4/4 BigInt
                100% 108/108 copyWithin
                    100% 46/46 BigInt
                100% 26/26 entries
                    100% 6/6 BigInt
                100% 74/74 every
                    100% 30/30 BigInt
                100% 84/84 fill
                    100% 34/34 BigInt
                100% 146/146 filter
                    100% 66/66 BigInt
                100% 58/58 find
                    100% 22/22 BigInt
                100% 58/58 findIndex
                    100% 22/22 BigInt
                100% 70/70 forEach
                    100% 28/28 BigInt
                100% 60/60 includes
                    100% 22/22 BigInt
                100% 60/60 indexOf
                    100% 22/22 BigInt
                100% 46/46 join
                    100% 14/14 BigInt
                100% 26/26 keys
                    100% 6/6 BigInt
                100% 56/56 lastIndexOf
                    100% 20/20 BigInt
                100% 22/22 length
                    100% 4/4 BigInt
                100% 142/142 map
                    100% 62/62 BigInt
                100% 86/86 reduce
                    100% 36/36 BigInt
                100% 86/86 reduceRight
                    100% 36/36 BigInt
                100% 34/34 reverse
                    100% 10/10 BigInt
                100% 192/192 set
                    100% 94/94 BigInt
                100% 156/156 slice
                    100% 70/70 BigInt
                100% 74/74 some
                    100% 30/30 BigInt
                 92% 52/56 sort
                    100% 18/18 BigInt
                100% 122/122 subarray
                    100% 54/54 BigInt
                100% 66/66 toLocaleString
                    100% 26/26 BigInt
                100% 4/4 toString
                    100% 2/2 BigInt
                100% 26/26 values
                    100% 6/6 BigInt
        100% 1360/1360 TypedArrayConstructors
            100% 22/22 BigInt64Array
                100% 8/8 prototype
            100% 22/22 BigUint64Array
                100% 8/8 prototype
            100% 20/20 Float32Array
                100% 8/8 prototype
            100% 20/20 Float64Array
                100% 8/8 prototype
            100% 20/20 Int16Array
                100% 8/8 prototype
            100% 20/20 Int32Array
                100% 8/8 prototype
            100% 20/20 Int8Array
                100% 8/8 prototype
            100% 20/20 Uint16Array
                100% 8/8 prototype
            100% 20/20 Uint32Array
                100% 8/8 prototype
            100% 20/20 Uint8Array
                100% 8/8 prototype
            100% 20/20 Uint8ClampedArray
                100% 8/8 prototype
            100% 252/252 ctors
                100% 104/104 buffer-arg
                100% 24/24 length-arg
                100% 14/14 no-args
                100% 50/50 object-arg
                100% 60/60 typedarray-arg
            100% 258/258 ctors-bigint
                100% 104/104 buffer-arg
                100% 24/24 length-arg
                100% 14/14 no-args
                100% 62/62 object-arg
                100% 54/54 typedarray-arg
            100% 112/112 from
                100% 54/54 BigInt
            100% 342/342 internals
                100% 84/84 DefineOwnProperty
                    100% 40/40 BigInt
                100% 56/56 Get
                    100% 28/28 BigInt
                100% 48/48 GetOwnProperty
                    100% 24/24 BigInt
                100% 58/58 HasProperty
                    100% 29/29 BigInt
                100% 16/16 OwnPropertyKeys
                    100% 8/8 BigInt
                100% 80/80 Set
                    100% 46/46 BigInt
            100% 52/52 of
                100% 24/24 BigInt
            100% 120/120 prototype
                100% 4/4 Symbol.toStringTag
                100% 4/4 buffer
                100% 4/4 byteLength
                100% 4/4 byteOffset
                100% 4/4 copyWithin
                100% 4/4 entries
                100% 4/4 every
                100% 4/4 fill
                100% 4/4 filter
                100% 4/4 find
                100% 4/4 findIndex
                100% 4/4 forEach
                100% 4/4 indexOf
                100% 4/4 join
                100% 4/4 keys
                100% 4/4 lastIndexOf
                100% 4/4 length
                100% 4/4 map
                100% 4/4 reduce
                100% 4/4 reduceRight
                100% 4/4 reverse
                100% 4/4 set
                100% 4/4 slice
                100% 4/4 some
                100% 4/4 sort
                100% 4/4 subarray
                100% 4/4 toLocaleString
                100% 4/4 toString
                100% 4/4 values
          0% 0/0 TypedArrays
        100% 176/176 WeakMap
            100% 132/132 prototype
                100% 36/36 delete
                100% 20/20 get
                100% 34/34 has
                100% 36/36 set
        100% 52/52 WeakRef
            100% 24/24 prototype
                100% 16/16 deref
        100% 150/150 WeakSet
            100% 114/114 prototype
                100% 36/36 add
                100% 4/4 constructor
                100% 36/36 delete
                100% 34/34 has
        100% 108/108 decodeURI
        100% 108/108 decodeURIComponent
        100% 60/60 encodeURI
        100% 60/60 encodeURIComponent
        100% 18/18 eval
        100% 56/56 global
        100% 32/32 isFinite
        100% 32/32 isNaN
        100% 116/116 parseFloat
        100% 120/120 parseInt
        100% 12/12 undefined
                       
### Annex B

        100% 12/12 annexB/built-ins/Date/prototype/getYear
        100% 12/12 annexB/built-ins/Date/prototype/setYear
        100% 30/30 annexB/built-ins/Object/prototype/__proto__
         92% 24/26 annexB/built-ins/String/prototype/substr
        100% 22/22 annexB/built-ins/escape
        100% 28/28 annexB/built-ins/unescape

## Failures

Here under are the failed tests. The comments are primarily here for the sake of future versions of XS. 

### Language

	language/eval-code/indirect/realm.js

One realm.

	language/expressions/assignment/S11.13.1_A5_T4.js (sloppy)
	language/expressions/assignment/S11.13.1_A5_T5.js

When the right hand side deletes the variable assigned by the left hand side, the assignment fails in strict mode.

	language/expressions/assignment/fn-name-lhs-cover.js

Assignments should rename functions only if the left hand side is an identifier. XS also rename functions if the left hand side is a group with only an identifier.

	language/expressions/call/11.2.3-3_3.js

When calling a member property, XS evaluates parameters before the member property.

	language/expressions/call/eval-realm-indirect.js (sloppy)

One realm.

	language/expressions/class/elements/direct-eval-err-contains-arguments.js
	language/expressions/class/elements/private-direct-eval-err-contains-arguments.js

Reference error instead of syntax error.
	
	language/expressions/class/elements/private-getter-is-not-a-own-property.js
	language/expressions/class/elements/private-setter-is-not-a-own-property.js

Test uses Annex B `__lookupGetter__`.

	language/expressions/compound-assignment/mod-whitespace.js
		
XS optimizes modulus for integer values, which fails for -1 % -1 == -0.

	language/expressions/dynamic-import/eval-self-once-module.js
	language/expressions/dynamic-import/eval-self-once-script.js
	
A module cannot dynamically imports itself.

	language/expressions/dynamic-import/for-await-resolution-and-error-agen-yield.js

?

	language/expressions/function/scope-name-var-open-non-strict.js (sloppy)
	language/expressions/generators/scope-name-var-open-non-strict.js (sloppy)

The name of a function expression always defines a constant variable that reference the current function. In sloppy mode it should define a variable that can be assigned but does not change!

	language/expressions/modulus/S11.5.3_A4_T2.js

XS optimizes modulus for integer values, which fails for -1 % -1 == -0.

	language/expressions/object/fn-name-cover.js

In object initializers, if property values are functions, the implementation must rename functions with property names. It happens at runtime for the sake of computed property names. If property values are groups, the implementation should rename functions only if they are the unique expression of their group, XS rename functions if they are the last expression of their group.

	language/expressions/super/call-proto-not-ctor.js

When calling `super`, XS evaluates parameters before `super`.

	language/expressions/tagged-template/cache-different-functions-same-site.js
	language/expressions/tagged-template/cache-eval-inner-function.js
	language/expressions/tagged-template/cache-same-site-top-level.js
	language/expressions/tagged-template/cache-same-site.js
	language/expressions/tagged-template/template-object-template-map.js

No tagged templates cache.

	language/function-code/10.4.3-1-104.js (strict)
	language/function-code/10.4.3-1-106.js (strict)
	
When calling a member, XS promotes primitives into objects to lookup the property then uses the object instead of the primitive as `this`.

	language/literals/string/legacy-octal-escape-sequence-prologue-strict.js (sloppy)

Strings with octal escape sequences are a lexical error in strict mode but in sloppy mode if "use strict" follows the string, it is too late for a lexical error...

	language/source-text/6.1.js

Code points vs code units.	

	language/statements/class/elements/direct-eval-err-contains-arguments.js
	language/statements/class/elements/private-direct-eval-err-contains-arguments.js

Reference error instead of syntax error.
	
	language/statements/class/elements/private-getter-is-not-a-own-property.js
	language/statements/class/elements/private-setter-is-not-a-own-property.js

Test uses Annex B `__lookupGetter__`.

	language/statements/class/subclass/class-definition-null-proto-super.js

When calling `super`, XS evaluates parameters before `super`.

	language/statements/for-of/map-expand.js
	language/statements/for-of/set-expand.js

XS `Map` and `Set` entries iterators do not visit entries added after the last one is returned.

	language/statements/try/tco-catch.js (strict)

XS does not tail call optimize `return` inside `catch`

	language/statements/with/cptn-abrupt-empty.js (sloppy)

When evaluating string with `eval`, XS does not return the correct value if the string contains `break` or `continue` inside `with` inside an iteration statement.

### Built-ins

	built-ins/Array/prototype/concat/Array.prototype.concat_spreadable-string-wrapper.js
	
Code points vs code units.	

	built-ins/Array/prototype/concat/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/filter/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/map/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/slice/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/splice/create-proto-from-ctor-realm-array.js
	
One realm.
	
	built-ins/FinalizationGroup/gc-has-one-chance-to-call-cleanupCallback.js
	built-ins/FinalizationGroup/prototype/register/holdings-same-as-target.js
	built-ins/FinalizationGroup/prototype/register/unregisterToken-same-as-holdings-and-target.js

?

	built-ins/Function/prototype/toString/method-computed-property-name.js

Invalid test.

	built-ins/Promise/all/does-not-invoke-array-setters.js
	built-ins/Promise/allSettled/does-not-invoke-array-setters.js
	
XS does invoke the setters.

	built-ins/RegExp/prototype/dotAll/cross-realm.js
	built-ins/RegExp/prototype/global/cross-realm.js
	built-ins/RegExp/prototype/ignoreCase/cross-realm.js
	built-ins/RegExp/prototype/multiline/cross-realm.js
	built-ins/RegExp/prototype/source/cross-realm.js 
	built-ins/RegExp/prototype/sticky/cross-realm.js
	built-ins/RegExp/prototype/unicode/cross-realm.js
	
One realm.
		
	built-ins/RegExp/dotall/with-dotall.js
	built-ins/RegExp/dotall/without-dotall.js
	built-ins/RegExp/prototype/Symbol.match/builtin-infer-unicode.js
	built-ins/RegExp/prototype/Symbol.replace/coerce-unicode.js
	built-ins/RegExp/prototype/exec/u-captured-value.js
	built-ins/RegExp/prototype/exec/u-lastindex-value.js
	
Code points vs code units.	
	
	built-ins/String/length.js 
	built-ins/String/prototype/codePointAt/return-code-unit-coerced-position.js 
	built-ins/String/prototype/padEnd/normal-operation.js 
	built-ins/String/prototype/padStart/normal-operation.js 
	built-ins/String/prototype/toLocaleLowerCase/special_casing_conditional.js 
	built-ins/String/prototype/toLowerCase/special_casing_conditional.js 
	built-ins/StringIteratorPrototype/next/next-iteration-surrogate-pairs.js 

Code points vs code units.	

	built-ins/Symbol/for/cross-realm.js
	built-ins/Symbol/for/cross-realm.js
	built-ins/Symbol/keyFor/cross-realm.js
	built-ins/Symbol/keyFor/cross-realm.js
	
One realm.

	built-ins/TypedArray/prototype/sort/sorted-values.js
	built-ins/TypedArray/prototype/sortstability.js
	
Sorting typed arrays is unstable.	
	
### Annex B

	annexB/built-ins/String/prototype/substr/surrogate-pairs.js
	
Code points vs code units.	
	
### Skipped cases

	AggregateError
	coalesce-expression
	optional-chaining
	regexp-match-indices

