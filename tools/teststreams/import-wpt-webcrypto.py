#!/usr/bin/env python3
#
# Copyright (c) 2026  Moddable Tech, Inc.
#
#   This file is part of the Moddable SDK Tools.
#
#   The Moddable SDK Tools is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   The Moddable SDK Tools is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
#
# Usage: python3 -I import-wpt-webcrypto.py <path to a web-platform-tests checkout> $MODDABLE/tests/modules/web/crypto/wpt
#
# Run the result with the teststreams app:  cd $MODDABLE/tools/teststreams && mcconfig -dl -m -p sim   then   test modules/web/crypto/wpt
# The PBKDF2 cases with 1000 and 100000 iterations run only when the app is built with slowTests=true.
import sys, os, re, subprocess
WPT, OUT = sys.argv[1], sys.argv[2]
API = os.path.join(WPT, "WebCryptoAPI")
commit = subprocess.check_output(["git", "-C", WPT, "log", "-1", "--format=%h %cd", "--date=short"]).decode().strip()
SHARED = {"../util/helpers.js", "../../util/helpers.js", "/common/subset-tests.js"}
TESTS = [
    "digest/digest.https.any.js",
    "encrypt_decrypt/aes_cbc.https.any.js", "encrypt_decrypt/aes_ctr.https.any.js",
    "encrypt_decrypt/aes_gcm.https.any.js", "encrypt_decrypt/aes_gcm_256_iv.https.any.js",
    "sign_verify/hmac.https.any.js",
    "derive_bits_keys/pbkdf2.https.any.js", "derive_bits_keys/hkdf.https.any.js", "derive_bits_keys/derived_bits_length.https.any.js",
    "generateKey/successes_AES-CBC.https.any.js", "generateKey/successes_AES-CTR.https.any.js", "generateKey/successes_AES-GCM.https.any.js",
    "generateKey/successes_AES-KW.https.any.js", "generateKey/successes_HMAC.https.any.js",
    "generateKey/failures_AES-CBC.https.any.js", "generateKey/failures_AES-CTR.https.any.js", "generateKey/failures_AES-GCM.https.any.js",
    "generateKey/failures_AES-KW.https.any.js", "generateKey/failures_HMAC.https.any.js", "generateKey/failures_bad_algorithm.https.any.js",
    "import_export/symmetric_importKey.https.any.js",
    "wrapKey_unwrapKey/wrapKey_unwrapKey.https.any.js",
    "import_export/rsa_importKey.https.any.js",
    "sign_verify/rsa_pkcs.https.any.js", "sign_verify/rsa_pss.https.any.js",
    "encrypt_decrypt/rsa_oaep.https.any.js",
    "getRandomValues.any.js", "randomUUID.https.any.js",
    "crypto_key_cached_slots.https.any.js", "normalize-algorithm-name.https.any.js",
    "export-key-error-order.https.any.js",
    "sign_verify/ecdsa.https.any.js",
    "derive_bits_keys/ecdh_bits.https.any.js", "derive_bits_keys/ecdh_keys.https.any.js",
    "derive_bits_keys/cfrg_curves_bits_curve25519.https.any.js", "derive_bits_keys/cfrg_curves_keys_curve25519.https.any.js",
    "derive_bits_keys/derive_key_and_encrypt.https.any.js",
    "import_export/ec_importKey.https.any.js", "import_export/ec_importKey_failures_ECDH.https.any.js", "import_export/ec_importKey_failures_ECDSA.https.any.js",
    "import_export/okp_importKey_X25519.https.any.js", "import_export/okp_importKey_failures_X25519.https.any.js", "import_export/okp_fixed_length.https.any.js",
    "generateKey/successes_ECDH.https.any.js", "generateKey/successes_ECDSA.https.any.js", "generateKey/successes_X25519.https.any.js",
    "generateKey/failures_ECDH.https.any.js", "generateKey/failures_ECDSA.https.any.js", "generateKey/failures_X25519.https.any.js",
    # not imported: sharedarraybuffer (needs WebAssembly.Memory), historical (secure-context behavior), the Ed25519/Ed448 and X448 suites (not implemented), P-521 is expected to fail inside the EC suites
]
# (file containing the text, old, new, reason) — local modifications, each marked in the output
PATCHES = {
    "derive_bits_keys/pbkdf2.js": [(
        "                        Object.keys(derivations[passwordSize][saltSize][hashName]).forEach(function(iterations) {\n",
        "                        Object.keys(derivations[passwordSize][saltSize][hashName]).forEach(function(iterations) {\n                            if (!globalThis.slowTests && (parseInt(iterations) > 1)) return;	// MODDABLE: the 1000 and 100000 iteration cases only when slowTests (minutes in JavaScript)\n",
        "skip 1000 and 100000-iteration cases unless slowTests")],
    "derive_bits_keys/derived_bits_length.js": [(
        "    Object.keys(testCases).forEach(algorithm => {\n",
        "    Object.keys(testCases).filter(algorithm => [\"HKDF\", \"PBKDF2\", \"ECDH\", \"X25519\"].includes(algorithm)).forEach(algorithm => {	// MODDABLE: no X448\n",
        "X448 is not implemented")],
    "derive_bits_keys/ecdh.js": [(
        "    const curves = Object.keys(fixtures.sizes);\n",
        "    const curves = Object.keys(fixtures.sizes).filter(curve => \"P-521\" !== curve);	// MODDABLE: P-521 is not implemented; its import would reject the whole setup\n",
        "P-521 is not implemented")],
    "wrapKey_unwrapKey/wrapKey_unwrapKey.https.any.js": [(
        "    // Import all the keys needed, then iterate over all combinations\n",
        "    // MODDABLE: only the symmetric algorithms are implemented\n    wrappingKeysParameters = wrappingKeysParameters.filter(p => [\"AES-CTR\", \"AES-CBC\", \"AES-GCM\", \"AES-KW\", \"RSA-OAEP\"].includes(p.name));\n    keysToWrapParameters = keysToWrapParameters.filter(p => [\"AES-CTR\", \"AES-CBC\", \"AES-GCM\", \"AES-KW\", \"HMAC\", \"ECDSA\", \"ECDH\", \"X25519\"].includes(p.algorithm.name));\n\n    // Import all the keys needed, then iterate over all combinations\n",
        "Ed25519, Ed448, X448 and ChaCha20-Poly1305 are not implemented; RSA keys cannot be generated")],
}
def patched(rel, text):
    for old, new, reason in PATCHES.get(rel, []):
        assert old in text, (rel, reason)
        text = text.replace(old, new)
    return text
