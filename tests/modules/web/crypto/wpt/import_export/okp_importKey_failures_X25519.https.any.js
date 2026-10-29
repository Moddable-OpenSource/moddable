/*---
flags: [async,onlyStrict]
---*/
// Imported from web-platform-tests WebCryptoAPI/import_export/okp_importKey_failures_X25519.https.any.js (672f0e8 2026-10-17); lines marked MODDABLE are local changes
// ---- inlined WebCryptoAPI/util/okp_key_fixtures.js ----
var okpKeyData = {
    "Ed25519": {
        privateUsages: ["sign"],
        publicUsages: ["verify"],
        spki: new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 112, 3, 33, 0, 216, 225, 137, 99, 216, 9, 212, 135, 217, 84, 154, 204, 174, 198, 116, 46, 126, 235, 162, 77, 138, 13, 59, 20, 183, 227, 202, 234, 6, 137, 61, 204]),
        raw: new Uint8Array([216, 225, 137, 99, 216, 9, 212, 135, 217, 84, 154, 204, 174, 198, 116, 46, 126, 235, 162, 77, 138, 13, 59, 20, 183, 227, 202, 234, 6, 137, 61, 204]),
        pkcs8: new Uint8Array([48, 46, 2, 1, 0, 48, 5, 6, 3, 43, 101, 112, 4, 34, 4, 32, 243, 200, 244, 196, 141, 248, 120, 20, 110, 140, 211, 191, 109, 244, 229, 14, 56, 155, 167, 7, 78, 21, 194, 53, 45, 205, 93, 48, 141, 76, 168, 31]),
        jwk: {
            crv: "Ed25519",
            d: "88j0xI34eBRujNO_bfTlDjibpwdOFcI1Lc1dMI1MqB8",
            x: "2OGJY9gJ1IfZVJrMrsZ0Ln7rok2KDTsUt-PK6gaJPcw",
            kty: "OKP"
        }
    },

    "Ed448": {
        privateUsages: ["sign"],
        publicUsages: ["verify"],
        spki: new Uint8Array([48, 67, 48, 5, 6, 3, 43, 101, 113, 3, 58, 0, 171, 75, 184, 133, 253, 125, 44, 90, 242, 78, 131, 113, 12, 255, 160, 199, 74, 87, 226, 116, 128, 29, 178, 5, 123, 11, 220, 94, 160, 50, 182, 254, 107, 199, 139, 128, 69, 54, 90, 235, 38, 232, 110, 31, 20, 253, 52, 157, 7, 196, 132, 149, 245, 164, 106, 90, 128]),
        raw: new Uint8Array([171, 75, 184, 133, 253, 125, 44, 90, 242, 78, 131, 113, 12, 255, 160, 199, 74, 87, 226, 116, 128, 29, 178, 5, 123, 11, 220, 94, 160, 50, 182, 254, 107, 199, 139, 128, 69, 54, 90, 235, 38, 232, 110, 31, 20, 253, 52, 157, 7, 196, 132, 149, 245, 164, 106, 90, 128]),
        pkcs8: new Uint8Array([48, 71, 2, 1, 0, 48, 5, 6, 3, 43, 101, 113, 4, 59, 4, 57, 14, 255, 3, 69, 140, 40, 224, 23, 156, 82, 29, 227, 18, 201, 105, 183, 131, 67, 72, 236, 171, 153, 26, 96, 227, 178, 233, 167, 158, 76, 217, 228, 128, 239, 41, 23, 18, 210, 200, 61, 4, 114, 114, 213, 201, 244, 40, 102, 79, 105, 109, 38, 112, 69, 143, 29, 46]),
        jwk: {
            crv: "Ed448",
            d: "Dv8DRYwo4BecUh3jEslpt4NDSOyrmRpg47Lpp55M2eSA7ykXEtLIPQRyctXJ9ChmT2ltJnBFjx0u",
            x: "q0u4hf19LFryToNxDP-gx0pX4nSAHbIFewvcXqAytv5rx4uARTZa6ybobh8U_TSdB8SElfWkalqA",
            kty: "OKP"
        }
    },

    "X25519": {
        privateUsages: ["deriveKey", "deriveBits"],
        publicUsages: [],
        spki: new Uint8Array([48, 42, 48, 5, 6, 3, 43, 101, 110, 3, 33, 0, 28, 242, 177, 230, 2, 46, 197, 55, 55, 30, 215, 245, 62, 84, 250, 17, 84, 216, 62, 152, 235, 100, 234, 81, 250, 229, 179, 48, 124, 254, 151, 6]),
        raw: new Uint8Array([28, 242, 177, 230, 2, 46, 197, 55, 55, 30, 215, 245, 62, 84, 250, 17, 84, 216, 62, 152, 235, 100, 234, 81, 250, 229, 179, 48, 124, 254, 151, 6]),
        pkcs8: new Uint8Array([48, 46, 2, 1, 0, 48, 5, 6, 3, 43, 101, 110, 4, 34, 4, 32, 200, 131, 142, 118, 208, 87, 223, 183, 216, 201, 90, 105, 225, 56, 22, 10, 221, 99, 115, 253, 113, 164, 210, 118, 187, 86, 227, 168, 27, 100, 255, 97]),
        jwk: {
            crv: "X25519",
            d: "yIOOdtBX37fYyVpp4TgWCt1jc_1xpNJ2u1bjqBtk_2E",
            x: "HPKx5gIuxTc3Htf1PlT6EVTYPpjrZOpR-uWzMHz-lwY",
            kty: "OKP"
        }
    },

    "X448": {
        privateUsages: ["deriveKey", "deriveBits"],
        publicUsages: [],
        spki: new Uint8Array([48, 66, 48, 5, 6, 3, 43, 101, 111, 3, 57, 0, 182, 4, 161, 209, 165, 205, 29, 148, 38, 213, 97, 239, 99, 10, 158, 177, 108, 190, 105, 213, 185, 202, 97, 94, 220, 83, 99, 62, 251, 82, 234, 49, 230, 230, 160, 161, 219, 172, 198, 231, 108, 188, 230, 72, 45, 126, 75, 163, 213, 93, 158, 128, 39, 101, 206, 111]),
        raw: new Uint8Array([182, 4, 161, 209, 165, 205, 29, 148, 38, 213, 97, 239, 99, 10, 158, 177, 108, 190, 105, 213, 185, 202, 97, 94, 220, 83, 99, 62, 251, 82, 234, 49, 230, 230, 160, 161, 219, 172, 198, 231, 108, 188, 230, 72, 45, 126, 75, 163, 213, 93, 158, 128, 39, 101, 206, 111]),
        pkcs8: new Uint8Array([48, 70, 2, 1, 0, 48, 5, 6, 3, 43, 101, 111, 4, 58, 4, 56, 88, 199, 210, 154, 62, 181, 25, 178, 157, 0, 207, 177, 145, 187, 100, 252, 109, 138, 66, 216, 241, 113, 118, 39, 43, 137, 242, 39, 45, 24, 25, 41, 92, 101, 37, 192, 130, 150, 113, 176, 82, 239, 7, 39, 83, 15, 24, 142, 49, 208, 204, 83, 191, 38, 146, 158]),
        jwk: {
            crv: "X448",
            d: "WMfSmj61GbKdAM-xkbtk_G2KQtjxcXYnK4nyJy0YGSlcZSXAgpZxsFLvBydTDxiOMdDMU78mkp4",
            x: "tgSh0aXNHZQm1WHvYwqesWy-adW5ymFe3FNjPvtS6jHm5qCh26zG52y85kgtfkuj1V2egCdlzm8",
            kty: "OKP"
        }
    },

};

