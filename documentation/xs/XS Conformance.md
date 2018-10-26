# XS Conformance

Copyright 2016-2018 Moddable Tech, Inc.

Revised: March 19, 2018

## Caveat

#### Function Code

XS does not store the source code of functions so `Function.prototype.toString` always fails.

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

The official conformance test suite, [test262](https://github.com/tc39/test262), contains cases for the published specifications, together with cases for proposals at stages 3 and 4, which is great to prepare XS for future editions. The XS harness, `xst` uses adhoc comparisons of the frontmatter `[features]` to skip cases related to not yet implemented proposals. See the skipped proposals at the end of this document.

Currently, on macOS, XS passes **99.8%** of the language tests (`26747/26786`) and **99.4%** of the built-ins tests (`25695/25849`). Details are here under. The numbers of skipped cases are between parentheses. The following section lists the failed tests with some explanations.

### Language

     99% 26747/26786 (556) language
        100% 231/231 arguments-object
            100% 40/40 mapped
            100% 8/8 unmapped
        100% 202/202 asi
        100% 222/222 block-scope
            100% 30/30 leave
            100% 4/4 return-from
            100% 30/30 shadowing
            100% 158/158 syntax
                100% 16/16 for-in
                100% 9/9 function-declarations
                100% 127/127 redeclaration
                100% 6/6 redeclaration-global
        100% 38/38 comments
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
        100% 54/54 directive-prologue
         99% 254/256 eval-code
            100% 140/140 direct
             98% 114/116 indirect
        100% 3/3 export
         99% 11691/11715 (291) expressions
            100% 83/83 (6) addition
            100% 104/104 array
            100% 525/525 arrow-function
                100% 10/10 arrow
                100% 77/77 syntax
                    100% 43/43 early-errors
             99% 572/577 assignment
                100% 6/6 destructuring
            100% 81/81 async-arrow-function
            100% 113/113 async-function
            100% 1154/1154 async-generator
            100% 30/30 await
            100% 47/47 (6) bitwise-and
            100% 28/28 (2) bitwise-not
            100% 47/47 (6) bitwise-or
            100% 47/47 (6) bitwise-xor
             98% 176/179 call
            100% 2892/2892 (160) class
            100% 11/11 comma
            100% 703/703 compound-assignment
            100% 10/10 concatenation
            100% 40/40 conditional
            100% 93/93 delete
            100% 71/71 (6) division
            100% 59/59 (8) does-not-equals
            100% 77/77 (8) equals
            100% 74/74 (7) exponentiation
             99% 438/439 function
             99% 507/508 generators
            100% 81/81 (4) greater-than
            100% 73/73 (4) greater-than-or-equal
            100% 16/16 grouping
            100% 27/27 in
            100% 85/85 instanceof
            100% 77/77 (6) left-shift
            100% 73/73 (4) less-than
            100% 83/83 (4) less-than-or-equal
            100% 34/34 logical-and
            100% 36/36 (1) logical-not
            100% 34/34 logical-or
             96% 63/65 (6) modulus
            100% 67/67 (5) multiplication
            100% 110/110 new
            100% 26/26 new.target
             99% 1723/1725 object
                100% 453/453 method-definition
            100% 62/62 (1) postfix-decrement
            100% 62/62 (1) postfix-increment
            100% 55/55 (1) prefix-decrement
            100% 54/54 (1) prefix-increment
            100% 42/42 property-accessors
            100% 2/2 relational
            100% 61/61 (6) right-shift
            100% 43/43 (8) strict-does-not-equals
            100% 43/43 (8) strict-equals
            100% 65/65 (5) subtraction
             98% 160/162 super
             82% 38/46 tagged-template
            100% 106/106 (1) template-literal
            100% 9/9 this
            100% 28/28 (1) typeof
            100% 24/24 (2) unary-minus
            100% 32/32 (1) unary-plus
            100% 77/77 (6) unsigned-right-shift
            100% 18/18 void
            100% 119/119 yield
         99% 285/287 function-code
        100% 85/85 future-reserved-words
        100% 65/65 global-code
        100% 19/19 identifier-resolution
        100% 283/283 (1) identifiers
        100% 4/4 import
        100% 50/50 keywords
        100% 118/118 line-terminators
         99% 483/484 (61) literals
              0% 0/0 (6) bigint
            100% 4/4 boolean
            100% 4/4 null
            100% 172/172 (52) numeric
            100% 178/178 (1) regexp
             99% 125/126 (2) string
        100% 273/273 (9) module-code
            100% 34/34 namespace
                100% 32/32 internals
        100% 22/22 punctuators
        100% 53/53 reserved-words
        100% 22/22 rest-parameters
          0% 0/2 source-text
         99% 11865/11873 (194) statements
            100% 107/107 async-function
            100% 552/552 async-generator
            100% 30/30 block
            100% 38/38 break
             99% 3304/3306 (194) class
                100% 4/4 arguments
                100% 122/122 definition
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
                100% 14/14 super
                100% 58/58 syntax
                    100% 36/36 early-errors
            100% 261/261 const
                100% 50/50 syntax
            100% 44/44 continue
            100% 4/4 debugger
            100% 70/70 do-while
            100% 4/4 empty
            100% 6/6 expression
            100% 734/734 for
            100% 2389/2389 for-await-of
            100% 172/172 for-in
             99% 1326/1330 for-of
            100% 745/745 function
            100% 482/482 generators
            100% 123/123 if
            100% 35/35 labeled
            100% 281/281 let
                100% 64/64 syntax
            100% 31/31 return
            100% 204/204 switch
                100% 127/127 syntax
                    100% 127/127 redeclaration
            100% 28/28 throw
             99% 369/370 try
            100% 282/282 variable
            100% 72/72 while
             99% 172/173 with
        100% 211/211 types
            100% 10/10 boolean
            100% 6/6 list
            100% 8/8 null
            100% 39/39 number
            100% 36/36 object
            100% 49/49 reference
            100% 48/48 string
            100% 15/15 undefined
        100% 94/94 white-space

### Built-ins

     99% 25695/25849 (1342) built-ins
         99% 5106/5118 (22) Array
            100% 8/8 Symbol.species
            100% 80/80 from
            100% 56/56 isArray
            100% 50/50 length
            100% 30/30 of
             99% 4790/4802 (22) prototype
                100% 4/4 Symbol.unscopables
                 96% 113/117 concat
                100% 66/66 copyWithin
                100% 16/16 entries
                100% 421/421 every
                100% 34/34 fill
                 99% 460/462 filter
                100% 32/32 find
                100% 32/32 findIndex
                  0% 0/0 (9) flatMap
                  0% 0/0 (13) flatten
                100% 364/364 forEach
                100% 50/50 includes
                100% 383/383 indexOf
                100% 38/38 join
                100% 16/16 keys
                100% 379/379 lastIndexOf
                 99% 409/411 map
                100% 34/34 pop
                100% 34/34 push
                100% 505/505 reduce
                100% 503/503 reduceRight
                100% 30/30 reverse
                100% 28/28 shift
                 98% 126/128 slice
                100% 422/422 some
                100% 51/51 sort
                 98% 150/152 splice
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
        100% 30/30 AsyncFunction
        100% 38/38 AsyncGeneratorFunction
            100% 10/10 prototype
        100% 90/90 AsyncGeneratorPrototype
            100% 22/22 next
            100% 32/32 return
            100% 32/32 throw
        100% 8/8 AsyncIteratorPrototype
            100% 8/8 Symbol.asyncIterator
        100% 252/252 Atomics
            100% 16/16 add
            100% 16/16 and
            100% 16/16 compareExchange
            100% 16/16 exchange
            100% 10/10 isLockFree
            100% 16/16 load
            100% 16/16 or
            100% 16/16 store
            100% 16/16 sub
            100% 58/58 wait
            100% 34/34 wake
            100% 16/16 xor
          0% 0/0 (65) BigInt
              0% 0/0 (13) asIntN
              0% 0/0 (13) asUintN
              0% 0/0 (1) parseInt
              0% 0/0 (18) prototype
                  0% 0/0 (8) toString
                  0% 0/0 (6) valueOf
        100% 96/96 Boolean
            100% 50/50 prototype
                100% 2/2 constructor
                100% 18/18 toString
                100% 18/18 valueOf
        100% 790/790 (59) DataView
            100% 680/680 (59) prototype
                100% 22/22 buffer
                100% 22/22 byteLength
                100% 22/22 byteOffset
                  0% 0/0 (19) getBigInt64
                  0% 0/0 (19) getBigUint64
                100% 38/38 getFloat32
                100% 38/38 getFloat64
                100% 32/32 getInt16
                100% 52/52 getInt32
                100% 30/30 getInt8
                100% 32/32 getUint16
                100% 32/32 getUint32
                100% 30/30 getUint8
                  0% 0/0 (21) setBigInt64
                100% 42/42 setFloat32
                100% 42/42 setFloat64
                100% 42/42 setInt16
                100% 42/42 setInt32
                100% 38/38 setInt8
                100% 42/42 setUint16
                100% 42/42 setUint32
                100% 38/38 setUint8
        100% 1378/1378 Date
            100% 40/40 UTC
            100% 10/10 now
            100% 16/16 parse
            100% 1166/1166 prototype
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
                100% 20/20 toDateString
                100% 32/32 toISOString
                100% 6/6 toJSON
                100% 16/16 toLocaleDateString
                100% 16/16 toLocaleString
                100% 16/16 toLocaleTimeString
                100% 22/22 toString
                100% 20/20 toTimeString
                100% 24/24 toUTCString
                100% 20/20 valueOf
        100% 74/74 Error
            100% 52/52 prototype
                100% 4/4 constructor
                100% 6/6 message
                100% 6/6 name
                100% 20/20 toString
         87% 755/867 Function
            100% 16/16 internals
                100% 4/4 Call
                100% 12/12 Construct
            100% 26/26 length
             80% 476/588 prototype
                100% 22/22 Symbol.hasInstance
                100% 96/96 apply
                100% 208/208 bind
                100% 92/92 call
                100% 2/2 constructor
                 16% 22/134 toString
        100% 40/40 GeneratorFunction
            100% 10/10 prototype
        100% 114/114 GeneratorPrototype
            100% 26/26 next
            100% 42/42 return
            100% 42/42 throw
        100% 12/12 Infinity
        100% 8/8 IteratorPrototype
            100% 8/8 Symbol.iterator
        100% 242/242 (4) JSON
            100% 116/116 parse
            100% 116/116 (4) stringify
        100% 285/285 Map
            100% 8/8 Symbol.species
            100% 223/223 prototype
                100% 20/20 clear
                100% 20/20 delete
                100% 18/18 entries
                100% 33/33 forEach
                100% 20/20 get
                100% 20/20 has
                100% 18/18 keys
                100% 26/26 set
                100% 22/22 size
                100% 18/18 values
        100% 22/22 MapIteratorPrototype
            100% 20/20 next
        100% 544/544 Math
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
            100% 14/14 fround
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
        100% 12/12 NaN
        100% 146/146 NativeErrors
            100% 24/24 EvalError
                100% 10/10 prototype
            100% 24/24 RangeError
                100% 10/10 prototype
            100% 24/24 ReferenceError
                100% 10/10 prototype
            100% 24/24 SyntaxError
                100% 10/10 prototype
            100% 24/24 TypeError
                100% 10/10 prototype
            100% 24/24 URIError
                100% 10/10 prototype
        100% 500/500 (31) Number
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
        100% 6132/6132 (3) Object
            100% 50/50 assign
            100% 630/630 create
            100% 1242/1242 defineProperties
            100% 2224/2224 defineProperty
            100% 34/34 entries
            100% 92/92 freeze
            100% 614/614 getOwnPropertyDescriptor
            100% 30/30 getOwnPropertyDescriptors
            100% 74/74 getOwnPropertyNames
            100% 8/8 getOwnPropertySymbols
            100% 76/76 getPrototypeOf
            100% 12/12 internals
                100% 12/12 DefineOwnProperty
            100% 40/40 is
            100% 74/74 isExtensible
            100% 114/114 isFrozen
            100% 62/62 isSealed
            100% 102/102 keys
            100% 72/72 preventExtensions
            100% 326/326 (1) prototype
                100% 4/4 constructor
                100% 130/130 hasOwnProperty
                100% 20/20 isPrototypeOf
                100% 32/32 propertyIsEnumerable
                100% 22/22 toLocaleString
                100% 52/52 (1) toString
                100% 40/40 valueOf
            100% 90/90 seal
            100% 20/20 (1) setPrototypeOf
            100% 34/34 values
        100% 668/668 Promise
            100% 10/10 Symbol.species
            100% 132/132 all
            100% 228/228 prototype
                100% 26/26 catch
                100% 46/46 finally
                100% 144/144 then
            100% 110/110 race
            100% 28/28 reject
            100% 58/58 resolve
        100% 467/467 Proxy
            100% 20/20 apply
            100% 34/34 construct
            100% 40/40 defineProperty
            100% 22/22 deleteProperty
            100% 2/2 enumerate
            100% 32/32 get
            100% 34/34 getOwnPropertyDescriptor
            100% 28/28 getPrototypeOf
            100% 33/33 has
            100% 18/18 isExtensible
            100% 48/48 ownKeys
            100% 18/18 preventExtensions
            100% 22/22 revocable
            100% 38/38 set
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
            100% 20/20 ownKeys
            100% 18/18 preventExtensions
            100% 34/34 set
            100% 26/26 setPrototypeOf
         99% 1686/1698 (381) RegExp
            100% 8/8 Symbol.species
             50% 4/8 dotall
            100% 34/34 lookBehind
            100% 48/48 named-groups
              0% 0/0 (381) property-escapes
                  0% 0/0 (372) generated
             98% 740/748 prototype
                 98% 98/100 Symbol.match
                 98% 102/104 Symbol.replace
                100% 40/40 Symbol.search
                100% 86/86 Symbol.split
                100% 14/14 dotAll
                 97% 144/148 exec
                100% 30/30 flags
                100% 18/18 global
                100% 18/18 ignoreCase
                100% 18/18 multiline
                100% 22/22 source
                100% 14/14 sticky
                100% 88/88 test
                100% 14/14 toString
                100% 14/14 unicode
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
         99% 1997/2009 (46) String
            100% 28/28 fromCharCode
            100% 20/20 fromCodePoint
             99% 1714/1724 (46) prototype
                100% 10/10 Symbol.iterator
                100% 58/58 charAt
                100% 48/48 charCodeAt
                 93% 28/30 codePointAt
                100% 42/42 concat
                100% 4/4 constructor
                100% 52/52 endsWith
                100% 52/52 includes
                100% 88/88 (2) indexOf
                100% 46/46 lastIndexOf
                100% 24/24 localeCompare
                100% 84/84 match
                100% 26/26 normalize
                 91% 22/24 padEnd
                 91% 22/24 padStart
                100% 30/30 repeat
                100% 90/90 replace
                100% 70/70 search
                100% 70/70 slice
                100% 214/214 split
                100% 40/40 startsWith
                100% 86/86 substring
                 96% 52/54 toLocaleLowerCase
                100% 50/50 toLocaleUpperCase
                 96% 52/54 toLowerCase
                100% 18/18 toString
                100% 50/50 toUpperCase
                100% 256/256 trim
                  0% 0/0 (22) trimEnd
                  0% 0/0 (22) trimStart
                100% 16/16 valueOf
            100% 58/58 raw
         85% 12/14 StringIteratorPrototype
             80% 8/10 next
         97% 136/140 Symbol
             85% 12/14 for
            100% 4/4 hasInstance
            100% 4/4 isConcatSpreadable
            100% 4/4 iterator
             85% 12/14 keyFor
            100% 4/4 match
            100% 46/46 prototype
                100% 14/14 Symbol.toPrimitive
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
        100% 1250/1250 (404) TypedArray
            100% 8/8 Symbol.species
            100% 26/26 from
            100% 12/12 of
            100% 1196/1196 (404) prototype
                100% 18/18 (9) Symbol.toStringTag
                      0% 0/0 (9) BigInt
                100% 18/18 (2) buffer
                      0% 0/0 (2) BigInt
                100% 18/18 (2) byteLength
                      0% 0/0 (2) BigInt
                100% 18/18 (2) byteOffset
                      0% 0/0 (2) BigInt
                100% 62/62 (23) copyWithin
                      0% 0/0 (23) BigInt
                100% 20/20 (3) entries
                      0% 0/0 (3) BigInt
                100% 44/44 (15) every
                      0% 0/0 (15) BigInt
                100% 50/50 (17) fill
                      0% 0/0 (17) BigInt
                100% 80/80 (33) filter
                      0% 0/0 (33) BigInt
                100% 36/36 (12) find
                      0% 0/0 (12) BigInt
                100% 36/36 (12) findIndex
                      0% 0/0 (12) BigInt
                100% 42/42 (14) forEach
                      0% 0/0 (14) BigInt
                100% 38/38 (11) includes
                      0% 0/0 (11) BigInt
                100% 38/38 (11) indexOf
                      0% 0/0 (11) BigInt
                100% 32/32 (7) join
                      0% 0/0 (7) BigInt
                100% 20/20 (3) keys
                      0% 0/0 (3) BigInt
                100% 36/36 (10) lastIndexOf
                      0% 0/0 (10) BigInt
                100% 18/18 (9) length
                      0% 0/0 (9) BigInt
                100% 52/52 (17) map
                      0% 0/0 (17) BigInt
                100% 50/50 (18) reduce
                      0% 0/0 (18) BigInt
                100% 50/50 (18) reduceRight
                      0% 0/0 (18) BigInt
                100% 24/24 (5) reverse
                      0% 0/0 (5) BigInt
                100% 96/96 (48) set
                      0% 0/0 (47) BigInt
                100% 86/86 (35) slice
                      0% 0/0 (35) BigInt
                100% 44/44 (15) some
                      0% 0/0 (15) BigInt
                100% 34/34 (9) sort
                      0% 0/0 (9) BigInt
                100% 68/68 (27) subarray
                      0% 0/0 (27) BigInt
                100% 40/40 (13) toLocaleString
                      0% 0/0 (13) BigInt
                100% 2/2 (1) toString
                      0% 0/0 (1) BigInt
                100% 20/20 (3) values
                      0% 0/0 (3) BigInt
        100% 739/739 (312) TypedArrayConstructors
              0% 0/0 (11) BigInt64Array
                  0% 0/0 (4) prototype
              0% 0/0 (11) BigUint64Array
                  0% 0/0 (4) prototype
            100% 18/18 (1) Float32Array
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
            100% 250/250 (1) ctors
                100% 104/104 buffer-arg
                100% 24/24 length-arg
                100% 14/14 no-args
                100% 50/50 object-arg
                100% 58/58 (1) typedarray-arg
              0% 0/0 (129) ctors-bigint
                  0% 0/0 (52) buffer-arg
                  0% 0/0 (12) length-arg
                  0% 0/0 (7) no-args
                  0% 0/0 (31) object-arg
                  0% 0/0 (27) typedarray-arg
            100% 58/58 (28) from
                  0% 0/0 (28) BigInt
            100% 165/165 (89) internals
                100% 44/44 (20) DefineOwnProperty
                      0% 0/0 (20) BigInt
                100% 28/28 (14) Get
                      0% 0/0 (14) BigInt
                100% 24/24 (12) GetOwnProperty
                      0% 0/0 (12) BigInt
                100% 29/29 (15) HasProperty
                      0% 0/0 (15) BigInt
                100% 8/8 (4) OwnPropertyKeys
                      0% 0/0 (4) BigInt
                100% 32/32 (24) Set
                      0% 0/0 (23) BigInt
            100% 28/28 (12) of
                  0% 0/0 (12) BigInt
            100% 60/60 (30) prototype
                100% 2/2 (1) Symbol.toStringTag
                100% 2/2 (1) buffer
                100% 2/2 (1) byteLength
                100% 2/2 (1) byteOffset
                100% 2/2 (1) copyWithin
                100% 2/2 (1) entries
                100% 2/2 (1) every
                100% 2/2 (1) fill
                100% 2/2 (1) filter
                100% 2/2 (1) find
                100% 2/2 (1) findIndex
                100% 2/2 (1) forEach
                100% 2/2 (1) indexOf
                100% 2/2 (1) join
                100% 2/2 (1) keys
                100% 2/2 (1) lastIndexOf
                100% 2/2 (1) length
                100% 2/2 (1) map
                100% 2/2 (1) reduce
                100% 2/2 (1) reduceRight
                100% 2/2 (1) reverse
                100% 2/2 (1) set
                100% 2/2 (1) slice
                100% 2/2 (1) some
                100% 2/2 (1) sort
                100% 2/2 (1) subarray
                100% 2/2 (1) toLocaleString
                100% 2/2 (1) toString
                100% 2/2 (1) values
          0% 0/0 TypedArrays
        100% 176/176 WeakMap
            100% 132/132 prototype
                100% 36/36 delete
                100% 20/20 get
                100% 34/34 has
                100% 36/36 set
        100% 150/150 WeakSet
            100% 114/114 prototype
                100% 36/36 add
                100% 4/4 constructor
                100% 36/36 delete
                100% 34/34 has
        100% 106/106 decodeURI
        100% 106/106 decodeURIComponent
        100% 58/58 encodeURI
        100% 58/58 encodeURIComponent
        100% 16/16 eval
        100% 58/58 global
        100% 32/32 isFinite
        100% 32/32 isNaN
        100% 84/84 (15) parseFloat
        100% 118/118 parseInt
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
	language/expressions/tagged-template/cache-same-site-top-level.js
	language/expressions/tagged-template/cache-same-site.js

No tagged templates cache.

	language/function-code/10.4.3-1-104.js (strict)
	language/function-code/10.4.3-1-106.js (strict)
	
When calling a member, XS promotes primitives into objects to lookup the property then uses the object instead of the primitive as `this`.

	language/literals/string/7.8.4-1-s.js (sloppy)

Strings with octal escape sequences are a lexical error in strict mode but in sloppy mode if "use strict" follows the string, it is too late for a lexical error...

	language/source-text/6.1.js

Code points vs code units.	

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
	
	built-ins/Function/prototype/toString/*	
No source code.	
	
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

### Annex B

	annexB/built-ins/String/prototype/substr/surrogate-pairs.js
	
Code points vs code units.	
	
### Skipped proposals

	Array.prototype.flatMap
	BigInt
	String.prototype.trimEnd
	String.prototype.trimStart
	class-fields-private
	class-fields-public
	numeric-separator-literal
	regexp-unicode-property-escapes
	string-trimming