for rel in TESTS:
    src = open(os.path.join(API, rel)).read()
    base = os.path.dirname(rel)
    out = ["/*---", "flags: [async,onlyStrict]", "---*/", f"// Imported from web-platform-tests WebCryptoAPI/{rel} ({commit}); lines marked MODDABLE are local changes"]
    inlined = []
    for m in re.finditer(r"^// META: script=(.+)$", src, flags=re.M):
        script = m.group(1).strip()
        if script in SHARED:
            continue
        srel = os.path.normpath(os.path.join(base, script))
        text = patched(srel, open(os.path.join(API, srel)).read())
        inlined.append(f"// ---- inlined WebCryptoAPI/{srel} ----\n{text}\n// ---- end {srel} ----\n")
    body = patched(rel, src)
    # WPT splits long suites with "META: variant=?a-b"; emit one file per variant, selecting the range through location.search
    variants = re.findall(r"^// META: variant=(\S+)$", src, flags=re.M) or [None]
    for variant in variants:
        dest = os.path.join(OUT, rel)
        prefix = ""
        if variant:
            dest = dest.replace(".https.any.js", f"-{variant[1:].replace('-', '-')}.https.any.js")
            prefix = f'location.search = "{variant}";	// MODDABLE: this WPT variant\n'
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        open(dest, "w").write("\n".join(out) + "\n" + prefix + "".join(inlined) + body)
        print(f"{os.path.getsize(dest):7d} {os.path.relpath(dest, OUT)}")
