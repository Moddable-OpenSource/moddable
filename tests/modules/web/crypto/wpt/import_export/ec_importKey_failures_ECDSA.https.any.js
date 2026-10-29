/*---
flags: [async,onlyStrict]
---*/
// Imported from web-platform-tests WebCryptoAPI/import_export/ec_importKey_failures_ECDSA.https.any.js (672f0e8 2026-10-17); lines marked MODDABLE are local changes
// ---- inlined WebCryptoAPI/util/ec_key_fixtures.js ----
var ecKeyData = {
    "P-521": {
        spki: new Uint8Array([48, 129, 155, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 35, 3, 129, 134, 0, 4, 1, 86, 244, 121, 248, 223, 30, 32, 167, 255, 192, 76, 228, 32, 195, 225, 84, 174, 37, 25, 150, 190, 228, 47, 3, 75, 132, 212, 27, 116, 63, 52, 228, 95, 49, 27, 129, 58, 156, 222, 200, 205, 165, 155, 187, 189, 49, 212, 96, 179, 41, 37, 33, 231, 193, 183, 34, 229, 102, 124, 3, 219, 47, 174, 117, 63, 1, 80, 23, 54, 207, 226, 71, 57, 67, 32, 216, 228, 175, 194, 253, 57, 181, 169, 51, 16, 97, 184, 30, 34, 65, 40, 43, 158, 23, 137, 24, 34, 181, 183, 158, 5, 47, 69, 151, 181, 150, 67, 253, 57, 55, 156, 81, 189, 81, 37, 196, 244, 139, 195, 240, 37, 206, 60, 211, 105, 83, 40, 108, 203, 56, 251]),
        spki_compressed: new Uint8Array([48, 88, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 35, 3, 68, 0, 3, 1, 86, 244, 121, 248, 223, 30, 32, 167, 255, 192, 76, 228, 32, 195, 225, 84, 174, 37, 25, 150, 190, 228, 47, 3, 75, 132, 212, 27, 116, 63, 52, 228, 95, 49, 27, 129, 58, 156, 222, 200, 205, 165, 155, 187, 189, 49, 212, 96, 179, 41, 37, 33, 231, 193, 183, 34, 229, 102, 124, 3, 219, 47, 174, 117, 63]),
        raw: new Uint8Array([4, 1, 86, 244, 121, 248, 223, 30, 32, 167, 255, 192, 76, 228, 32, 195, 225, 84, 174, 37, 25, 150, 190, 228, 47, 3, 75, 132, 212, 27, 116, 63, 52, 228, 95, 49, 27, 129, 58, 156, 222, 200, 205, 165, 155, 187, 189, 49, 212, 96, 179, 41, 37, 33, 231, 193, 183, 34, 229, 102, 124, 3, 219, 47, 174, 117, 63, 1, 80, 23, 54, 207, 226, 71, 57, 67, 32, 216, 228, 175, 194, 253, 57, 181, 169, 51, 16, 97, 184, 30, 34, 65, 40, 43, 158, 23, 137, 24, 34, 181, 183, 158, 5, 47, 69, 151, 181, 150, 67, 253, 57, 55, 156, 81, 189, 81, 37, 196, 244, 139, 195, 240, 37, 206, 60, 211, 105, 83, 40, 108, 203, 56, 251]),
        raw_compressed: new Uint8Array([3, 1, 86, 244, 121, 248, 223, 30, 32, 167, 255, 192, 76, 228, 32, 195, 225, 84, 174, 37, 25, 150, 190, 228, 47, 3, 75, 132, 212, 27, 116, 63, 52, 228, 95, 49, 27, 129, 58, 156, 222, 200, 205, 165, 155, 187, 189, 49, 212, 96, 179, 41, 37, 33, 231, 193, 183, 34, 229, 102, 124, 3, 219, 47, 174, 117, 63]),
        pkcs8: new Uint8Array([48, 129, 238, 2, 1, 0, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 35, 4, 129, 214, 48, 129, 211, 2, 1, 1, 4, 66, 0, 244, 8, 117, 131, 104, 186, 147, 15, 48, 247, 106, 224, 84, 254, 92, 210, 206, 127, 218, 44, 159, 118, 166, 212, 54, 207, 117, 214, 108, 68, 11, 254, 99, 49, 199, 193, 114, 161, 36, 120, 25, 60, 130, 81, 72, 123, 201, 18, 99, 250, 80, 33, 127, 133, 255, 99, 111, 89, 205, 84, 110, 58, 180, 131, 180, 161, 129, 137, 3, 129, 134, 0, 4, 1, 86, 244, 121, 248, 223, 30, 32, 167, 255, 192, 76, 228, 32, 195, 225, 84, 174, 37, 25, 150, 190, 228, 47, 3, 75, 132, 212, 27, 116, 63, 52, 228, 95, 49, 27, 129, 58, 156, 222, 200, 205, 165, 155, 187, 189, 49, 212, 96, 179, 41, 37, 33, 231, 193, 183, 34, 229, 102, 124, 3, 219, 47, 174, 117, 63, 1, 80, 23, 54, 207, 226, 71, 57, 67, 32, 216, 228, 175, 194, 253, 57, 181, 169, 51, 16, 97, 184, 30, 34, 65, 40, 43, 158, 23, 137, 24, 34, 181, 183, 158, 5, 47, 69, 151, 181, 150, 67, 253, 57, 55, 156, 81, 189, 81, 37, 196, 244, 139, 195, 240, 37, 206, 60, 211, 105, 83, 40, 108, 203, 56, 251]),
        pkcs8_private_only: new Uint8Array([48, 96, 2, 1, 0, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 35, 4, 73, 48, 71, 2, 1, 1, 4, 66, 0, 244, 8, 117, 131, 104, 186, 147, 15, 48, 247, 106, 224, 84, 254, 92, 210, 206, 127, 218, 44, 159, 118, 166, 212, 54, 207, 117, 214, 108, 68, 11, 254, 99, 49, 199, 193, 114, 161, 36, 120, 25, 60, 130, 81, 72, 123, 201, 18, 99, 250, 80, 33, 127, 133, 255, 99, 111, 89, 205, 84, 110, 58, 180, 131, 180]),
        jwk: {
            kty: "EC",
            crv: "P-521",
            x: "AVb0efjfHiCn_8BM5CDD4VSuJRmWvuQvA0uE1Bt0PzTkXzEbgTqc3sjNpZu7vTHUYLMpJSHnwbci5WZ8A9svrnU_",
            y: "AVAXNs_iRzlDINjkr8L9ObWpMxBhuB4iQSgrnheJGCK1t54FL0WXtZZD_Tk3nFG9USXE9IvD8CXOPNNpUyhsyzj7",
            d: "APQIdYNoupMPMPdq4FT-XNLOf9osn3am1DbPddZsRAv-YzHHwXKhJHgZPIJRSHvJEmP6UCF_hf9jb1nNVG46tIO0"
        }
    },

    "P-256": {
        spki: new Uint8Array([48, 89, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 3, 66, 0, 4, 210, 16, 176, 166, 249, 217, 240, 18, 134, 128, 88, 180, 63, 164, 244, 113, 1, 133, 67, 187, 160, 12, 146, 80, 223, 146, 87, 194, 172, 174, 93, 209, 206, 3, 117, 82, 212, 129, 69, 12, 227, 155, 77, 16, 149, 112, 27, 23, 91, 250, 179, 75, 142, 108, 9, 158, 24, 241, 193, 152, 53, 131, 97, 232]),
        spki_compressed: new Uint8Array([48, 57, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 3, 34, 0, 2, 210, 16, 176, 166, 249, 217, 240, 18, 134, 128, 88, 180, 63, 164, 244, 113, 1, 133, 67, 187, 160, 12, 146, 80, 223, 146, 87, 194, 172, 174, 93, 209]),
        raw: new Uint8Array([4, 210, 16, 176, 166, 249, 217, 240, 18, 134, 128, 88, 180, 63, 164, 244, 113, 1, 133, 67, 187, 160, 12, 146, 80, 223, 146, 87, 194, 172, 174, 93, 209, 206, 3, 117, 82, 212, 129, 69, 12, 227, 155, 77, 16, 149, 112, 27, 23, 91, 250, 179, 75, 142, 108, 9, 158, 24, 241, 193, 152, 53, 131, 97, 232]),
        raw_compressed: new Uint8Array([2, 210, 16, 176, 166, 249, 217, 240, 18, 134, 128, 88, 180, 63, 164, 244, 113, 1, 133, 67, 187, 160, 12, 146, 80, 223, 146, 87, 194, 172, 174, 93, 209]),
        pkcs8: new Uint8Array([48, 129, 135, 2, 1, 0, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 4, 109, 48, 107, 2, 1, 1, 4, 32, 19, 211, 58, 45, 90, 191, 156, 249, 235, 178, 31, 248, 96, 212, 174, 254, 110, 86, 231, 119, 144, 244, 222, 233, 180, 8, 132, 235, 211, 53, 68, 234, 161, 68, 3, 66, 0, 4, 210, 16, 176, 166, 249, 217, 240, 18, 134, 128, 88, 180, 63, 164, 244, 113, 1, 133, 67, 187, 160, 12, 146, 80, 223, 146, 87, 194, 172, 174, 93, 209, 206, 3, 117, 82, 212, 129, 69, 12, 227, 155, 77, 16, 149, 112, 27, 23, 91, 250, 179, 75, 142, 108, 9, 158, 24, 241, 193, 152, 53, 131, 97, 232]),
        pkcs8_private_only: new Uint8Array([48, 65, 2, 1, 0, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 4, 39, 48, 37, 2, 1, 1, 4, 32, 19, 211, 58, 45, 90, 191, 156, 249, 235, 178, 31, 248, 96, 212, 174, 254, 110, 86, 231, 119, 144, 244, 222, 233, 180, 8, 132, 235, 211, 53, 68, 234]),
        jwk: {
            kty: "EC",
            crv: "P-256",
            x: "0hCwpvnZ8BKGgFi0P6T0cQGFQ7ugDJJQ35JXwqyuXdE",
            y: "zgN1UtSBRQzjm00QlXAbF1v6s0uObAmeGPHBmDWDYeg",
            d: "E9M6LVq_nPnrsh_4YNSu_m5W53eQ9N7ptAiE69M1ROo"
        }
    },

    "P-384": {
        spki: new Uint8Array([48, 118, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 34, 3, 98, 0, 4, 33, 156, 20, 214, 102, 23, 179, 110, 198, 216, 133, 107, 56, 91, 115, 167, 77, 52, 79, 216, 174, 117, 239, 4, 100, 53, 221, 165, 78, 59, 68, 189, 95, 189, 235, 209, 208, 141, 214, 158, 45, 125, 193, 220, 33, 140, 180, 53, 189, 40, 19, 140, 199, 120, 51, 122, 132, 47, 107, 214, 27, 36, 14, 116, 36, 159, 36, 102, 124, 42, 88, 16, 167, 107, 252, 40, 224, 51, 95, 136, 166, 80, 29, 236, 1, 151, 109, 168, 90, 251, 0, 134, 156, 182, 172, 232]),
        spki_compressed: new Uint8Array([48, 70, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 34, 3, 50, 0, 2, 33, 156, 20, 214, 102, 23, 179, 110, 198, 216, 133, 107, 56, 91, 115, 167, 77, 52, 79, 216, 174, 117, 239, 4, 100, 53, 221, 165, 78, 59, 68, 189, 95, 189, 235, 209, 208, 141, 214, 158, 45, 125, 193, 220, 33, 140, 180, 53]),
        raw: new Uint8Array([4, 33, 156, 20, 214, 102, 23, 179, 110, 198, 216, 133, 107, 56, 91, 115, 167, 77, 52, 79, 216, 174, 117, 239, 4, 100, 53, 221, 165, 78, 59, 68, 189, 95, 189, 235, 209, 208, 141, 214, 158, 45, 125, 193, 220, 33, 140, 180, 53, 189, 40, 19, 140, 199, 120, 51, 122, 132, 47, 107, 214, 27, 36, 14, 116, 36, 159, 36, 102, 124, 42, 88, 16, 167, 107, 252, 40, 224, 51, 95, 136, 166, 80, 29, 236, 1, 151, 109, 168, 90, 251, 0, 134, 156, 182, 172, 232]),
        raw_compressed: new Uint8Array([2, 33, 156, 20, 214, 102, 23, 179, 110, 198, 216, 133, 107, 56, 91, 115, 167, 77, 52, 79, 216, 174, 117, 239, 4, 100, 53, 221, 165, 78, 59, 68, 189, 95, 189, 235, 209, 208, 141, 214, 158, 45, 125, 193, 220, 33, 140, 180, 53]),
        pkcs8: new Uint8Array([48, 129, 182, 2, 1, 0, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 34, 4, 129, 158, 48, 129, 155, 2, 1, 1, 4, 48, 69, 55, 181, 153, 7, 132, 211, 194, 210, 46, 150, 168, 249, 47, 161, 170, 73, 46, 232, 115, 229, 118, 164, 21, 130, 225, 68, 24, 60, 152, 136, 209, 14, 107, 158, 180, 206, 212, 178, 204, 64, 18, 228, 172, 94, 168, 64, 115, 161, 100, 3, 98, 0, 4, 33, 156, 20, 214, 102, 23, 179, 110, 198, 216, 133, 107, 56, 91, 115, 167, 77, 52, 79, 216, 174, 117, 239, 4, 100, 53, 221, 165, 78, 59, 68, 189, 95, 189, 235, 209, 208, 141, 214, 158, 45, 125, 193, 220, 33, 140, 180, 53, 189, 40, 19, 140, 199, 120, 51, 122, 132, 47, 107, 214, 27, 36, 14, 116, 36, 159, 36, 102, 124, 42, 88, 16, 167, 107, 252, 40, 224, 51, 95, 136, 166, 80, 29, 236, 1, 151, 109, 168, 90, 251, 0, 134, 156, 182, 172, 232]),
        pkcs8_private_only: new Uint8Array([48, 78, 2, 1, 0, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 34, 4, 55, 48, 53, 2, 1, 1, 4, 48, 69, 55, 181, 153, 7, 132, 211, 194, 210, 46, 150, 168, 249, 47, 161, 170, 73, 46, 232, 115, 229, 118, 164, 21, 130, 225, 68, 24, 60, 152, 136, 209, 14, 107, 158, 180, 206, 212, 178, 204, 64, 18, 228, 172, 94, 168, 64, 115]),
        jwk: {
            kty: "EC",
            crv: "P-384",
            x: "IZwU1mYXs27G2IVrOFtzp000T9iude8EZDXdpU47RL1fvevR0I3Wni19wdwhjLQ1",
            y: "vSgTjMd4M3qEL2vWGyQOdCSfJGZ8KlgQp2v8KOAzX4imUB3sAZdtqFr7AIactqzo",
            d: "RTe1mQeE08LSLpao-S-hqkku6HPldqQVguFEGDyYiNEOa560ztSyzEAS5KxeqEBz"
        }
    },

};

