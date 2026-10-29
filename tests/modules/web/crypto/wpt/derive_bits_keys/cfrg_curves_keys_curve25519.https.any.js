/*---
flags: [async,onlyStrict]
---*/
// Imported from web-platform-tests WebCryptoAPI/derive_bits_keys/cfrg_curves_keys_curve25519.https.any.js (672f0e8 2026-10-17); lines marked MODDABLE are local changes
// ---- inlined WebCryptoAPI/derive_bits_keys/cfrg_curves_bits_fixtures.js ----
var pkcs8 = {
    "X25519": new Uint8Array([48, 46, 2, 1, 0, 48, 5, 6, 3, 43, 101, 110, 4, 34, 4, 32, 200, 131, 142, 118, 208, 87, 223, 183, 216, 201, 90, 105, 225, 56, 22, 10, 221, 99, 115, 253, 113, 164, 210, 118, 187, 86, 227, 168, 27, 100, 255, 97]),
    "X448": new Uint8Array([48, 70, 2, 1, 0, 48, 5, 6, 3, 43, 101, 111, 4, 58, 4, 56, 88, 199, 210, 154, 62, 181, 25, 178, 157, 0, 207, 177, 145, 187, 100, 252, 109, 138, 66, 216, 241, 113, 118, 39, 43, 137, 242, 39, 45, 24, 25, 41, 92, 101, 37, 192, 130, 150, 113, 176, 82, 239, 7, 39, 83, 15, 24, 142, 49, 208, 204, 83, 191, 38, 146, 158])
};

var spki = {
    "X25519": new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 28, 242, 177, 230, 2, 46, 197, 55, 55, 30, 215, 245, 62, 84, 250, 17, 84, 216, 62, 152, 235, 100, 234, 81, 250, 229, 179, 48, 124, 254, 151, 6]),
    "X448": new Uint8Array([48, 66, 48, 5, 6, 3, 43, 101, 111, 3, 57, 0, 182, 4, 161, 209, 165, 205, 29, 148, 38, 213, 97, 239, 99, 10, 158, 177, 108, 190, 105, 213, 185, 202, 97, 94, 220, 83, 99, 62, 251, 82, 234, 49, 230, 230, 160, 161, 219, 172, 198, 231, 108, 188, 230, 72, 45, 126, 75, 163, 213, 93, 158, 128, 39, 101, 206, 111])
};

var sizes = {
    "X25519": 32,
    "X448": 56
};

var derivations = {
    "X25519": new Uint8Array([39, 104, 64, 157, 250, 185, 158, 194, 59, 140, 137, 185, 63, 245, 136, 2, 149, 247, 97, 118, 8, 143, 137, 228, 61, 254, 190, 126, 161, 149, 0, 8]),
    "X448": new Uint8Array([240, 246, 197, 241, 127, 148, 244, 41, 30, 171, 113, 120, 134, 109, 55, 236, 137, 6, 221, 108, 81, 65, 67, 220, 133, 190, 124, 242, 141, 239, 243, 155, 114, 110, 15, 109, 207, 129, 14, 181, 148, 220, 169, 123, 72, 130, 189, 68, 196, 62, 167, 220, 103, 244, 154, 78])
};

var kSmallOrderPoint = {
    "X25519": [
        { order: "0",                vector : new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]) },
        { order: "1",                vector : new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]) },
        { order: "8",                vector : new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 224, 235, 122, 124, 59, 65, 184, 174, 22, 86, 227, 250, 241, 159, 196, 106, 218, 9, 141, 235, 156, 50, 177, 253, 134, 98, 5, 22, 95, 73, 184, 0]) },
        { order: "p-1 (order 2)",    vector : new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 127]) },
        { order: "p (=0, order 4)",  vector : new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 237, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 127]) },
        { order: "p+1 (=1, order 1)", vector : new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 238, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 127]) },
    ],
    "X448": [
        { order: "0",                 vector : new Uint8Array([48, 66, 48, 5, 6, 3, 43, 101, 111, 3, 57, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]) },
        { order: "1",                 vector : new Uint8Array([48, 66, 48, 5, 6, 3, 43, 101, 111, 3, 57, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]) },
        { order: "p-1 (order 2)",     vector : new Uint8Array([48, 66, 48, 5, 6, 3, 43, 101, 111, 3, 57, 0, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]) },
        { order: "p (=0, order 4)",   vector : new Uint8Array([48, 66, 48, 5, 6, 3, 43, 101, 111, 3, 57, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]) },
        { order: "p+1 (=1, order 1)", vector : new Uint8Array([48, 66, 48, 5, 6, 3, 43, 101, 111, 3, 57, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255]) },
    ]
};