// ---- end util/okp_key_fixtures.js ----
// ---- inlined WebCryptoAPI/import_export/okp_importKey_failures_fixtures.js ----
function algorithmName(algorithm) {
    return algorithm.name || algorithm;
}

function privateJwk(name) {
    return Object.assign({}, okpKeyData[name].jwk);
}

function publicJwk(name) {
    var jwk = privateJwk(name);
    delete jwk.d;
    return jwk;
}

function getValidKeyData(algorithm) {
    var name = algorithmName(algorithm);
    var key = okpKeyData[name];
    return [
        {format: "spki", data: key.spki},
        {format: "pkcs8", data: key.pkcs8},
        {format: "raw", data: key.raw},
        {format: "jwk", data: privateJwk(name)},
        {format: "jwk", data: publicJwk(name)}
    ];
}

function getBadKeyLengthData(algorithm) {
    var name = algorithmName(algorithm);
    var key = okpKeyData[name];
    var badPrivateJwk = privateJwk(name);
    var badPublicJwk = publicJwk(name);
    badPrivateJwk.d = badPrivateJwk.d.slice(0, -1);
    badPublicJwk.x = badPublicJwk.x.slice(0, -1);
    return [
        {format: "spki", data: key.spki.slice(0, -1)},
        {format: "pkcs8", data: key.pkcs8.slice(0, -1)},
        {format: "raw", data: key.raw.slice(0, -1)},
        {format: "jwk", data: badPrivateJwk},
        {format: "jwk", data: badPublicJwk}
    ];
}