// ---- end util/ec_key_fixtures.js ----
// ---- inlined WebCryptoAPI/import_export/ec_importKey_failures_fixtures.js ----
function ecPrivateJwk(namedCurve) {
    return Object.assign({}, ecKeyData[namedCurve].jwk);
}

function getValidKeyData(algorithm) {
    var key = ecKeyData[algorithm.namedCurve];
    return [
        {format: "spki", data: key.spki},
        {format: "raw", data: key.raw},
        {format: "pkcs8", data: key.pkcs8},
        {format: "jwk", data: ecPrivateJwk(algorithm.namedCurve)}
    ];
}

function getBadKeyLengthData(algorithm) {
    var key = ecKeyData[algorithm.namedCurve];
    var jwk = ecPrivateJwk(algorithm.namedCurve);
    jwk.x = jwk.x.slice(0, -1);
    return [
        {format: "spki", data: key.spki.slice(0, -1)},
        {format: "raw", data: key.raw.slice(0, -1)},
        {format: "pkcs8", data: key.pkcs8.slice(0, -1)},
        {format: "jwk", data: jwk}
    ];
}

function getMissingJWKFieldKeyData(algorithm) {
    var missingX = ecPrivateJwk("P-521");
    var missingKty = ecPrivateJwk("P-521");
    var missingCrv = ecPrivateJwk("P-521");
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
    // TODO: Implement test cases where the public key doesn't match the private key.
    return [];
}

function getMismatchedKtyField(algorithm) {
    return "OKP";
}

function getMismatchedCrvField(algorithm) {
    return mismatchedEcCurves[algorithm.namedCurve];
}

var mismatchedEcCurves = {
    "P-521": "P-256",
    "P-256": "P-384",
    "P-384": "P-521"
};

// ---- end import_export/ec_importKey_failures_fixtures.js ----
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
// META: script=../util/ec_key_fixtures.js
// META: script=ec_importKey_failures_fixtures.js
// META: script=importKey_failures.js

// Setup: define the correct behaviors that should be sought, and create
// helper functions that generate all possible test parameters for
// different situations.
run_test(["ECDSA"]);