// "P-256":
var ecSPKI = new Uint8Array([48, 89, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 3, 66, 0, 4, 154, 116, 32, 120, 126, 95, 77, 105, 211, 232, 34, 114, 115, 1, 109, 56, 224, 71, 129, 133, 223, 127, 238, 156, 142, 103, 60, 202, 211, 79, 126, 128, 254, 49, 141, 182, 221, 107, 119, 218, 99, 32, 165, 246, 151, 89, 9, 68, 23, 177, 52, 239, 138, 139, 116, 193, 101, 4, 57, 198, 115, 0, 90, 61]);

// ---- end derive_bits_keys/cfrg_curves_bits_fixtures.js ----
// ---- inlined WebCryptoAPI/derive_bits_keys/derive.js ----
function registerDeriveTests(options) {
    const {
        operation,
        algorithmName,
        testName = algorithmName,
        mixedCaseName,
        size,
        derivation,
        privateKey,
        publicKey,
        noUsagePrivateKey,
        invalidPublicKeys,
        missingPublicLabel,
    } = options;
    const subtle = self.crypto.subtle;
    const isDeriveBits = operation === "deriveBits";
    const derivedKeyAlgorithm = {name: "HMAC", hash: "SHA-256", length: 256};
    const derivedKeyUsages = ["sign", "verify"];

    function derive(name, publicKey, baseKey, length = 8 * size, keyOptions = {}) {
        const algorithm = {name, public: publicKey};
        if (isDeriveBits) {
            return subtle.deriveBits(algorithm, baseKey, length);
        }

        return subtle.deriveKey(
            algorithm,
            baseKey,
            keyOptions.algorithm || derivedKeyAlgorithm,
            true,
            keyOptions.usages || derivedKeyUsages
        ).then(key => subtle.exportKey("raw", key));
    }

    function assertDerived(result, length) {
        if (isDeriveBits) {
            assert_true(equalBuffers(result, derivation, length), "Derived correct bits");
        } else {
            assert_array_equals(
                new Uint8Array(result),
                derivation.slice(0, 32),
                "Derived correct key"
            );
        }
    }

    function successTest(name, algorithm, length = 8 * size) {
        promise_test(() => {
            return derive(algorithm, publicKey, privateKey, length).then(
                result => assertDerived(result, length),
                error => assert_unreached(
                    operation + " failed with error " + error.name + ": " + error.message
                )
            );
        }, name);
    }

    function failureTest(name, expectedError, deriveOperation) {
        promise_test(() => {
            return Promise.resolve().then(deriveOperation).then(
                () => assert_unreached(
                    operation + " succeeded but should have failed with " + expectedError
                ),
                error => assert_equals(
                    error.name,
                    expectedError,
                    "Should throw correct error, not " + error.name + ": " + error.message
                )
            );
        }, name);
    }

    successTest(testName + " good parameters", algorithmName);
    successTest(testName + " mixed case parameters", mixedCaseName);

    if (isDeriveBits) {
        successTest(testName + " short result", algorithmName, 8 * size - 32);
        successTest(
            testName + " non-multiple of 8 bits",
            algorithmName,
            8 * size - 11
        );
    }

    failureTest(missingPublicLabel, "TypeError", () => {
        return subtle[operation](
            {name: algorithmName},
            privateKey,
            ...(isDeriveBits
                ? [8 * size]
                : [derivedKeyAlgorithm, true, derivedKeyUsages])
        );
    });

    failureTest(
        testName + " public property of algorithm is not a CryptoKey",
        "TypeError",
        () => derive(algorithmName, {message: "Not a CryptoKey"}, privateKey)
    );

    invalidPublicKeys.forEach(test => {
        failureTest(
            test.name,
            "InvalidAccessError",
            () => derive(algorithmName, test.key, privateKey, test.length)
        );
    });

    failureTest(
        testName + " no " + operation + " usage for base key",
        "InvalidAccessError",
        () => derive(algorithmName, publicKey, noUsagePrivateKey)
    );

    failureTest(
        testName + " base key is not a private key",
        "InvalidAccessError",
        () => derive(algorithmName, publicKey, publicKey)
    );

    failureTest(
        testName + " public property value is a private key",
        "InvalidAccessError",
        () => derive(algorithmName, privateKey, privateKey)
    );

    promise_test(async() => {
        const secretKey = isDeriveBits
            ? await subtle.generateKey(
                {name: "AES-CBC", length: 128},
                true,
                ["encrypt", "decrypt"]
            )
            : await subtle.generateKey(
                derivedKeyAlgorithm,
                true,
                derivedKeyUsages
            );
        const keyOptions = isDeriveBits ? {} : {
            algorithm: {name: "AES-CBC", length: 256},
            usages: ["sign", "verify"],
        };

        return derive(
            algorithmName,
            secretKey,
            privateKey,
            8 * size,
            keyOptions
        ).then(
            () => assert_unreached(
                operation + " succeeded but should have failed with InvalidAccessError"
            ),
            error => assert_equals(
                error.name,
                "InvalidAccessError",
                "Should throw correct error, not " + error.name + ": " + error.message
            )
        );
    }, testName + " public property value is a secret key");

    if (isDeriveBits) {
        failureTest(
            testName + " asking for too many bits",
            "OperationError",
            () => derive(algorithmName, publicKey, privateKey, 8 * size + 8)
        );
    }
}

