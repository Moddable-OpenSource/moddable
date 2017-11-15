# Crypt
Copyright 2017 Moddable Tech, Inc.

Revised: November 7, 2017

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.


## class Digest (Crypt)

The Digest class creates cryptographic hashes using a variety of algorithms.

	import {Digest} from "crypt";

### MD5 Hash

	let digest = new Digest("MD5");
	digest.write("hello, world);
	trace(`MD5 Hash: ${digest.close()}\n`);

### SHA1 Hash

	let digest = new Digest("SHA1");
	digest.write("hello,");
	digest.write(" world");
	trace(`SHA1 Hash: ${digest.close()}\n`);

### new Digest(type)

The `Digest` constructor takes the type of the hash to calculate as its sole argument.

	let digest = new Digest("SHA1");

The following hash functions are supported:

* MD5
* SHA1
* SHA224
* SHA256
* SHA384
* SHA512

### write(message)

The `write` function adds a message to the hash being calculated. There is no restriction on the length of the message. The message argument to write may be a `String` or `ArrayBuffer`. The `write` function may be called more than once for a given digest calculation.

	digest.write("123");
	digest.write("456");

### close()

The `close` function returns the the calculated hash. The hash is returned as an `ArrayBuffer`. The `close` function may only be called once, as it frees all resources associated with the digest calculation.

<!-- 11/7/2017 BSF
We should probably document the reset function here with an example.
-->

<!-- 11/7/2017 BSF
There are also blockSize and outputSize accessor/getter functions in addition to process and update helper functions. Should those be documented here?
-->

<!-- 11/7/2017 BSF
BlockCipher, StreamCipher and Mode classes need to be documented. They are used in the cryptblockcipher example app.
-->
