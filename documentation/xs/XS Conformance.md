# XS Conformance

Copyright 2016-2021 Moddable Tech, Inc.

Revised: January 5, 2021

## Caveat

#### Realm

XS supports one realm by virtual machine. Tests that expect `$262.createRealm` to return a new realm fail.

#### String

Strings are UTF-8 encoded, their length is the number of code points they contain and they are indexed by code points. The *Unicode Escape Sequence* parser and `String.fromCharCode` combine surrogate pairs into one code point. But some tests related to code units fail.

XS depends on the platform for `String.prototype.normalize`, which succeeds only on iOS, macOS and Windows. For `String.prototype.toLowerCase` and `String.prototype.toUpperCase`, XS relies on the platform when possible, or uses compact tables that do do not completely conform to the specifications. 

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
	$MODDABLE/build/bin/mac/debug/xst language/block-scope
	$MODDABLE/build/bin/mac/debug/xst built-ins/TypedArrays/buffer-arg-*

## Results

After the 6th edition, TC39 adopted a [process](https://tc39.github.io/process-document/) based on [proposals](https://github.com/tc39/proposals). Each proposal has a maturity stage. At stage 4, proposals are finished and will be published in the following edition of the specifications.

The official conformance test suite, [test262](https://github.com/tc39/test262), contains cases for the published specifications, together with cases for proposals at stages 3 and 4, which is great to prepare XS for future editions. The XS harness, `xst` uses adhoc comparisons of the frontmatter `[features]` to skip cases related to not yet implemented proposals. See the skipped cases at the end of this document.

Currently, on macOS, XS passes **99.9%** of the language tests (`39997/40026`) and **99.4%** of the built-ins tests (`30737/30929`). Details are here under. The numbers of skipped cases are between parentheses. The following section lists the failed tests with some explanations.

### Language

     99% 39997/40026 language
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
         99% 454/456 eval-code
            100% 338/338 direct
             98% 116/118 indirect
        100% 3/3 export
         99% 18640/18658 expressions
            100% 95/95 addition
            100% 104/104 array
            100% 630/630 arrow-function
                100% 10/10 arrow
                100% 454/454 dstr
                100% 77/77 syntax
                    100% 43/43 early-errors
             99% 813/818 assignment
                100% 6/6 destructuring
                100% 626/626 dstr
            100% 101/101 async-arrow-function
            100% 129/129 async-function
            100% 1192/1192 async-generator
                100% 744/744 dstr
            100% 42/42 await
            100% 59/59 bitwise-and
            100% 32/32 bitwise-not
            100% 59/59 bitwise-or
            100% 59/59 bitwise-xor
             99% 178/179 call
            100% 7428/7428 class
                100% 42/42 accessor-name-inst
                100% 42/42 accessor-name-static
                100% 186/186 async-gen-method
                100% 186/186 async-gen-method-static
                100% 42/42 async-method
                100% 42/42 async-method-static
                100% 3840/3840 dstr
                100% 2645/2645 elements
                    100% 156/156 async-gen-private-method
                    100% 156/156 async-gen-private-method-static
                    100% 12/12 async-private-method
                    100% 12/12 async-private-method-static
                    100% 12/12 evaluation-error
                    100% 20/20 gen-private-method
                    100% 20/20 gen-private-method-static
                    100% 40/40 private-accessor-name
                    100% 10/10 private-methods
                    100% 494/494 syntax
                        100% 440/440 early-errors
                            100% 192/192 delete
                            100% 56/56 invalid-names
                        100% 54/54 valid
                100% 50/50 gen-method
                100% 50/50 gen-method-static
                100% 30/30 method
                100% 30/30 method-static
            100% 46/46 coalesce
            100% 11/11 comma
             99% 699/701 compound-assignment
            100% 10/10 concatenation
            100% 42/42 conditional
            100% 86/86 delete
            100% 89/89 division
            100% 75/75 does-not-equals
            100% 999/999 dynamic-import
                100% 53/53 assignment-expression
                100% 208/208 catch
                100% 116/116 namespace
                100% 360/360 syntax
                    100% 234/234 invalid
                    100% 126/126 valid
                100% 216/216 usage
            100% 93/93 equals
            100% 88/88 exponentiation
             99% 465/466 function
                100% 372/372 dstr
                100% 8/8 early-errors
             99% 528/529 generators
                100% 372/372 dstr
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
            100% 90/90 logical-assignment
            100% 38/38 logical-not
            100% 34/34 logical-or
             97% 77/79 modulus
            100% 79/79 multiplication
            100% 118/118 new
            100% 28/28 new.target
             99% 2090/2094 object
                100% 1118/1118 dstr
                100% 500/500 method-definition
            100% 76/76 optional-chaining
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
             98% 166/168 super
            100% 48/48 tagged-template
            100% 114/114 template-literal
            100% 11/11 this
            100% 32/32 typeof
            100% 28/28 unary-minus
            100% 34/34 unary-plus
            100% 89/89 unsigned-right-shift
            100% 18/18 void
            100% 123/123 yield
        100% 281/281 function-code
        100% 85/85 future-reserved-words
        100% 73/73 global-code
         95% 19/20 identifier-resolution
        100% 375/375 identifiers
        100% 4/4 import
        100% 50/50 keywords
        100% 82/82 line-terminators
         99% 850/851 literals
            100% 118/118 bigint
                100% 92/92 numeric-separators
            100% 4/4 boolean
            100% 4/4 null
            100% 299/299 numeric
                100% 126/126 numeric-separators
            100% 310/310 regexp
                100% 112/112 named-groups
             99% 115/116 string
        100% 547/547 module-code
            100% 36/36 namespace
                100% 34/34 internals
            100% 244/244 top-level-await
                100% 211/211 syntax
        100% 22/22 punctuators
        100% 53/53 reserved-words
        100% 22/22 rest-parameters
          0% 0/2 source-text
        100% 160/160 statementList
         99% 16771/16776 statements
            100% 114/114 async-function
            100% 581/581 async-generator
                100% 372/372 dstr
            100% 38/38 block
                100% 8/8 early-errors
            100% 38/38 break
            100% 7944/7944 class
                100% 42/42 accessor-name-inst
                100% 42/42 accessor-name-static
                100% 4/4 arguments
                100% 186/186 async-gen-method
                100% 186/186 async-gen-method-static
                100% 42/42 async-method
                100% 42/42 async-method-static
                100% 124/124 definition
                100% 3840/3840 dstr
                100% 2814/2814 elements
                    100% 156/156 async-gen-private-method
                    100% 156/156 async-gen-private-method-static
                    100% 12/12 async-private-method
                    100% 12/12 async-private-method-static
                    100% 12/12 evaluation-error
                    100% 20/20 gen-private-method
                    100% 20/20 gen-private-method-static
                    100% 40/40 private-accessor-name
                    100% 10/10 private-methods
                    100% 494/494 syntax
                        100% 440/440 early-errors
                            100% 192/192 delete
                            100% 56/56 invalid-names
                        100% 54/54 valid
                100% 50/50 gen-method
                100% 50/50 gen-method-static
                100% 30/30 method
                100% 30/30 method-static
                100% 12/12 name-binding
                100% 4/4 strict-mode
                100% 198/198 subclass
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
            100% 267/267 const
                100% 186/186 dstr
                100% 50/50 syntax
            100% 44/44 continue
            100% 4/4 debugger
            100% 70/70 do-while
            100% 4/4 empty
            100% 6/6 expression
            100% 756/756 for
                100% 570/570 dstr
            100% 2425/2425 for-await-of
            100% 196/196 for-in
                100% 49/49 dstr
             99% 1390/1394 for-of
                100% 1081/1081 dstr
            100% 771/771 function
                100% 372/372 dstr
                100% 8/8 early-errors
            100% 503/503 generators
                100% 372/372 dstr
            100% 125/125 if
            100% 35/35 labeled
            100% 283/283 let
                100% 186/186 dstr
                100% 60/60 syntax
            100% 31/31 return
            100% 204/204 switch
                100% 127/127 syntax
                    100% 127/127 redeclaration
            100% 28/28 throw
             99% 375/376 try
                100% 186/186 dstr
            100% 297/297 variable
                100% 186/186 dstr
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

     99% 30737/30823 (106) built-ins
         99% 5287/5301 Array
            100% 8/8 Symbol.species
            100% 86/86 from
            100% 56/56 isArray
            100% 58/58 length
            100% 32/32 of
             99% 4949/4963 prototype
                100% 4/4 Symbol.unscopables
                 97% 131/135 concat
                100% 72/72 copyWithin
                100% 16/16 entries
                100% 421/421 every
                100% 36/36 fill
                 99% 466/468 filter
                100% 32/32 find
                100% 32/32 findIndex
                100% 34/34 flat
                100% 41/41 flatMap
                100% 364/364 forEach
                100% 50/50 includes
                100% 389/389 indexOf
                100% 38/38 join
                100% 16/16 keys
                100% 385/385 lastIndexOf
                 99% 415/417 map
                100% 36/36 pop
                100% 38/38 push
                100% 505/505 reduce
                 99% 503/505 reduceRight
                100% 30/30 reverse
                100% 30/30 shift
                 98% 132/134 slice
                100% 422/422 some
                100% 59/59 sort
                 98% 158/160 splice
                100% 14/14 toLocaleString
                100% 18/18 toString
                100% 32/32 unshift
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
         92% 48/52 AsyncFromSyncIteratorPrototype
             85% 12/14 next
             90% 18/20 return
            100% 18/18 throw
        100% 34/34 AsyncFunction
        100% 44/44 AsyncGeneratorFunction
            100% 12/12 prototype
        100% 90/90 AsyncGeneratorPrototype
            100% 22/22 next
            100% 32/32 return
            100% 32/32 throw
        100% 8/8 AsyncIteratorPrototype
            100% 8/8 Symbol.asyncIterator
        100% 504/504 (106) Atomics
            100% 28/28 add
                100% 6/6 bigint
            100% 28/28 and
                100% 6/6 bigint
            100% 30/30 compareExchange
                100% 6/6 bigint
            100% 30/30 exchange
                100% 6/6 bigint
            100% 12/12 isLockFree
                100% 2/2 bigint
            100% 26/26 load
                100% 6/6 bigint
            100% 92/92 notify
                100% 16/16 bigint
            100% 28/28 or
                100% 6/6 bigint
            100% 30/30 store
                100% 6/6 bigint
            100% 28/28 sub
                100% 6/6 bigint
            100% 138/138 (7) wait
                100% 46/46 (2) bigint
              0% 0/0 (99) waitAsync
                  0% 0/0 (44) bigint
            100% 28/28 xor
                100% 6/6 bigint
        100% 136/136 BigInt
            100% 26/26 asIntN
            100% 26/26 asUintN
            100% 2/2 parseInt
            100% 42/42 prototype
                100% 20/20 toString
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
        100% 1414/1414 Date
            100% 40/40 UTC
            100% 10/10 now
            100% 22/22 parse
            100% 1194/1194 prototype
                100% 36/36 Symbol.toPrimitive
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
                100% 24/24 toJSON
                100% 16/16 toLocaleDateString
                100% 16/16 toLocaleString
                100% 16/16 toLocaleTimeString
                100% 24/24 toString
                100% 20/20 toTimeString
                100% 26/26 toUTCString
                100% 20/20 valueOf
        100% 84/84 Error
            100% 60/60 prototype
                100% 4/4 constructor
                100% 6/6 message
                100% 6/6 name
                100% 26/26 toString
        100% 114/114 FinalizationRegistry
            100% 82/82 prototype
                100% 26/26 cleanupSome
                100% 30/30 register
                100% 18/18 unregister
         99% 883/885 Function
            100% 16/16 internals
                100% 4/4 Call
                100% 12/12 Construct
            100% 26/26 length
             99% 596/598 prototype
                100% 22/22 Symbol.hasInstance
                100% 88/88 apply
                100% 198/198 bind
                100% 92/92 call
                100% 2/2 constructor
                 98% 158/160 toString
        100% 44/44 GeneratorFunction
            100% 12/12 prototype
        100% 114/114 GeneratorPrototype
            100% 26/26 next
            100% 42/42 return
            100% 42/42 throw
        100% 10/10 Infinity
        100% 8/8 IteratorPrototype
            100% 8/8 Symbol.iterator
         99% 278/280 JSON
            100% 138/138 parse
             98% 128/130 stringify
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
        100% 216/216 NativeErrors
            100% 46/46 AggregateError
                100% 12/12 prototype
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
        100% 564/564 Number
            100% 6/6 MAX_VALUE
            100% 6/6 MIN_VALUE
            100% 8/8 NEGATIVE_INFINITY
            100% 8/8 POSITIVE_INFINITY
            100% 14/14 isFinite
            100% 16/16 isInteger
            100% 12/12 isNaN
            100% 18/18 isSafeInteger
            100% 238/238 prototype
                100% 28/28 toExponential
                100% 24/24 toFixed
                100% 6/6 toLocaleString
                100% 30/30 toPrecision
                100% 100/100 toString
                100% 20/20 valueOf
        100% 6280/6280 Object
            100% 54/54 assign
            100% 638/638 create
            100% 1244/1244 defineProperties
            100% 2226/2226 defineProperty
            100% 38/38 entries
            100% 98/98 freeze
            100% 48/48 fromEntries
            100% 618/618 getOwnPropertyDescriptor
            100% 34/34 getOwnPropertyDescriptors
            100% 84/84 getOwnPropertyNames
            100% 18/18 getOwnPropertySymbols
            100% 76/76 getPrototypeOf
            100% 12/12 internals
                100% 12/12 DefineOwnProperty
            100% 40/40 is
            100% 74/74 isExtensible
            100% 116/116 isFrozen
            100% 64/64 isSealed
            100% 114/114 keys
            100% 76/76 preventExtensions
            100% 334/334 prototype
                100% 4/4 constructor
                100% 126/126 hasOwnProperty
                100% 18/18 isPrototypeOf
                100% 32/32 propertyIsEnumerable
                100% 22/22 toLocaleString
                100% 66/66 toString
                100% 40/40 valueOf
            100% 96/96 seal
            100% 22/22 setPrototypeOf
            100% 38/38 values
        100% 1192/1192 Promise
            100% 10/10 Symbol.species
            100% 188/188 all
            100% 198/198 allSettled
            100% 180/180 any
            100% 236/236 prototype
                100% 26/26 catch
                100% 54/54 finally
                100% 144/144 then
            100% 190/190 race
            100% 28/28 reject
            100% 58/58 resolve
        100% 598/598 Proxy
            100% 28/28 apply
            100% 58/58 construct
            100% 48/48 defineProperty
            100% 30/30 deleteProperty
            100% 2/2 enumerate
            100% 38/38 get
            100% 42/42 getOwnPropertyDescriptor
            100% 38/38 getPrototypeOf
            100% 43/43 has
            100% 24/24 isExtensible
            100% 54/54 ownKeys
            100% 23/23 preventExtensions
            100% 30/30 revocable
            100% 52/52 set
            100% 34/34 setPrototypeOf
        100% 278/278 Reflect
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
            100% 24/24 ownKeys
            100% 18/18 preventExtensions
            100% 34/34 set
            100% 26/26 setPrototypeOf
         98% 2884/2926 RegExp
            100% 48/48 CharacterClassEscapes
            100% 8/8 Symbol.species
             50% 4/8 dotall
            100% 34/34 lookBehind
             84% 22/26 match-indices
             96% 50/52 named-groups
            100% 1092/1092 property-escapes
                100% 806/806 generated
             96% 814/846 prototype
                 98% 98/100 Symbol.match
                100% 50/50 Symbol.matchAll
                 90% 120/132 Symbol.replace
                100% 44/44 Symbol.search
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
                100% 16/16 toString
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
         99% 2213/2225 String
            100% 28/28 fromCharCode
            100% 20/20 fromCodePoint
             99% 1928/1938 prototype
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
                100% 86/86 match
                100% 38/38 matchAll
                100% 26/26 normalize
                 91% 22/24 padEnd
                 91% 22/24 padStart
                100% 30/30 repeat
                100% 88/88 replace
                100% 80/80 replaceAll
                100% 72/72 search
                100% 70/70 slice
                100% 226/226 split
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
         97% 162/166 Symbol
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
         99% 2128/2132 TypedArray
            100% 8/8 Symbol.species
            100% 26/26 from
            100% 12/12 of
             99% 2074/2078 prototype
                100% 36/36 Symbol.toStringTag
                    100% 18/18 BigInt
                100% 24/24 buffer
                    100% 4/4 BigInt
                100% 22/22 byteLength
                    100% 4/4 BigInt
                100% 22/22 byteOffset
                    100% 4/4 BigInt
                100% 114/114 copyWithin
                    100% 46/46 BigInt
                100% 26/26 entries
                    100% 6/6 BigInt
                100% 74/74 every
                    100% 30/30 BigInt
                100% 90/90 fill
                    100% 34/34 BigInt
                100% 146/146 filter
                    100% 66/66 BigInt
                100% 58/58 find
                    100% 22/22 BigInt
                100% 58/58 findIndex
                    100% 22/22 BigInt
                100% 70/70 forEach
                    100% 28/28 BigInt
                100% 64/64 includes
                    100% 22/22 BigInt
                100% 64/64 indexOf
                    100% 24/24 BigInt
                100% 46/46 join
                    100% 14/14 BigInt
                100% 26/26 keys
                    100% 6/6 BigInt
                100% 60/60 lastIndexOf
                    100% 22/22 BigInt
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
                100% 196/196 set
                    100% 96/96 BigInt
                100% 156/156 slice
                    100% 70/70 BigInt
                100% 74/74 some
                    100% 30/30 BigInt
                 93% 54/58 sort
                    100% 18/18 BigInt
                100% 122/122 subarray
                    100% 54/54 BigInt
                100% 66/66 toLocaleString
                    100% 26/26 BigInt
                100% 4/4 toString
                    100% 2/2 BigInt
                100% 26/26 values
                    100% 6/6 BigInt
        100% 1362/1362 TypedArrayConstructors
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
            100% 254/254 ctors
                100% 104/104 buffer-arg
                100% 24/24 length-arg
                100% 14/14 no-args
                100% 52/52 object-arg
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

	language/expressions/call/eval-realm-indirect.js (sloppy)

One realm.
		
	language/expressions/compound-assignment/mod-whitespace.js
		
XS optimizes modulus for integer values, which fails for -1 % -1 == -0.

	language/expressions/function/scope-name-var-open-non-strict.js (sloppy)
	language/expressions/generators/scope-name-var-open-non-strict.js (sloppy)

The name of a function expression always defines a constant variable that reference the current function. In sloppy mode it should define a variable that can be assigned but does not change!

	language/expressions/modulus/S11.5.3_A4_T2.js

XS optimizes modulus for integer values, which fails for -1 % -1 == -0.

	language/expressions/object/fn-name-cover.js

In object initializers, if property values are functions, the implementation must rename functions with property names. It happens at runtime for the sake of computed property names. If property values are groups, the implementation should rename functions only if they are the unique expression of their group, XS rename functions if they are the last expression of their group.

	language/expressions/object/literal-property-name-bigint.js
	
XS does not support bigint as property name.

	language/expressions/super/call-proto-not-ctor.js 
	
XS checks if super is a constructor before evaluating arguments.

	language/identifier-resolution/assign-to-global-undefined.js

?		
	
	language/literals/string/legacy-octal-escape-sequence-prologue-strict.js (sloppy)

Strings with octal escape sequences are a lexical error in strict mode but in sloppy mode if "use strict" follows the string, it is too late for a lexical error...

	language/source-text/6.1.js

Code points vs code units.	

	language/statements/for-of/map-expand.js
	language/statements/for-of/set-expand.js

XS `Map` and `Set` entries iterators do not visit entries added after the last one is returned.

	language/statements/try/tco-catch.js (strict)

XS does not tail call optimize `return` inside `catch`

### Built-ins

	built-ins/Array/prototype/concat/Array.prototype.concat_spreadable-string-wrapper.js
	
Code points vs code units.	

	built-ins/Array/prototype/reduceRight/length-near-integer-limit.js

?

	built-ins/Array/prototype/concat/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/filter/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/map/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/slice/create-proto-from-ctor-realm-array.js
	built-ins/Array/prototype/splice/create-proto-from-ctor-realm-array.j	
One realm.
	
	built-ins/AsyncFromSyncIteratorPrototype/next/absent-value-not-passed.js	built-ins/AsyncFromSyncIteratorPrototype/return/absent-value-not-passed.js
	
?

	built-ins/Function/prototype/toString/method-computed-property-name.js

Invalid test.

	built-ins/JSON/stringify/replacer-function-object-deleted-property.js

?

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
	built-ins/RegExp/match-indices/indices-array-non-unicode-match.js
	built-ins/RegExp/match-indices/indices-array-unicode-match.js
	built-ins/RegExp/prototype/Symbol.match/builtin-infer-unicode.js
	built-ins/RegExp/prototype/Symbol.replace/coerce-unicode.js
	built-ins/RegExp/prototype/exec/u-captured-value.js
	built-ins/RegExp/prototype/exec/u-lastindex-value.js
	
Code points vs code units.	

	built-ins/RegExp/named-groups/non-unicode-property-names-valid.js
	built-ins/RegExp/prototype/Symbol.replace/coerce-lastindex.js
	built-ins/RegExp/prototype/Symbol.replace/result-coerce-groups-err.js
	built-ins/RegExp/prototype/Symbol.replace/result-coerce-groups-prop-err.js
	built-ins/RegExp/prototype/Symbol.replace/result-coerce-groups-prop.js
	built-ins/RegExp/prototype/Symbol.replace/result-coerce-groups.js
	
?
			
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
	
### Skipped cases

`xst` skips the *Atomics.waitAsync* cases. There are also skipped cases are in Atomics because the main thread of XS cannot block.

