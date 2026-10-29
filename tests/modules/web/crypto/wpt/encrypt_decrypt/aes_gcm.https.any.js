/*---
flags: [async,onlyStrict]
---*/
// Imported from web-platform-tests WebCryptoAPI/encrypt_decrypt/aes_gcm.https.any.js (672f0e8 2026-10-17); lines marked MODDABLE are local changes
// ---- inlined WebCryptoAPI/encrypt_decrypt/aes_common_fixtures.js ----
function getAesCommonFixtures() {
  var plaintext = new Uint8Array([
    84, 104, 105, 115, 32, 115, 112, 101, 99, 105, 102, 105, 99, 97, 116, 105,
    111, 110, 32, 100, 101, 115, 99, 114, 105, 98, 101, 115, 32, 97, 32, 74, 97,
    118, 97, 83, 99, 114, 105, 112, 116, 32, 65, 80, 73, 32, 102, 111, 114, 32,
    112, 101, 114, 102, 111, 114, 109, 105, 110, 103, 32, 98, 97, 115, 105, 99,
    32, 99, 114, 121, 112, 116, 111, 103, 114, 97, 112, 104, 105, 99, 32, 111,
    112, 101, 114, 97, 116, 105, 111, 110, 115, 32, 105, 110, 32, 119, 101, 98,
    32, 97, 112, 112, 108, 105, 99, 97, 116, 105, 111, 110, 115, 44, 32, 115,
    117, 99, 104, 32, 97, 115, 32, 104, 97, 115, 104, 105, 110, 103, 44, 32,
    115, 105, 103, 110, 97, 116, 117, 114, 101, 32, 103, 101, 110, 101, 114, 97,
    116, 105, 111, 110, 32, 97, 110, 100, 32, 118, 101, 114, 105, 102, 105, 99,
    97, 116, 105, 111, 110, 44, 32, 97, 110, 100, 32, 101, 110, 99, 114, 121,
    112, 116, 105, 111, 110, 32, 97, 110, 100, 32, 100, 101, 99, 114, 121, 112,
    116, 105, 111, 110, 46, 32, 65, 100, 100, 105, 116, 105, 111, 110, 97, 108,
    108, 121, 44, 32, 105, 116, 32, 100, 101, 115, 99, 114, 105, 98, 101, 115,
    32, 97, 110, 32, 65, 80, 73, 32, 102, 111, 114, 32, 97, 112, 112, 108, 105,
    99, 97, 116, 105, 111, 110, 115, 32, 116, 111, 32, 103, 101, 110, 101, 114,
    97, 116, 101, 32, 97, 110, 100, 47, 111, 114, 32, 109, 97, 110, 97, 103,
    101, 32, 116, 104, 101, 32, 107, 101, 121, 105, 110, 103, 32, 109, 97, 116,
    101, 114, 105, 97, 108, 32, 110, 101, 99, 101, 115, 115, 97, 114, 121, 32,
    116, 111, 32, 112, 101, 114, 102, 111, 114, 109, 32, 116, 104, 101, 115,
    101, 32, 111, 112, 101, 114, 97, 116, 105, 111, 110, 115, 46, 32, 85, 115,
    101, 115, 32, 102, 111, 114, 32, 116, 104, 105, 115, 32, 65, 80, 73, 32,
    114, 97, 110, 103, 101, 32, 102, 114, 111, 109, 32, 117, 115, 101, 114, 32,
    111, 114, 32, 115, 101, 114, 118, 105, 99, 101, 32, 97, 117, 116, 104, 101,
    110, 116, 105, 99, 97, 116, 105, 111, 110, 44, 32, 100, 111, 99, 117, 109,
    101, 110, 116, 32, 111, 114, 32, 99, 111, 100, 101, 32, 115, 105, 103, 110,
    105, 110, 103, 44, 32, 97, 110, 100, 32, 116, 104, 101, 32, 99, 111, 110,
    102, 105, 100, 101, 110, 116, 105, 97, 108, 105, 116, 121, 32, 97, 110, 100,
    32, 105, 110, 116, 101, 103, 114, 105, 116, 121, 32, 111, 102, 32, 99, 111,
    109, 109, 117, 110, 105, 99, 97, 116, 105, 111, 110, 115, 46,
  ]);

  var keyBytes = {
    128: new Uint8Array([
      222, 192, 212, 252, 191, 60, 71, 65, 200, 146, 218, 189, 28, 212, 192, 78,
    ]),
    192: new Uint8Array([
      208, 238, 131, 65, 63, 68, 196, 63, 186, 208, 61, 207, 166, 18, 99, 152,
      29, 109, 221, 95, 240, 30, 28, 246,
    ]),
    256: new Uint8Array([
      103, 105, 56, 35, 251, 29, 88, 7, 63, 145, 236, 233, 204, 58, 249, 16,
      229, 83, 38, 22, 164, 210, 123, 19, 235, 123, 116, 216, 0, 11, 191, 48,
    ]),
  };

  var additionalData = new Uint8Array([
    84, 104, 101, 114, 101, 32, 97, 114, 101, 32, 55, 32, 102, 117, 114, 116,
    104, 101, 114, 32, 101, 100, 105, 116, 111, 114, 105, 97, 108, 32, 110, 111,
    116, 101, 115, 32, 105, 110, 32, 116, 104, 101, 32, 100, 111, 99, 117, 109,
    101, 110, 116, 46,
  ]);

  return { plaintext, keyBytes, additionalData };
}