function getMissingJWKFieldKeyData(algorithm) {
    var name = algorithmName(algorithm);
    var missingX = privateJwk(name);
    var missingKty = privateJwk(name);
    var missingCrv = name === "Ed448" ? privateJwk(name) : publicJwk(name);
    delete missingX.x;
    delete missingKty.kty;
    delete missingCrv.crv;
    return [
        {param: "x", data: missingX},
        {param: "kty", data: missingKty},
        {param: "crv", data: missingCrv}
    ];
}

function getMismatchedJWKKeyData(algorithm) {
    var name = algorithmName(algorithm);
    var jwk = privateJwk(name);
    jwk.x = mismatchedPublicKeys[name];
    return [jwk];
}

function getMismatchedKtyField(algorithm) {
    return "EC";
}

function getMismatchedCrvField(algorithm) {
    return mismatchedCurves[algorithmName(algorithm)];
}

var mismatchedPublicKeys = {
    "Ed25519": "11qYAYKxCrfVS_7TyWQHOg7hcvPapiMlrwIaaPcHURo",
    "Ed448": "X9dEm1m0Yf0s54fsYWrUah2hNCSFpw4fig6nXYDpZ3jt8SR2m0bHBhvWeD3x5Q9s0foavq_oJWGA",
    "X25519": "hSDwCYkwp1R0i33ctD73Wg2_Og0mOBr066SpjqqbTmo",
    "X448": "mwj3zDG34+Z9ItWuoSEHSic70rg94Jxj+qc9LCLF2bvINmRyQdlT1AxbEtqIEg1TF3+A5TLEH6A"
};

var mismatchedCurves = {
    "Ed25519": "X25519",
    "X25519": "Ed25519",
    "Ed448": "X448",
    "X448": "Ed448"
};