// ---- end derive_bits_keys/derive.js ----
// ---- inlined WebCryptoAPI/derive_bits_keys/cfrg_curves.js ----
async function defineCfrgTests(algorithmName, operation) {
    const subtle = self.crypto.subtle;
    const isDeriveBits = operation === "deriveBits";

    kSmallOrderPoint[algorithmName].forEach(test => {
        promise_test(async() => {
            let privateKey;
            let publicKey;
            let derived;
            let error;

            try {
                privateKey = await subtle.importKey(
                    "pkcs8",
                    pkcs8[algorithmName],
                    {name: algorithmName},
                    false,
                    ["deriveBits", "deriveKey"]
                );
                publicKey = await subtle.importKey(
                    "spki",
                    test.vector,
                    {name: algorithmName},
                    false,
                    []
                );
                derived = isDeriveBits
                    ? await subtle.deriveBits(
                        {name: algorithmName, public: publicKey},
                        privateKey,
                        8 * sizes[algorithmName]
                    )
                    : await subtle.deriveKey(
                        {name: algorithmName, public: publicKey},
                        privateKey,
                        {name: "HMAC", hash: "SHA-256", length: 256},
                        true,
                        ["sign", "verify"]
                    );
            } catch (caught) {
                error = caught;
            }

            assert_not_equals(privateKey, undefined, "Private key should be valid.");
            assert_not_equals(publicKey, undefined, "Public key should be valid.");
            assert_not_equals(error, undefined, "Operation should fail.");
            assert_equals(
                error.name,
                "OperationError",
                "Should throw correct error, not " + error.name + ": " + error.message + "."
            );
            assert_equals(derived, undefined, "Operation succeeded, but should not have.");
        }, algorithmName +
            (isDeriveBits ? " key derivation" : " deriveBits") +
            " checks for all-zero value result with a key of order " + test.order);
    });

    if (!isDeriveBits) {
        promise_test(async() => {
            const key = await subtle.generateKey(
                {name: algorithmName},
                true,
                ["deriveKey", "deriveBits"]
            );
            const derived = await subtle.deriveKey(
                {name: algorithmName, public: key.publicKey},
                key.privateKey,
                {name: "HMAC", hash: "SHA-256", length: 256},
                true,
                ["sign", "verify"]
            );
            assert_not_equals(derived, undefined, "Key derivation failed.");
        }, "Key derivation using a " + algorithmName + " generated keys.");
    }

    const noUsage = isDeriveBits ? ["deriveKey"] : ["deriveBits"];
    const [
        privateKey,
        noUsagePrivateKey,
        publicKey,
        ecdhPublicKey,
    ] = await Promise.all([
        subtle.importKey(
            "pkcs8",
            pkcs8[algorithmName],
            {name: algorithmName},
            false,
            ["deriveBits", "deriveKey"]
        ),
        subtle.importKey(
            "pkcs8",
            pkcs8[algorithmName],
            {name: algorithmName},
            false,
            noUsage
        ),
        subtle.importKey(
            "spki",
            spki[algorithmName],
            {name: algorithmName},
            false,
            []
        ),
        subtle.importKey(
            "spki",
            ecSPKI,
            {name: "ECDH", namedCurve: "P-256"},
            false,
            []
        ),
    ]);

    registerDeriveTests({
        operation,
        algorithmName,
        mixedCaseName: algorithmName.toLowerCase(),
        size: sizes[algorithmName],
        derivation: derivations[algorithmName],
        privateKey,
        publicKey,
        noUsagePrivateKey,
        invalidPublicKeys: [{
            name: algorithmName + " mismatched algorithms",
            key: ecdhPublicKey,
        }],
        missingPublicLabel: algorithmName + " missing public property",
    });
}

// ---- end derive_bits_keys/cfrg_curves.js ----
// ---- inlined WebCryptoAPI/derive_bits_keys/cfrg_curves_keys.js ----
function define_tests_25519() {
    return defineCfrgTests("X25519", "deriveKey");
}

function define_tests_448() {
    return defineCfrgTests("X448", "deriveKey");
}

// ---- end derive_bits_keys/cfrg_curves_keys.js ----
// META: title=WebCryptoAPI: deriveKey() Using ECDH with CFRG Elliptic Curves
// META: script=../util/helpers.js
// META: script=cfrg_curves_bits_fixtures.js
// META: script=derive.js
// META: script=cfrg_curves.js
// META: script=cfrg_curves_keys.js

// Define subtests from a `promise_test` to ensure the harness does not
// complete before the subtests are available. `explicit_done` cannot be used
// for this purpose because the global `done` function is automatically invoked
// by the WPT infrastructure in dedicated worker tests defined using the
// "multi-global" pattern.
promise_test(define_tests_25519, 'setup - define tests');