// ---- end encrypt_decrypt/aes_common_fixtures.js ----
// ---- inlined WebCryptoAPI/encrypt_decrypt/aes_gcm_96_iv_fixtures.js ----
function getFixtures() {
  var commonFixtures = getAesCommonFixtures();
  var plaintext = commonFixtures.plaintext;
  var keyBytes = commonFixtures.keyBytes;

  // AES-GCM specification recommends that the IV should be 96 bits long.
  var iv = new Uint8Array([
    58, 146, 115, 42, 166, 234, 57, 191, 57, 134, 224, 199,
  ]);

  var additionalData = commonFixtures.additionalData;

  //  The length of the tag defaults to 16 bytes (128 bit).
  var tag = {
    128: new Uint8Array([
      180, 165, 14, 180, 121, 113, 220, 168, 254, 117, 18, 66, 110, 98, 146,
      240,
    ]),
    192: new Uint8Array([
      43, 102, 63, 121, 1, 120, 252, 2, 95, 149, 99, 207, 161, 10, 139, 159,
    ]),
    256: new Uint8Array([
      53, 0, 70, 11, 217, 64, 250, 241, 175, 160, 37, 78, 92, 160, 107, 38,
    ]),
  };

  var tag_with_empty_ad = {
    128: new Uint8Array([
      168, 116, 195, 94, 178, 179, 227, 160, 158, 207, 188, 132, 23, 137, 246,
      129,
    ]),
    192: new Uint8Array([
      111, 84, 157, 153, 12, 219, 247, 161, 220, 24, 0, 74, 203, 228, 83, 201,
    ]),
    256: new Uint8Array([
      125, 85, 225, 240, 220, 112, 144, 9, 168, 179, 251, 128, 126, 147, 131,
      244,
    ]),
  };

  // Results. These were created using OpenSSL.

  // AES-GCM produces ciphertext and a tag.
  var ciphertext = {
    128: new Uint8Array([
      46, 244, 139, 198, 120, 180, 9, 39, 83, 58, 203, 107, 69, 71, 8, 165, 132,
      200, 94, 31, 228, 120, 170, 81, 241, 29, 38, 175, 99, 215, 241, 157, 144,
      97, 35, 42, 36, 231, 2, 94, 214, 140, 67, 48, 189, 242, 21, 208, 110, 179,
      30, 90, 181, 105, 242, 17, 244, 42, 42, 36, 125, 228, 82, 250, 87, 199,
      95, 168, 210, 57, 174, 20, 220, 188, 107, 65, 242, 43, 217, 122, 145, 160,
      100, 139, 54, 135, 175, 139, 115, 89, 15, 236, 234, 83, 2, 135, 51, 125,
      63, 168, 184, 235, 148, 68, 132, 124, 166, 171, 53, 68, 94, 187, 31, 68,
      119, 47, 252, 73, 63, 138, 154, 84, 167, 0, 54, 33, 11, 200, 22, 91, 245,
      62, 64, 192, 7, 180, 210, 52, 233, 23, 24, 181, 50, 230, 63, 118, 228, 24,
      1, 242, 75, 62, 196, 222, 122, 154, 227, 125, 89, 73, 112, 100, 154, 249,
      61, 141, 126, 145, 46, 247, 102, 242, 62, 148, 94, 172, 128, 181, 110, 6,
      7, 209, 58, 222, 51, 169, 83, 189, 200, 47, 22, 80, 49, 169, 227, 245,
      165, 24, 96, 152, 228, 14, 252, 199, 193, 148, 46, 84, 49, 248, 198, 7, 0,
      134, 255, 174, 151, 103, 48, 154, 178, 198, 103, 45, 226, 118, 19, 41, 85,
      2, 55, 71, 7, 6, 0, 24, 150, 145, 227, 162, 126, 102, 248, 134, 116, 174,
      215, 217, 166, 160, 140, 129, 21, 220, 131, 110, 242, 94, 249, 103, 151,
      154, 81, 225, 35, 111, 131, 129, 111, 172, 214, 168, 30, 169, 71, 210, 64,
      68, 56, 228, 223, 248, 233, 234, 140, 86, 145, 121, 29, 232, 55, 165, 61,
      175, 147, 66, 33, 92, 6, 209, 241, 149, 73, 77, 9, 104, 2, 154, 247, 92,
      87, 159, 191, 113, 82, 122, 148, 89, 28, 122, 111, 93, 110, 60, 42, 34,
      70, 161, 14, 50, 153, 238, 189, 173, 99, 10, 118, 252, 1, 28, 67, 151,
      114, 46, 78, 181, 78, 233, 183, 6, 254, 57, 29, 53, 118, 175, 80, 97, 156,
      237, 219, 196, 71, 80, 161, 248, 139, 96, 124, 181, 154, 124, 149, 219,
      47, 90, 11, 98, 63, 21, 64, 144, 77, 161, 204, 127, 209, 209, 7, 86, 65,
      39, 142, 251, 183, 43, 227, 120, 155, 72, 70, 204, 89, 227, 199, 203, 28,
      128, 23, 104, 188, 215, 32, 190, 18, 156, 57, 105, 7, 179, 155, 136, 236,
      82, 173, 156, 170, 124, 210, 22, 11, 27, 182, 236, 109, 200, 172, 227, 72,
      37, 1, 175, 9, 214, 227, 23, 141, 169, 215, 77, 134, 76, 229, 169, 241,
      116, 222, 157, 77, 158, 213, 118, 223, 17, 31, 212, 97, 21, 237, 83, 2,
      218, 239, 59, 147, 30, 169, 97, 12,
    ]),

    192: new Uint8Array([
      129, 16, 61, 38, 99, 56, 226, 139, 71, 251, 211, 15, 91, 152, 159, 219,
      112, 147, 210, 73, 97, 204, 203, 240, 183, 243, 104, 241, 37, 67, 169,
      198, 56, 76, 96, 202, 250, 212, 177, 157, 93, 115, 247, 176, 19, 3, 229,
      102, 75, 200, 252, 222, 197, 58, 31, 44, 123, 151, 9, 191, 88, 123, 35,
      48, 47, 25, 149, 35, 191, 219, 223, 94, 251, 152, 109, 171, 225, 31, 236,
      252, 223, 174, 128, 238, 173, 32, 32, 79, 22, 100, 112, 215, 153, 128, 63,
      158, 247, 18, 215, 81, 247, 208, 91, 28, 223, 222, 170, 9, 135, 210, 143,
      47, 247, 132, 183, 252, 84, 19, 78, 85, 17, 215, 20, 51, 32, 124, 149,
      172, 129, 202, 161, 217, 207, 24, 45, 177, 11, 106, 17, 108, 17, 12, 6,
      62, 90, 132, 2, 54, 96, 90, 30, 239, 216, 173, 76, 67, 7, 221, 62, 124,
      228, 156, 243, 31, 111, 160, 192, 188, 87, 107, 182, 138, 95, 122, 152,
      202, 51, 118, 100, 124, 67, 220, 116, 52, 99, 15, 39, 2, 14, 209, 173,
      119, 88, 6, 174, 106, 236, 150, 28, 189, 112, 161, 224, 186, 58, 110, 91,
      54, 211, 132, 149, 7, 188, 77, 232, 118, 197, 43, 107, 101, 179, 44, 195,
      159, 4, 124, 5, 30, 48, 227, 251, 199, 72, 98, 177, 206, 234, 228, 58,
      191, 150, 28, 211, 29, 182, 138, 141, 249, 152, 142, 244, 203, 210, 128,
      143, 244, 44, 187, 251, 221, 101, 152, 31, 119, 194, 51, 27, 167, 215,
      122, 244, 193, 224, 191, 198, 210, 2, 143, 185, 207, 145, 228, 193, 153,
      207, 119, 167, 75, 145, 43, 17, 1, 42, 146, 164, 21, 15, 164, 221, 216,
      140, 122, 248, 49, 19, 246, 84, 214, 176, 226, 118, 140, 130, 123, 163,
      217, 61, 198, 243, 182, 217, 52, 127, 190, 127, 135, 18, 239, 163, 195,
      102, 136, 227, 128, 38, 244, 49, 208, 229, 249, 126, 157, 100, 72, 246,
      10, 102, 163, 241, 155, 112, 165, 95, 32, 61, 66, 24, 233, 123, 236, 190,
      124, 214, 65, 135, 114, 118, 122, 222, 196, 47, 120, 120, 64, 117, 253,
      165, 28, 17, 152, 104, 119, 10, 53, 140, 109, 79, 246, 246, 28, 104, 228,
      175, 102, 71, 246, 183, 79, 30, 31, 186, 32, 64, 146, 72, 228, 1, 175,
      252, 115, 254, 95, 66, 87, 196, 134, 41, 115, 165, 206, 253, 245, 147,
      137, 163, 230, 235, 238, 77, 218, 74, 157, 65, 97, 43, 198, 130, 190, 195,
      142, 22, 166, 4, 179, 184, 167, 254, 156, 243, 38, 46, 66, 68, 252, 252,
      161, 209, 83, 177, 128, 115, 92, 158, 182, 177, 185, 23, 39, 138, 245, 29,
      216, 17, 178, 142, 225, 135, 8, 115,
    ]),

    256: new Uint8Array([
      191, 72, 167, 1, 122, 218, 148, 218, 15, 239, 202, 129, 96, 108, 229, 157,
      138, 161, 232, 71, 80, 188, 118, 61, 75, 105, 120, 201, 14, 102, 102, 240,
      111, 131, 180, 83, 95, 73, 2, 138, 205, 56, 9, 137, 227, 235, 73, 71, 200,
      62, 246, 0, 223, 209, 3, 255, 113, 112, 63, 103, 41, 154, 77, 13, 149, 89,
      94, 79, 132, 193, 114, 40, 158, 33, 55, 242, 130, 109, 136, 69, 124, 130,
      150, 40, 69, 211, 224, 154, 209, 243, 65, 58, 230, 253, 31, 21, 72, 102,
      18, 250, 139, 230, 235, 11, 108, 184, 133, 108, 181, 138, 188, 189, 91,
      91, 115, 216, 68, 9, 229, 30, 154, 132, 118, 219, 183, 235, 177, 197, 221,
      58, 13, 90, 126, 198, 74, 87, 162, 226, 7, 51, 184, 15, 209, 81, 86, 138,
      169, 154, 12, 206, 58, 187, 228, 177, 68, 65, 62, 68, 141, 93, 241, 105,
      29, 239, 20, 102, 222, 49, 209, 18, 162, 247, 200, 240, 122, 244, 204,
      148, 67, 58, 118, 164, 95, 230, 68, 242, 203, 138, 145, 132, 6, 224, 206,
      234, 131, 183, 137, 249, 2, 11, 254, 123, 235, 70, 14, 136, 207, 76, 57,
      22, 38, 49, 197, 219, 123, 43, 241, 191, 64, 211, 152, 178, 140, 165, 1,
      189, 52, 79, 184, 213, 56, 215, 182, 27, 27, 70, 243, 101, 255, 50, 108,
      210, 105, 13, 22, 218, 176, 238, 36, 113, 251, 18, 218, 138, 214, 193, 21,
      122, 224, 125, 118, 134, 161, 174, 130, 86, 233, 149, 151, 33, 31, 88, 63,
      91, 63, 209, 145, 158, 109, 42, 176, 43, 23, 151, 49, 101, 199, 35, 101,
      158, 139, 198, 219, 209, 125, 221, 205, 99, 69, 142, 165, 139, 110, 220,
      184, 226, 238, 149, 161, 175, 171, 167, 170, 65, 19, 156, 166, 219, 231,
      87, 20, 226, 58, 210, 134, 110, 160, 176, 118, 250, 73, 86, 213, 116, 53,
      114, 24, 101, 34, 185, 59, 237, 47, 39, 206, 67, 12, 74, 236, 130, 7, 249,
      217, 203, 245, 122, 14, 230, 53, 203, 126, 93, 131, 51, 2, 0, 231, 161,
      111, 42, 126, 173, 121, 80, 179, 59, 186, 133, 236, 252, 188, 149, 99,
      221, 182, 55, 5, 38, 83, 132, 43, 123, 233, 174, 208, 140, 165, 77, 1,
      202, 46, 6, 183, 207, 246, 125, 37, 110, 226, 61, 155, 194, 198, 153, 107,
      1, 8, 0, 23, 124, 18, 4, 144, 235, 146, 77, 220, 123, 152, 114, 219, 127,
      59, 126, 10, 79, 106, 198, 11, 27, 111, 11, 155, 1, 137, 38, 74, 3, 248,
      225, 221, 203, 86, 4, 148, 25, 88, 144, 185, 38, 114, 139, 48, 74, 82,
      172, 36, 115, 193, 223, 220, 144, 69, 91, 5, 83, 56, 138, 63,
    ]),
  };

  return {
    plaintext,
    keyBytes,
    iv,
    additionalData,
    tag,
    tag_with_empty_ad,
    ciphertext,
  };
}