// ---- end import_export/okp_importKey_failures_fixtures.js ----
// ---- inlined WebCryptoAPI/import_export/importKey_failures.js ----
function run_test(algorithmNames) {
    var subtle = crypto.subtle; // Change to test prefixed implementations

// These tests check that importKey and exportKey throw an error, and that
// the error is of the right type, for a wide set of incorrect parameters.

// Error testing occurs by setting the parameter that should trigger the
// error to an invalid value, then combining that with all valid
// parameters that should be checked earlier by importKey, and all
// valid and invalid parameters that should be checked later by
// importKey.
//
// There are a lot of combinations of possible parameters for both
// success and failure modes, resulting in a very large number of tests
// performed.


    var allTestVectors = [ // Parameters that should work for importKey / exportKey
        {name: "Ed25519", privateUsages: ["sign"], publicUsages: ["verify"]},
        {name: "ML-DSA-44", privateUsages: ["sign"], publicUsages: ["verify"]},
        {name: "ML-DSA-65", privateUsages: ["sign"], publicUsages: ["verify"]},
        {name: "ML-DSA-87", privateUsages: ["sign"], publicUsages: ["verify"]},
        {name: "ML-KEM-512", privateUsages: ["decapsulateKey", "decapsulateBits"], publicUsages: ["encapsulateKey", "encapsulateBits"]},
        {name: "ML-KEM-768", privateUsages: ["decapsulateKey", "decapsulateBits"], publicUsages: ["encapsulateKey", "encapsulateBits"]},
        {name: "ML-KEM-1024", privateUsages: ["decapsulateKey", "decapsulateBits"], publicUsages: ["encapsulateKey", "encapsulateBits"]},
        {name: "Ed448", privateUsages: ["sign"], publicUsages: ["verify"]},
        {name: "ECDSA", privateUsages: ["sign"], publicUsages: ["verify"]},
        {name: "X25519",  privateUsages: ["deriveKey", "deriveBits"], publicUsages: []},
        {name: "X448",  privateUsages: ["deriveKey", "deriveBits"], publicUsages: []},
        {name: "ECDH",  privateUsages: ["deriveKey", "deriveBits"], publicUsages: []}
    ];

    var testVectors = [];
    if (algorithmNames && !Array.isArray(algorithmNames)) {
        algorithmNames = [algorithmNames];
    };
    allTestVectors.forEach(function(vector) {
        if (!algorithmNames || algorithmNames.includes(vector.name)) {
            testVectors.push(vector);
        }
    });

    function parameterString(format, algorithm, extractable, usages, data) {
        if (typeof algorithm !== "object" && typeof algorithm !== "string") {
            alert(algorithm);
        }

        var jwk_label = "";
        if (format === "jwk")
            jwk_label = isPublicKey(data) ? " (public) " : "(private)";

        var result = "(" +
                        objectToString(format) + jwk_label + ", " +
                        objectToString(algorithm) + ", " +
                        objectToString(extractable) + ", " +
                        objectToString(usages) +
                     ")";

        return result;
    }

    // Test that a given combination of parameters results in an error,
    // AND that it is the correct kind of error.
    //
    // Expected error is either a number, tested against the error code,
    // or a string, tested against the error name.
    function testError(format, algorithm, keyData, keySize, usages, extractable, expectedError, testTag) {
        promise_test(async() => {
            let key;
            try {
                key = await subtle.importKey(format, keyData, algorithm, extractable, usages);
            } catch(err) {
                let actualError = typeof expectedError === "number" ? err.code : err.name;
                assert_equals(actualError, expectedError, testTag + " not supported.");
            }
            assert_equals(key, undefined, "Operation succeeded, but should not have.");
        }, testTag + ": importKey" + parameterString(format, algorithm, extractable, usages, keyData));
    }

    // Don't create an exhaustive list of all invalid usages because
    // there would be too many to test. Instead, create every singleton
    // of an illegal usage, and "poison" every valid usage
    // with an illegal one.
    function invalidUsages(validUsages, mandatoryUsages) {
        var results = [];

        var illegalUsages = [];
        allKeyUsages.forEach(function(usage) {
            if (!validUsages.includes(usage)) {
                illegalUsages.push(usage);
            }
        });

        var goodUsageCombinations = validUsages.length === 0 ? [] : allValidUsages(validUsages, false, mandatoryUsages);

        illegalUsages.forEach(function(illegalUsage) {
            results.push([illegalUsage]);
            goodUsageCombinations.forEach(function(usageCombination) {
                results.push(usageCombination.concat([illegalUsage]));
            });
        });

        return results;
    }

    function validUsages(usages, format, data) {
        if (format === 'spki' || format === 'raw' || format === 'raw-public') return usages.publicUsages
        if (format === 'pkcs8' || format === 'raw-private' || format === 'raw-seed') return usages.privateUsages
        if (format === 'jwk') {
            if (data === undefined)
                return [];
            return isPublicKey(data) ? usages.publicUsages : usages.privateUsages;
        }
        return [];
    }

    function isPublicKey(data) {
        return data.d === undefined && data.priv === undefined;
    }

    function isPrivateKey(data) {
        return !isPublicKey(data);
    }

// Now test for properly handling errors
// - Unsupported algorithm
// - Bad usages for algorithm
// - Bad key lengths
// - Lack of a mandatory format field
// - Incompatible keys pair

    // Algorithms normalize okay, but usages bad (though not empty).
    // It shouldn't matter what other extractable is. Should fail
    // due to SyntaxError
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getValidKeyData(algorithm).forEach(function(test) {
                invalidUsages(validUsages(vector, test.format, test.data)).forEach(function(usages) {
                    [true, false].forEach(function(extractable) {
                        testError(test.format, algorithm, test.data, name, usages, extractable, "SyntaxError", "Bad usages");
                    });
                });
            });
        });
    });

    // Algorithms normalize okay, but usages bad (empty).
    // Should fail due to SyntaxError
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getValidKeyData(algorithm).filter((test) => test.format === 'pkcs8' || (test.format === 'jwk' && isPrivateKey(test.data))).forEach(function(test) {
                [true, false].forEach(function(extractable) {
                    testError(test.format, algorithm, test.data, name, [/* Empty usages */], extractable, "SyntaxError", "Empty usages");
                });
            });
        });
    });

    // Algorithms normalize okay, usages ok. The length of the key must throw a DataError exception.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getBadKeyLengthData(algorithm).forEach(function(test) {
                allValidUsages(validUsages(vector, test.format, test.data)).forEach(function(usages) {
                    [true, false].forEach(function(extractable) {
                        testError(test.format, algorithm, test.data, name, usages, extractable, "DataError", "Bad key length");
                    });
                });
            });
        });
    });

    // Algorithms normalize okay, usages ok and valid key. The lack of the mandatory JWK parameter must throw a DataError exception.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
           getMissingJWKFieldKeyData(algorithm).forEach(function(test) {
                allValidUsages(validUsages(vector, 'jwk', test.data)).forEach(function(usages) {
                    [true, false].forEach(function(extractable) {
                        testError('jwk', algorithm, test.data, name, usages, extractable, "DataError", "Missing JWK '" + test.param + "' parameter");
                    });
                });
            });
        });
    });

    // Algorithms normalize okay, usages ok and valid key. The public key is not compatible with the private key.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getMismatchedJWKKeyData(algorithm).forEach(function(data) {
                allValidUsages(vector.privateUsages).forEach(function(usages) {
                    [true].forEach(function(extractable) {
                        testError('jwk', algorithm, data, name, usages, extractable, "DataError", "Invalid key pair");
                    });
                });
            });
        });
    });

    // Missing mandatory "name" field on algorithm
    testVectors.forEach(function(vector) {
        var name = vector.name;
        // We just need *some* valid keydata, so pick the first available algorithm.
        var algorithm = allAlgorithmSpecifiersFor(name)[0];
        getValidKeyData(algorithm).forEach(function(test) {
            validUsages(vector, test.format, test.data).forEach(function(usages) {
                [true, false].forEach(function(extractable) {
                    testError(test.format, {}, test.data, name, usages, extractable, "TypeError", "Missing algorithm name");
                });
            });
        });
    });

    // The 'kty' field is not correct.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getValidKeyData(algorithm).forEach(function(test) {
                if (test.format === "jwk") {
                    var data = {crv: test.data.crv, kty: test.data.kty, d: test.data.d, x: test.data.x, y: test.data.y};
                    data.kty = getMismatchedKtyField(algorithm);
                    var usages =  validUsages(vector, 'jwk', test.data);
                    testError('jwk', algorithm, data, name, usages, true, "DataError", "Invalid 'kty' field");
                }
            });
        });
    });

    // The 'ext' field is not correct.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getValidKeyData(algorithm).forEach(function(test) {
                if (test.format === "jwk") {
                    var data = {crv: test.data.crv, kty: test.data.kty, d: test.data.d, x: test.data.x, y: test.data.y};
                    data.ext = false;
                    var usages =  validUsages(vector, 'jwk', test.data);
                    testError('jwk', algorithm, data, name, usages, true, "DataError", "Import from a non-extractable");
                }
            });
        });
    });

    // The 'use' field is incorrect.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getValidKeyData(algorithm).forEach(function(test) {
                if (test.format === "jwk") {
                    var data = {crv: test.data.crv, kty: test.data.kty, d: test.data.d, x: test.data.x, y: test.data.y};
                    data.use = "invalid";
                    var usages =  validUsages(vector, 'jwk', test.data);
                    if (usages.length !== 0)
                        testError('jwk', algorithm, data, name, usages, true, "DataError", "Invalid 'use' field");
                }
            });
        });
    });

    // The 'crv' field is incorrect.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getValidKeyData(algorithm).forEach(function(test) {
                if (test.format === "jwk") {
                    var data = {crv: test.data.crv, kty: test.data.kty, d: test.data.d, x: test.data.x, y: test.data.y};
                    data.crv = getMismatchedCrvField(algorithm)
                    var usages =  validUsages(vector, 'jwk', test.data);
                    testError('jwk', algorithm, data, name, usages, true, "DataError", "Invalid 'crv' field");
                }
            });
        });
    });

    // Use an 'alg' field with incorrect casing.
    testVectors.forEach(function(vector) {
        var name = vector.name;
        if (name !== "Ed25519" && name !== "Ed448")
            return; // The rest ignore the 'alg' field.
        allAlgorithmSpecifiersFor(name).forEach(function(algorithm) {
            getValidKeyData(algorithm).forEach(function(test) {
                if (test.format === "jwk") {
                    var data = {crv: test.data.crv, kty: test.data.kty, d: test.data.d, x: test.data.x, y: test.data.y};
                    var usages =  validUsages(vector, 'jwk', test.data);
                    [name.toLowerCase(), name.toUpperCase()].forEach(function(algName) {
                        data.alg = algName;
                        testError('jwk', algorithm, data, name, usages, true, "DataError", "Invalid 'alg' field '" + data.alg + "'");
                    });
                }
            });
        });
    });
}

// ---- end import_export/importKey_failures.js ----
// META: title=WebCryptoAPI: importKey() for Failures
// META: timeout=long
// META: script=../util/helpers.js
// META: script=../util/okp_key_fixtures.js
// META: script=okp_importKey_failures_fixtures.js
// META: script=importKey_failures.js


run_test(["X25519"]);
