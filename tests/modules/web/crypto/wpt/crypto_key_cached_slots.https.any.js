/*---
flags: [async,onlyStrict]
---*/
// Imported from web-platform-tests WebCryptoAPI/crypto_key_cached_slots.https.any.js (672f0e8 2026-10-17); lines marked MODDABLE are local changes
// META: title=WebCryptoAPI: CryptoKey cached ECMAScript objects

// https://w3c.github.io/webcrypto/#dom-cryptokey-algorithm
// https://github.com/servo/servo/issues/33908

promise_test(function() {
    return self.crypto.subtle.generateKey(
        {
          name: "AES-CTR",
          length: 256,
        },
        true,
        ["encrypt"],
      ).then(
        function(key) {
          let a = key.algorithm;
          let b = key.algorithm;
          assert_true(a === b);
        },
        function(err) {
            assert_unreached("generateKey threw an unexpected error: " + err.toString());
        }
    );
}, "CryptoKey.algorithm getter returns cached object");

promise_test(function() {
    return self.crypto.subtle.generateKey(
        {
          name: "AES-CTR",
          length: 256,
        },
        true,
        ["encrypt"],
      ).then(
        function(key) {
          let a = key.usages;
          let b = key.usages;
          assert_true(a === b);
        },
        function(err) {
            assert_unreached("generateKey threw an unexpected error: " + err.toString());
        }
    );
}, "CryptoKey.usages getter returns cached object");