// ---- end encrypt_decrypt/aes_gcm_96_iv_fixtures.js ----
// ---- inlined WebCryptoAPI/encrypt_decrypt/aes_gcm_vectors.js ----
// aes_gcm_vectors.js

// The following function returns an array of test vectors
// for the subtleCrypto encrypt method.
//
// Each test vector has the following fields:
//     name - a unique name for this vector
//     keyBuffer - an arrayBuffer with the key data in raw form
//     key - a CryptoKey object for the keyBuffer. INITIALLY null! You must fill this in first to use it!
//     algorithm - the value of the AlgorithmIdentifier parameter to provide to encrypt
//     plaintext - the text to encrypt
//     result - the expected result (usually just ciphertext, sometimes with added authentication)
function getTestVectors() {
    const {
      plaintext,
      keyBytes,
      iv,
      additionalData,
      tag,
      tag_with_empty_ad,
      ciphertext,
    } = getFixtures();

    var keyLengths = [128, 192, 256];
    var tagLengths = [32, 64, 96, 104, 112, 120, 128];

    // All the scenarios that should succeed, if the key has "encrypt" usage
    var passing = [];
    keyLengths.forEach(function(keyLength) {
        tagLengths.forEach(function(tagLength) {
            var byteCount = tagLength / 8;

            var result = new Uint8Array(ciphertext[keyLength].byteLength + byteCount);
            result.set(ciphertext[keyLength], 0);
            result.set(tag[keyLength].slice(0, byteCount), ciphertext[keyLength].byteLength);
            passing.push({
                    name: "AES-GCM " + keyLength.toString() + "-bit key, " + tagLength.toString() + "-bit tag, " + (iv.byteLength << 3).toString() + "-bit iv",
                    keyBuffer: keyBytes[keyLength],
                    key: null,
                    algorithm: {name: "AES-GCM", iv: iv, additionalData: additionalData, tagLength: tagLength},
                    plaintext: plaintext,
                    result: result
            });

            var noadresult = new Uint8Array(ciphertext[keyLength].byteLength + byteCount);
            noadresult.set(ciphertext[keyLength], 0);
            noadresult.set(tag_with_empty_ad[keyLength].slice(0, byteCount), ciphertext[keyLength].byteLength);
            passing.push({
                    name: "AES-GCM " + keyLength.toString() + "-bit key, no additional data, " + tagLength.toString() + "-bit tag, " + (iv.byteLength << 3).toString() + "-bit iv",
                    keyBuffer: keyBytes[keyLength],
                    key: null,
                    algorithm: {name: "AES-GCM", iv: iv, tagLength: tagLength},
                    plaintext: plaintext,
                    result: noadresult
            });
        });
    });

    // Scenarios that should fail because of a bad tag length, causing an OperationError
    var failing = [];
    keyLengths.forEach(function(keyLength) {
        [24, 48, 72, 95, 129].forEach(function(badTagLength) {
            failing.push({
                name: "AES-GCM " + keyLength.toString() + "-bit key, " + (iv.byteLength << 3).toString() + "-bit iv, " + "illegal tag length " + badTagLength.toString() + "-bits",
                keyBuffer: keyBytes[keyLength],
                key: null,
                algorithm: {name: "AES-GCM", iv: iv, additionalData: additionalData, tagLength: badTagLength},
                plaintext: plaintext,
                result: ciphertext[keyLength]
            });
        });
    });

    return {passing: passing, failing: failing, decryptionFailing: []};
}

