# Crypt
Copyright 2017-2022 Moddable Tech, Inc.<BR>
Revised: June 14, 2022


<a id="digest"></a>
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

<a id="transform"></a>
## class Transform

The `Transform` class contains static methods to perform common transformations of certificate data.

	import Transform from "crypt/transform";

Whenever possible, transformation of data should not be performed at runtime because it uses additional time and memory. Instead, the data should be stored in the optimal format for the target device. These transformation functions are provided for situations where the transformation must be performed at runtime, such as some device provisioning flows.

Add the `Transform` module in a manifest to use it in a project.

```json
"modules": {
	"crypt/transform": "$(MODULES)/crypt/etc/transform"
}
```

<a id="transform-pemToDER"></a>
### static pemToDER(data)

The `pemToDER` function transforms a certificate in PEM format (Base64 encoded ASCII) to [DER](https://en.wikipedia.org/wiki/X.690#DER_encoding) format (binary data). The input `data` may be a `String`, `ArrayBuffer`, or host buffer. The return value is an `ArrayBuffer`.

For PEM files containing more than one certificate, `pemToDER` converts only the first certificate.

This function is similar to the following `openssl` command line: 

```
openssl x509 -inform pem -in data.pem -out data.der -outform der
```

`pemToDER` looks for both `-----BEGIN CERTIFICATE-----` and `-----BEGIN RSA PRIVATE KEY-----` as delimeters.

<a id="transform-privateKeyToPrivateKeyInfo"></a>
### static privateKeyToPrivateKeyInfo(data[, oid])

The `privateKeyToPrivateKeyInfo ` function transforms a Private Key to a Private Key Info (both in binary DER format). The input `data` is an `ArrayBuffer` or host buffer. The optional `oid` parameter is the Object ID for the key algorithm as an `Array`. If not provided, it defaults to the OID for `PKCS#1`, `[1, 2, 840, 113549, 1, 1, 1]`. The return value is an `ArrayBuffer`.

Using `pemToDER` and `privateKeyToPrivateKeyInfo ` together is similar to the following `openssl` command line:

```
openssl pkcs8 -topk8 -in private_key.pem -inform pem -out private_key.pk8.der -outform der -nocrypt
```