// ---- end encrypt_decrypt/aes_gcm_vectors.js ----
// ---- inlined WebCryptoAPI/encrypt_decrypt/aes.js ----

function run_test() {
    var subtle = self.crypto.subtle; // Change to test prefixed implementations

    // When are all these tests really done? When all the promises they use have resolved.
    var all_promises = [];

    // Source file aes_XXX_vectors.js provides the getTestVectors method
    // for the AES-XXX algorithm that drives these tests.
    var vectors = getTestVectors();
    var passingVectors = vectors.passing;
    var failingVectors = vectors.failing;
    var decryptionFailingVectors = vectors.decryptionFailing;

    // Check for successful encryption.
    passingVectors.forEach(function(vector) {
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.encrypt(vector.algorithm, vector.key, vector.plaintext)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.result), "Should return expected result");
                }, function(err) {
                    assert_unreached("encrypt error for test " + vector.name + ": " + err.message);
                });
            }, vector.name);
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name);
        });

        all_promises.push(promise);
    });

    // Check for successful encryption even if the buffer is changed while calling encrypt.
    passingVectors.forEach(function(vector) {
        var plaintext = copyBuffer(vector.plaintext);
        plaintext[0] = 255 - plaintext[0];
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.encrypt({
                    ...vector.algorithm,
                    get name() {
                        plaintext[0] = vector.plaintext[0];
                        return vector.algorithm.name;
                    }
                }, vector.key, plaintext)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.result), "Should return expected result");
                }, function(err) {
                    assert_unreached("encrypt error for test " + vector.name + ": " + err.message);
                });
                return operation;
            }, vector.name + " with altered plaintext during call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name + " with altered plaintext during call");
        });

        all_promises.push(promise);
    });

    // Check for successful encryption even if the buffer is changed after calling encrypt.
    passingVectors.forEach(function(vector) {
        var plaintext = copyBuffer(vector.plaintext);
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.encrypt(vector.algorithm, vector.key, plaintext)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.result), "Should return expected result");
                }, function(err) {
                    assert_unreached("encrypt error for test " + vector.name + ": " + err.message);
                });
                plaintext[0] = 255 - plaintext[0];
                return operation;
            }, vector.name + " with altered plaintext after call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name + " with altered plaintext after call");
        });

        all_promises.push(promise);
    });

    // Check for encryption of an empty value if the buffer is transferred while calling encrypt.
    passingVectors.forEach(function(vector) {
        var plaintext = copyBuffer(vector.plaintext);
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.encrypt({
                    ...vector.algorithm,
                    get name() {
                        if (!plaintext.buffer.detached) {
                            plaintext.buffer.transfer();
                        }
                        return vector.algorithm.name;
                    }
                }, vector.key, plaintext)
                .then(function(result) {
                    var expectedLength =
                        ["AES-GCM", "AES-OCB"].includes(vector.algorithm.name) ? vector.algorithm.tagLength / 8 :
                        vector.algorithm.name === "AES-CBC" ? 16 :
                        0;
                    assert_equals(result.byteLength, expectedLength, "Transferred plaintext yields an empty ciphertext");
                }, function(err) {
                    assert_unreached("encrypt error for test " + vector.name + ": " + err.message);
                });
                return operation;
            }, vector.name + " with transferred plaintext during call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name + " with transferred plaintext during call");
        });

        all_promises.push(promise);
    });

    // Check for successful encryption even if the buffer is transferred after calling encrypt.
    passingVectors.forEach(function(vector) {
        var plaintext = copyBuffer(vector.plaintext);
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.encrypt(vector.algorithm, vector.key, plaintext)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.result), "Should return expected result");
                }, function(err) {
                    assert_unreached("encrypt error for test " + vector.name + ": " + err.message);
                });
                plaintext.buffer.transfer();
                return operation;
            }, vector.name + " with transferred plaintext after call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name + " with transferred plaintext after call");
        });

        all_promises.push(promise);
    });

    // Check for successful decryption.
    passingVectors.forEach(function(vector) {
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.decrypt(vector.algorithm, vector.key, vector.result)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.plaintext), "Should return expected result");
                }, function(err) {
                    assert_unreached("decrypt error for test " + vector.name + ": " + err.message);
                });
            }, vector.name + " decryption");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step for decryption: " + vector.name);
        });

        all_promises.push(promise);
    });

    // Check for successful decryption even if ciphertext is altered while calling encrypt.
    passingVectors.forEach(function(vector) {
        var ciphertext = copyBuffer(vector.result);
        ciphertext[0] = 255 - ciphertext[0];
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.decrypt({
                    ...vector.algorithm,
                    get name() {
                        ciphertext[0] = vector.result[0];
                        return vector.algorithm.name;
                    }
                }, vector.key, ciphertext)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.plaintext), "Should return expected result");
                }, function(err) {
                    assert_unreached("decrypt error for test " + vector.name + ": " + err.message);
                });
                return operation;
            }, vector.name + " decryption with altered ciphertext during call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step for decryption: " + vector.name + " with altered ciphertext during call");
        });

        all_promises.push(promise);
    });

    // Check for successful decryption even if ciphertext is altered after calling encrypt.
    passingVectors.forEach(function(vector) {
        var ciphertext = copyBuffer(vector.result);
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.decrypt(vector.algorithm, vector.key, ciphertext)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.plaintext), "Should return expected result");
                }, function(err) {
                    assert_unreached("decrypt error for test " + vector.name + ": " + err.message);
                });
                ciphertext[0] = 255 - ciphertext[0];
                return operation;
            }, vector.name + " decryption with altered ciphertext after call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step for decryption: " + vector.name + " with altered ciphertext after call");
        });

        all_promises.push(promise);
    });

    // Check for decryption when ciphertext is transferred while calling decrypt.
    passingVectors.forEach(function(vector) {
        var ciphertext = copyBuffer(vector.result);
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.decrypt({
                    ...vector.algorithm,
                    get name() {
                        if (!ciphertext.buffer.detached) {
                            ciphertext.buffer.transfer();
                        }
                        return vector.algorithm.name;
                    }
                }, vector.key, ciphertext)
                .then(function(result) {
                    if (vector.algorithm.name === "AES-CTR") {
                        assert_equals(result.byteLength, 0, "Transferred ciphertext yields empty plaintext");
                    } else {
                        assert_unreached("decrypt should not have succeeded for " + vector.name);
                    }
                }, function(err) {
                    if (vector.algorithm.name === "AES-CTR") {
                        assert_unreached("decrypt error for test " + vector.name + ": " + err.message);
                    } else {
                        assert_equals(err.name, "OperationError", "Should throw an OperationError instead of " + err.message);
                    }
                });
                return operation;
            }, vector.name + " decryption with transferred ciphertext during call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step for decryption: " + vector.name + " with transferred ciphertext during call");
        });

        all_promises.push(promise);
    });

    // Check for successful decryption even if ciphertext is transferred after calling encrypt.
    passingVectors.forEach(function(vector) {
        var ciphertext = copyBuffer(vector.result);
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                var operation = subtle.decrypt(vector.algorithm, vector.key, ciphertext)
                .then(function(result) {
                    assert_true(equalBuffers(result, vector.plaintext), "Should return expected result");
                }, function(err) {
                    assert_unreached("decrypt error for test " + vector.name + ": " + err.message);
                });
                ciphertext.buffer.transfer();
                return operation;
            }, vector.name + " decryption with transferred ciphertext after call");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step for decryption: " + vector.name + " with transferred ciphertext after call");
        });

        all_promises.push(promise);
    });

    // Everything that succeeded should fail if no "encrypt" usage.
    passingVectors.forEach(function(vector) {
        // Don't want to overwrite key being used for success tests!
        var badVector = Object.assign({}, vector);
        badVector.key = null;

        var promise = importVectorKey(badVector, ["decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.encrypt(vector.algorithm, vector.key, vector.plaintext)
                .then(function(result) {
                    assert_unreached("should have thrown exception for test " + vector.name);
                }, function(err) {
                    assert_equals(err.name, "InvalidAccessError", "Should throw an InvalidAccessError instead of " + err.message)
                });
            }, vector.name + " without encrypt usage");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name + " without encrypt usage");
        });

        all_promises.push(promise);
    });

    // Encryption should fail if algorithm of key doesn't match algorithm of function call.
    passingVectors.forEach(function(vector) {
        var algorithm = Object.assign({}, vector.algorithm);
        if (algorithm.name === "AES-CBC") {
            algorithm.name = "AES-CTR";
            algorithm.counter = new Uint8Array(16);
            algorithm.length = 64;
        } else {
            algorithm.name = "AES-CBC";
            algorithm.iv = new Uint8Array(16); // Need syntactically valid parameter to get to error being checked.
        }

        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.encrypt(algorithm, vector.key, vector.plaintext)
                .then(function(result) {
                    assert_unreached("encrypt succeeded despite mismatch " + vector.name + ": " + err.message);
                }, function(err) {
                    assert_equals(err.name, "InvalidAccessError", "Mismatch should cause InvalidAccessError instead of " + err.message);
                });
            }, vector.name + " with mismatched key and algorithm");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name + " with mismatched key and algorithm");
        });

        all_promises.push(promise);
    });

    // Everything that succeeded decrypting should fail if no "decrypt" usage.
    passingVectors.forEach(function(vector) {
        // Don't want to overwrite key being used for success tests!
        var badVector = Object.assign({}, vector);
        badVector.key = null;

        var promise = importVectorKey(badVector, ["encrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.decrypt(vector.algorithm, vector.key, vector.result)
                .then(function(result) {
                    assert_unreached("should have thrown exception for test " + vector.name);
                }, function(err) {
                    assert_equals(err.name, "InvalidAccessError", "Should throw an InvalidAccessError instead of " + err.message)
                });
            }, vector.name + " without decrypt usage");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name + " without decrypt usage");
        });

        all_promises.push(promise);
    });

    // Check for OperationError due to data lengths.
    failingVectors.forEach(function(vector) {
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.encrypt(vector.algorithm, vector.key, vector.plaintext)
                .then(function(result) {
                    assert_unreached("should have thrown exception for test " + vector.name);
                }, function(err) {
                    assert_equals(err.name, "OperationError", "Should throw an OperationError instead of " + err.message)
                });
            }, vector.name);
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: " + vector.name);
        });

        all_promises.push(promise);
    });

    // Check for OperationError due to data lengths for decryption, too.
    failingVectors.forEach(function(vector) {
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.decrypt(vector.algorithm, vector.key, vector.result)
                .then(function(result) {
                    assert_unreached("should have thrown exception for test " + vector.name);
                }, function(err) {
                    assert_equals(err.name, "OperationError", "Should throw an OperationError instead of " + err.message)
                });
            }, vector.name + " decryption");
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: decryption " + vector.name);
        });

        all_promises.push(promise);
    });

    // Check for decryption failing for algorithm-specific reasons (such as bad
    // padding for AES-CBC).
    decryptionFailingVectors.forEach(function(vector) {
        var promise = importVectorKey(vector, ["encrypt", "decrypt"])
        .then(function(vector) {
            promise_test(function(test) {
                return subtle.decrypt(vector.algorithm, vector.key, vector.result)
                .then(function(result) {
                    assert_unreached("should have thrown exception for test " + vector.name);
                }, function(err) {
                    assert_equals(err.name, "OperationError", "Should throw an OperationError instead of " + err.message)
                });
            }, vector.name);
        }, function(err) {
            // We need a failed test if the importVectorKey operation fails, so
            // we know we never tested encryption
            promise_test(function(test) {
                assert_unreached("importKey failed for " + vector.name);
            }, "importKey step: decryption " + vector.name);
        });

        all_promises.push(promise);
    });

    promise_test(function() {
        return Promise.all(all_promises).finally(done);
    }, "setup");

    // A test vector has all needed fields for encryption, EXCEPT that the
    // key field may be null. This function replaces that null with the Correct
    // CryptoKey object.
    //
    // Returns a Promise that yields an updated vector on success.
    function importVectorKey(vector, usages) {
        if (vector.key !== null) {
            return Promise.resolve(vector);
        } else {
            return subtle.importKey(vector.algorithm.name.toUpperCase() === "AES-OCB" ? "raw-secret" : "raw", vector.keyBuffer, {name: vector.algorithm.name}, false, usages)
            .then(function(key) {
                vector.key = key;
                return vector;
            });
        }
    }

    return;
}

// ---- end encrypt_decrypt/aes.js ----
// META: title=WebCryptoAPI: encrypt() Using AES-GCM w/ 96-bit iv
// META: script=../util/helpers.js
// META: script=aes_common_fixtures.js
// META: script=aes_gcm_96_iv_fixtures.js
// META: script=aes_gcm_vectors.js
// META: script=aes.js
// META: timeout=long

run_test();
