/*---
flags: [async,onlyStrict]
---*/
// Imported from web-platform-tests WebCryptoAPI/wrapKey_unwrapKey/wrapKey_unwrapKey.https.any.js (672f0e8 2026-10-17); lines marked MODDABLE are local changes
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
// ---- inlined WebCryptoAPI/wrapKey_unwrapKey/wrapKey_unwrapKey_vectors.js ----
let wrappingKeyData = {
    "RSA": { // modulusLength: 4096, hash: "SHA-256"
        spki: new Uint8Array([48, 130, 2, 34, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1, 5, 0, 3, 130, 2, 15, 0, 48, 130, 2, 10, 2, 130, 2, 1, 0, 218, 170, 246, 76, 189, 156, 216, 153, 155, 176, 221, 14, 44, 132, 103, 104, 0, 127, 100, 166, 245, 248, 104, 125, 31, 74, 155, 226, 90, 193, 184, 54, 170, 145, 111, 222, 20, 252, 19, 248, 146, 44, 190, 115, 73, 188, 52, 251, 4, 178, 121, 238, 212, 204, 34, 62, 122, 100, 203, 111, 233, 231, 210, 73, 53, 146, 147, 211, 14, 161, 109, 137, 212, 175, 226, 18, 183, 173, 103, 103, 30, 128, 31, 218, 69, 126, 234, 65, 88, 231, 160, 91, 51, 245, 77, 54, 4, 167, 192, 33, 68, 244, 163, 242, 187, 111, 209, 180, 241, 221, 107, 172, 5, 40, 134, 47, 210, 85, 8, 112, 57, 186, 29, 131, 176, 93, 116, 198, 202, 82, 108, 251, 209, 3, 72, 75, 143, 59, 44, 222, 56, 89, 69, 103, 159, 211, 160, 19, 214, 173, 77, 133, 0, 68, 219, 164, 79, 64, 238, 65, 189, 201, 248, 173, 180, 146, 196, 238, 86, 232, 215, 109, 39, 165, 162, 16, 230, 46, 134, 234, 148, 106, 34, 230, 198, 63, 231, 143, 16, 179, 208, 109, 22, 100, 54, 156, 107, 132, 28, 208, 118, 205, 217, 89, 228, 75, 196, 169, 181, 5, 85, 157, 144, 110, 129, 186, 141, 119, 104, 162, 206, 170, 115, 7, 96, 82, 240, 33, 143, 81, 243, 215, 67, 96, 137, 207, 209, 22, 162, 251, 108, 208, 232, 32, 236, 205, 167, 174, 161, 116, 13, 249, 187, 22, 240, 185, 172, 160, 103, 94, 162, 147, 26, 15, 143, 183, 147, 98, 231, 117, 134, 185, 50, 64, 40, 30, 27, 13, 152, 132, 40, 138, 32, 78, 158, 162, 207, 212, 229, 210, 251, 88, 116, 67, 229, 164, 164, 147, 59, 32, 94, 217, 197, 242, 149, 102, 74, 219, 46, 127, 68, 28, 116, 10, 2, 249, 231, 130, 123, 29, 45, 73, 56, 17, 195, 208, 45, 25, 60, 252, 98, 189, 109, 25, 0, 253, 151, 254, 124, 211, 48, 23, 156, 78, 163, 154, 188, 17, 69, 14, 188, 16, 64, 59, 190, 136, 70, 162, 253, 237, 156, 111, 41, 27, 40, 63, 205, 204, 94, 0, 50, 237, 62, 87, 211, 115, 91, 68, 194, 104, 119, 72, 106, 226, 160, 48, 165, 138, 134, 2, 138, 153, 181, 38, 249, 48, 120, 72, 15, 245, 227, 15, 164, 64, 188, 74, 4, 84, 213, 83, 67, 73, 87, 181, 72, 94, 46, 54, 193, 252, 188, 14, 207, 28, 82, 159, 131, 168, 238, 168, 145, 28, 230, 27, 126, 151, 93, 5, 96, 68, 126, 66, 174, 155, 101, 123, 20, 218, 131, 92, 124, 78, 82, 44, 55, 139, 77, 105, 177, 136, 121, 177, 43, 77, 12, 240, 0, 76, 20, 133, 121, 129, 73, 15, 160, 200, 150, 114, 95, 59, 59, 165, 240, 204, 13, 156, 134, 194, 4, 70, 158, 213, 111, 229, 103, 216, 239, 132, 16, 184, 151, 206, 254, 229, 62, 23, 58, 125, 49, 144, 208, 215, 2, 3, 1, 0, 1]),
        pkcs8: new Uint8Array([48, 130, 9, 68, 2, 1, 0, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1, 5, 0, 4, 130, 9, 46, 48, 130, 9, 42, 2, 1, 0, 2, 130, 2, 1, 0, 218, 170, 246, 76, 189, 156, 216, 153, 155, 176, 221, 14, 44, 132, 103, 104, 0, 127, 100, 166, 245, 248, 104, 125, 31, 74, 155, 226, 90, 193, 184, 54, 170, 145, 111, 222, 20, 252, 19, 248, 146, 44, 190, 115, 73, 188, 52, 251, 4, 178, 121, 238, 212, 204, 34, 62, 122, 100, 203, 111, 233, 231, 210, 73, 53, 146, 147, 211, 14, 161, 109, 137, 212, 175, 226, 18, 183, 173, 103, 103, 30, 128, 31, 218, 69, 126, 234, 65, 88, 231, 160, 91, 51, 245, 77, 54, 4, 167, 192, 33, 68, 244, 163, 242, 187, 111, 209, 180, 241, 221, 107, 172, 5, 40, 134, 47, 210, 85, 8, 112, 57, 186, 29, 131, 176, 93, 116, 198, 202, 82, 108, 251, 209, 3, 72, 75, 143, 59, 44, 222, 56, 89, 69, 103, 159, 211, 160, 19, 214, 173, 77, 133, 0, 68, 219, 164, 79, 64, 238, 65, 189, 201, 248, 173, 180, 146, 196, 238, 86, 232, 215, 109, 39, 165, 162, 16, 230, 46, 134, 234, 148, 106, 34, 230, 198, 63, 231, 143, 16, 179, 208, 109, 22, 100, 54, 156, 107, 132, 28, 208, 118, 205, 217, 89, 228, 75, 196, 169, 181, 5, 85, 157, 144, 110, 129, 186, 141, 119, 104, 162, 206, 170, 115, 7, 96, 82, 240, 33, 143, 81, 243, 215, 67, 96, 137, 207, 209, 22, 162, 251, 108, 208, 232, 32, 236, 205, 167, 174, 161, 116, 13, 249, 187, 22, 240, 185, 172, 160, 103, 94, 162, 147, 26, 15, 143, 183, 147, 98, 231, 117, 134, 185, 50, 64, 40, 30, 27, 13, 152, 132, 40, 138, 32, 78, 158, 162, 207, 212, 229, 210, 251, 88, 116, 67, 229, 164, 164, 147, 59, 32, 94, 217, 197, 242, 149, 102, 74, 219, 46, 127, 68, 28, 116, 10, 2, 249, 231, 130, 123, 29, 45, 73, 56, 17, 195, 208, 45, 25, 60, 252, 98, 189, 109, 25, 0, 253, 151, 254, 124, 211, 48, 23, 156, 78, 163, 154, 188, 17, 69, 14, 188, 16, 64, 59, 190, 136, 70, 162, 253, 237, 156, 111, 41, 27, 40, 63, 205, 204, 94, 0, 50, 237, 62, 87, 211, 115, 91, 68, 194, 104, 119, 72, 106, 226, 160, 48, 165, 138, 134, 2, 138, 153, 181, 38, 249, 48, 120, 72, 15, 245, 227, 15, 164, 64, 188, 74, 4, 84, 213, 83, 67, 73, 87, 181, 72, 94, 46, 54, 193, 252, 188, 14, 207, 28, 82, 159, 131, 168, 238, 168, 145, 28, 230, 27, 126, 151, 93, 5, 96, 68, 126, 66, 174, 155, 101, 123, 20, 218, 131, 92, 124, 78, 82, 44, 55, 139, 77, 105, 177, 136, 121, 177, 43, 77, 12, 240, 0, 76, 20, 133, 121, 129, 73, 15, 160, 200, 150, 114, 95, 59, 59, 165, 240, 204, 13, 156, 134, 194, 4, 70, 158, 213, 111, 229, 103, 216, 239, 132, 16, 184, 151, 206, 254, 229, 62, 23, 58, 125, 49, 144, 208, 215, 2, 3, 1, 0, 1, 2, 130, 2, 1, 0, 185, 115, 209, 92, 24, 92, 19, 159, 131, 89, 166, 193, 68, 164, 46, 135, 24, 20, 243, 42, 94, 230, 4, 200, 73, 103, 159, 121, 131, 251, 83, 222, 153, 30, 171, 191, 176, 16, 114, 103, 152, 161, 118, 12, 148, 246, 152, 0, 100, 101, 113, 224, 74, 125, 174, 117, 74, 156, 125, 165, 54, 189, 179, 172, 255, 80, 135, 42, 178, 247, 217, 204, 209, 163, 49, 155, 42, 72, 88, 176, 46, 63, 255, 195, 192, 184, 248, 183, 223, 76, 226, 197, 54, 245, 206, 60, 8, 10, 181, 122, 1, 223, 113, 196, 133, 143, 58, 77, 185, 235, 78, 76, 32, 59, 212, 66, 110, 162, 75, 123, 210, 153, 180, 58, 97, 179, 129, 60, 175, 142, 228, 123, 85, 50, 241, 119, 147, 204, 94, 43, 65, 163, 4, 167, 243, 247, 41, 134, 105, 197, 165, 63, 45, 145, 56, 174, 203, 192, 135, 209, 29, 195, 83, 179, 14, 184, 131, 104, 152, 48, 245, 179, 207, 178, 60, 23, 21, 1, 84, 207, 82, 124, 9, 137, 171, 141, 187, 55, 172, 180, 180, 10, 48, 185, 97, 79, 156, 39, 249, 192, 27, 98, 77, 250, 93, 18, 157, 130, 72, 210, 115, 96, 36, 132, 116, 101, 225, 96, 234, 79, 89, 243, 89, 135, 97, 252, 53, 72, 97, 34, 226, 41, 41, 45, 144, 243, 189, 162, 243, 43, 69, 136, 143, 182, 140, 223, 134, 93, 38, 245, 36, 125, 46, 93, 48, 94, 215, 39, 156, 57, 86, 93, 207, 204, 72, 106, 112, 215, 203, 230, 80, 20, 137, 224, 242, 33, 146, 33, 108, 188, 185, 254, 117, 189, 240, 82, 64, 60, 186, 247, 190, 138, 170, 159, 147, 75, 49, 148, 101, 174, 130, 21, 177, 211, 121, 6, 153, 144, 230, 166, 181, 155, 94, 232, 2, 4, 119, 236, 35, 133, 253, 223, 14, 30, 199, 57, 215, 31, 251, 90, 167, 19, 231, 154, 54, 225, 85, 68, 17, 234, 158, 53, 50, 243, 182, 149, 193, 214, 60, 188, 6, 38, 2, 200, 161, 232, 193, 30, 153, 231, 221, 57, 140, 55, 69, 35, 21, 153, 34, 238, 175, 65, 253, 210, 119, 125, 120, 116, 153, 127, 67, 204, 9, 66, 210, 200, 165, 212, 216, 2, 62, 19, 15, 171, 77, 183, 247, 127, 224, 138, 41, 208, 170, 227, 36, 158, 176, 111, 128, 172, 70, 73, 241, 148, 172, 50, 174, 126, 80, 177, 235, 93, 89, 102, 84, 76, 221, 30, 216, 49, 125, 142, 35, 45, 96, 224, 60, 161, 63, 48, 85, 143, 20, 76, 182, 111, 15, 156, 139, 55, 155, 113, 226, 248, 239, 130, 252, 241, 197, 247, 124, 61, 39, 197, 170, 119, 76, 136, 195, 180, 169, 106, 240, 234, 101, 114, 207, 11, 160, 170, 139, 194, 187, 48, 22, 114, 84, 64, 151, 30, 212, 99, 213, 176, 106, 79, 232, 127, 197, 153, 133, 8, 56, 210, 83, 67, 106, 124, 231, 96, 2, 145, 2, 130, 1, 1, 0, 244, 218, 215, 194, 174, 36, 99, 217, 1, 4, 236, 11, 160, 86, 85, 65, 206, 36, 36, 143, 205, 108, 166, 191, 91, 209, 75, 117, 7, 81, 33, 179, 44, 101, 145, 215, 39, 117, 195, 81, 31, 111, 36, 7, 26, 105, 30, 249, 91, 2, 2, 237, 126, 141, 231, 153, 213, 181, 100, 234, 219, 192, 114, 179, 215, 229, 39, 212, 107, 9, 55, 220, 136, 233, 237, 28, 74, 97, 6, 22, 26, 47, 150, 83, 82, 95, 186, 146, 22, 38, 176, 231, 255, 166, 199, 223, 217, 86, 142, 56, 43, 199, 25, 247, 249, 122, 59, 142, 152, 20, 49, 147, 13, 132, 249, 203, 251, 146, 116, 96, 88, 81, 232, 45, 106, 100, 187, 99, 73, 32, 203, 134, 30, 223, 100, 179, 179, 128, 81, 242, 25, 85, 137, 125, 96, 153, 240, 224, 86, 20, 206, 24, 26, 197, 233, 164, 158, 50, 222, 103, 197, 211, 144, 101, 182, 205, 201, 51, 23, 231, 125, 229, 130, 61, 139, 204, 195, 243, 69, 38, 185, 187, 48, 249, 140, 107, 137, 39, 234, 21, 13, 43, 24, 112, 108, 109, 15, 25, 57, 55, 127, 40, 152, 238, 227, 96, 86, 157, 114, 35, 52, 54, 38, 140, 85, 42, 119, 53, 99, 35, 133, 208, 240, 65, 171, 8, 71, 255, 243, 248, 176, 166, 17, 178, 92, 62, 203, 56, 158, 31, 169, 223, 123, 7, 118, 216, 166, 132, 83, 62, 112, 160, 99, 244, 132, 29, 2, 130, 1, 1, 0, 228, 158, 249, 243, 243, 94, 42, 189, 87, 61, 152, 139, 197, 122, 33, 97, 4, 39, 135, 66, 219, 225, 11, 70, 103, 92, 115, 10, 8, 225, 5, 2, 220, 32, 23, 147, 56, 111, 237, 98, 48, 174, 122, 207, 109, 152, 187, 125, 220, 186, 73, 127, 42, 82, 39, 228, 163, 12, 188, 36, 71, 107, 52, 235, 223, 200, 7, 38, 6, 167, 28, 158, 26, 213, 126, 186, 90, 152, 133, 44, 53, 156, 61, 130, 92, 163, 3, 27, 35, 185, 141, 112, 236, 246, 210, 107, 75, 245, 33, 126, 134, 215, 41, 1, 244, 220, 36, 93, 22, 232, 50, 62, 68, 141, 153, 118, 62, 1, 167, 197, 202, 113, 187, 196, 186, 251, 161, 128, 66, 211, 145, 103, 133, 69, 207, 155, 117, 65, 76, 251, 125, 43, 224, 105, 171, 6, 29, 254, 31, 111, 144, 5, 158, 166, 180, 143, 163, 205, 212, 151, 7, 11, 50, 234, 82, 37, 143, 75, 104, 124, 97, 69, 220, 246, 202, 45, 25, 40, 220, 23, 92, 116, 112, 114, 204, 198, 140, 48, 111, 191, 53, 28, 9, 134, 234, 90, 168, 243, 108, 75, 197, 99, 162, 173, 31, 194, 97, 224, 184, 76, 227, 170, 199, 106, 129, 14, 77, 234, 231, 38, 192, 197, 233, 174, 150, 240, 55, 252, 241, 27, 97, 169, 49, 49, 115, 9, 218, 65, 253, 14, 253, 217, 91, 141, 44, 68, 32, 247, 219, 199, 31, 45, 212, 68, 46, 131, 2, 130, 1, 1, 0, 225, 142, 199, 187, 155, 88, 2, 114, 225, 49, 123, 144, 170, 63, 93, 130, 165, 55, 62, 71, 10, 97, 208, 169, 239, 23, 58, 127, 176, 33, 216, 253, 137, 36, 119, 216, 207, 140, 248, 68, 62, 196, 207, 87, 139, 200, 210, 179, 186, 86, 124, 3, 243, 213, 29, 72, 229, 73, 152, 145, 145, 166, 19, 4, 1, 26, 36, 58, 213, 239, 67, 250, 112, 85, 174, 11, 165, 169, 3, 70, 81, 17, 13, 85, 236, 72, 43, 66, 112, 13, 108, 98, 11, 107, 196, 44, 61, 182, 50, 133, 36, 46, 225, 137, 65, 212, 140, 16, 171, 159, 206, 155, 60, 149, 6, 216, 22, 3, 176, 25, 32, 195, 51, 50, 195, 19, 208, 91, 129, 254, 39, 254, 129, 106, 33, 6, 57, 145, 55, 235, 225, 210, 158, 57, 85, 71, 250, 81, 110, 122, 243, 239, 216, 154, 0, 197, 152, 198, 27, 131, 85, 5, 179, 187, 63, 79, 10, 205, 122, 115, 209, 210, 30, 204, 59, 128, 129, 242, 19, 253, 188, 146, 232, 102, 186, 40, 69, 204, 243, 34, 57, 99, 61, 188, 50, 229, 180, 70, 244, 34, 95, 141, 50, 116, 190, 24, 253, 49, 68, 247, 145, 29, 97, 29, 93, 71, 37, 81, 148, 230, 32, 91, 125, 55, 193, 42, 123, 201, 25, 34, 58, 248, 128, 204, 225, 149, 38, 248, 29, 17, 230, 22, 236, 234, 207, 92, 124, 232, 225, 22, 96, 2, 32, 146, 27, 49, 2, 130, 1, 1, 0, 129, 62, 34, 61, 183, 242, 31, 37, 68, 193, 108, 144, 111, 133, 248, 130, 184, 239, 131, 182, 215, 72, 164, 176, 27, 84, 151, 48, 48, 14, 205, 95, 109, 131, 178, 240, 38, 50, 152, 55, 47, 32, 36, 11, 73, 128, 211, 85, 118, 199, 213, 46, 207, 132, 252, 74, 115, 166, 138, 97, 212, 2, 22, 59, 214, 25, 101, 121, 40, 191, 166, 28, 247, 60, 132, 84, 227, 76, 95, 212, 187, 69, 229, 59, 226, 20, 193, 119, 193, 61, 111, 105, 76, 124, 200, 61, 162, 6, 36, 246, 59, 82, 61, 59, 126, 234, 72, 160, 91, 135, 206, 135, 135, 7, 169, 158, 191, 180, 253, 220, 129, 242, 195, 220, 150, 124, 20, 51, 199, 19, 133, 154, 201, 43, 203, 14, 174, 61, 201, 64, 78, 229, 212, 10, 200, 133, 63, 197, 94, 142, 26, 20, 35, 57, 72, 207, 255, 33, 40, 50, 108, 231, 246, 211, 162, 182, 219, 8, 29, 60, 91, 93, 60, 106, 67, 167, 53, 22, 245, 61, 59, 166, 19, 191, 194, 101, 231, 240, 165, 235, 169, 33, 125, 125, 72, 213, 17, 183, 243, 27, 238, 173, 193, 212, 47, 37, 27, 98, 7, 174, 103, 242, 46, 163, 213, 235, 121, 62, 247, 135, 223, 232, 194, 143, 81, 130, 225, 147, 219, 213, 199, 226, 247, 13, 102, 100, 70, 127, 145, 136, 189, 22, 248, 123, 153, 111, 182, 87, 136, 102, 76, 9, 3, 123, 187, 243, 2, 130, 1, 0, 36, 121, 149, 41, 189, 115, 193, 110, 98, 69, 30, 145, 9, 231, 177, 98, 120, 118, 126, 102, 62, 220, 58, 207, 73, 211, 60, 15, 24, 107, 208, 95, 29, 107, 40, 190, 182, 84, 106, 17, 217, 198, 210, 27, 233, 227, 153, 252, 128, 181, 44, 145, 101, 156, 7, 209, 23, 149, 66, 78, 109, 145, 138, 13, 241, 174, 198, 3, 26, 222, 15, 241, 120, 176, 54, 190, 97, 80, 215, 99, 49, 62, 204, 135, 226, 32, 141, 102, 251, 32, 152, 108, 113, 237, 59, 142, 30, 185, 195, 135, 145, 1, 86, 115, 56, 253, 215, 186, 221, 202, 196, 36, 227, 118, 177, 130, 60, 59, 56, 190, 198, 157, 142, 18, 96, 43, 218, 199, 150, 42, 174, 44, 198, 65, 103, 139, 167, 177, 46, 26, 155, 248, 209, 56, 155, 209, 204, 42, 89, 224, 212, 75, 80, 135, 106, 203, 4, 81, 181, 85, 128, 247, 73, 134, 41, 48, 183, 57, 127, 28, 234, 26, 244, 177, 159, 113, 90, 249, 120, 32, 248, 134, 79, 99, 123, 155, 173, 201, 185, 216, 166, 32, 152, 181, 6, 154, 118, 18, 181, 245, 106, 25, 37, 146, 118, 16, 215, 30, 83, 96, 35, 154, 93, 0, 13, 5, 206, 156, 129, 147, 118, 87, 248, 155, 49, 135, 7, 39, 157, 226, 171, 96, 16, 112, 122, 173, 58, 145, 19, 6, 90, 11, 221, 109, 208, 16, 251, 188, 18, 120, 106, 170, 143, 149, 79, 192]),
        jwk: {
            kty: "RSA",
            n: "2qr2TL2c2JmbsN0OLIRnaAB_ZKb1-Gh9H0qb4lrBuDaqkW_eFPwT-JIsvnNJvDT7BLJ57tTMIj56ZMtv6efSSTWSk9MOoW2J1K_iEretZ2cegB_aRX7qQVjnoFsz9U02BKfAIUT0o_K7b9G08d1rrAUohi_SVQhwObodg7BddMbKUmz70QNIS487LN44WUVnn9OgE9atTYUARNukT0DuQb3J-K20ksTuVujXbSelohDmLobqlGoi5sY_548Qs9BtFmQ2nGuEHNB2zdlZ5EvEqbUFVZ2QboG6jXdoos6qcwdgUvAhj1Hz10Ngic_RFqL7bNDoIOzNp66hdA35uxbwuaygZ16ikxoPj7eTYud1hrkyQCgeGw2YhCiKIE6eos_U5dL7WHRD5aSkkzsgXtnF8pVmStsuf0QcdAoC-eeCex0tSTgRw9AtGTz8Yr1tGQD9l_580zAXnE6jmrwRRQ68EEA7vohGov3tnG8pGyg_zcxeADLtPlfTc1tEwmh3SGrioDClioYCipm1JvkweEgP9eMPpEC8SgRU1VNDSVe1SF4uNsH8vA7PHFKfg6juqJEc5ht-l10FYER-Qq6bZXsU2oNcfE5SLDeLTWmxiHmxK00M8ABMFIV5gUkPoMiWcl87O6XwzA2chsIERp7Vb-Vn2O-EELiXzv7lPhc6fTGQ0Nc",
            e: "AQAB",
            d: "uXPRXBhcE5-DWabBRKQuv,hxgU8ype5gTISWefeYP7U96ZHqu_sBByZ5ihdgyU9pgAZGVx4Ep9rnVKnH2lNr2zrP9Qhyqy99nM0aMxmypIWLAuP__DwLj4t99M4sU29c48CAq1egHfccSFjzpNuetOTCA71EJuokt70pm0OmGzgTyvjuR7VTLxd5PMXitBowSn8_cphmnFpT8tkTiuy8CH0R3DU7MOuINomDD1s8-yPBcVAVTPUnwJiauNuzestLQKMLlhT5wn-cAbYk36XRKdgkjSc2AkhHRl4WDqT1nzWYdh_DVIYSLiKSktkPO9ovMrRYiPtozfhl0m9SR9Ll0wXtcnnDlWXc_MSGpw18vmUBSJ4PIhkiFsvLn-db3wUkA8uve-iqqfk0sxlGWughWx03kGmZDmprWbXugCBHfsI4X93w4exznXH_tapxPnmjbhVUQR6p41MvO2lcHWPLwGJgLIoejBHpnn3TmMN0UjFZki7q9B_dJ3fXh0mX9DzAlC0sil1NgCPhMPq02393_giinQquMknrBvgKxGSfGUrDKuflCx611ZZlRM3R7YMX2OIy1g4DyhPzBVjxRMtm8PnIs3m3Hi-O-C_PHF93w9J8Wqd0yIw7SpavDqZXLPC6Cqi8K7MBZyVECXHtRj1bBqT-h_xZmFCDjSU0NqfOdgApE",
            p: "9NrXwq4kY9kBBOwLoFZVQc4kJI_NbKa_W9FLdQdRIbMsZZHXJ3XDUR9vJAcaaR75WwIC7X6N55nVtWTq28Bys9flJ9RrCTfciOntHEphBhYaL5ZTUl-6khYmsOf_psff2VaOOCvHGff5ejuOmBQxkw2E-cv7knRgWFHoLWpku2NJIMuGHt9ks7OAUfIZVYl9YJnw4FYUzhgaxemknjLeZ8XTkGW2zckzF-d95YI9i8zD80Umubsw-YxriSfqFQ0rGHBsbQ8ZOTd_KJju42BWnXIjNDYmjFUqdzVjI4XQ8EGrCEf_8_iwphGyXD7LOJ4fqd97B3bYpoRTPnCgY_SEHQ",
            q: "5J758_NeKr1XPZiLxXohYQQnh0Lb4QtGZ1xzCgjhBQLcIBeTOG_tYjCues9tmLt93LpJfypSJ-SjDLwkR2s069_IByYGpxyeGtV-ulqYhSw1nD2CXKMDGyO5jXDs9tJrS_UhfobXKQH03CRdFugyPkSNmXY-AafFynG7xLr7oYBC05FnhUXPm3VBTPt9K-BpqwYd_h9vkAWeprSPo83UlwcLMupSJY9LaHxhRdz2yi0ZKNwXXHRwcszGjDBvvzUcCYbqWqjzbEvFY6KtH8Jh4LhM46rHaoEOTernJsDF6a6W8Df88RthqTExcwnaQf0O_dlbjSxEIPfbxx8t1EQugw",
            dp: "4Y7Hu5tYAnLhMXuQqj9dgqU3PkcKYdCp7xc6f7Ah2P2JJHfYz4z4RD7Ez1eLyNKzulZ8A_PVHUjlSZiRkaYTBAEaJDrV70P6cFWuC6WpA0ZREQ1V7EgrQnANbGILa8QsPbYyhSQu4YlB1IwQq5_OmzyVBtgWA7AZIMMzMsMT0FuB_if-gWohBjmRN-vh0p45VUf6UW568-_YmgDFmMYbg1UFs7s_TwrNenPR0h7MO4CB8hP9vJLoZrooRczzIjljPbwy5bRG9CJfjTJ0vhj9MUT3kR1hHV1HJVGU5iBbfTfBKnvJGSI6-IDM4ZUm-B0R5hbs6s9cfOjhFmACIJIbMQ",
            dq: "gT4iPbfyHyVEwWyQb4X4grjvg7bXSKSwG1SXMDAOzV9tg7LwJjKYNy8gJAtJgNNVdsfVLs-E_Epzpoph1AIWO9YZZXkov6Yc9zyEVONMX9S7ReU74hTBd8E9b2lMfMg9ogYk9jtSPTt-6kigW4fOh4cHqZ6_tP3cgfLD3JZ8FDPHE4WaySvLDq49yUBO5dQKyIU_xV6OGhQjOUjP_yEoMmzn9tOittsIHTxbXTxqQ6c1FvU9O6YTv8Jl5_Cl66khfX1I1RG38xvurcHULyUbYgeuZ_Iuo9XreT73h9_owo9RguGT29XH4vcNZmRGf5GIvRb4e5lvtleIZkwJA3u78w",
            qi: "JHmVKb1zwW5iRR6RCeexYnh2fmY-3DrPSdM8Dxhr0F8dayi-tlRqEdnG0hvp45n8gLUskWWcB9EXlUJObZGKDfGuxgMa3g_xeLA2vmFQ12MxPsyH4iCNZvsgmGxx7TuOHrnDh5EBVnM4_de63crEJON2sYI8Ozi-xp2OEmAr2seWKq4sxkFni6exLhqb-NE4m9HMKlng1EtQh2rLBFG1VYD3SYYpMLc5fxzqGvSxn3Fa-Xgg-IZPY3ubrcm52KYgmLUGmnYStfVqGSWSdhDXHlNgI5pdAA0FzpyBk3ZX-JsxhwcnneKrYBBweq06kRMGWgvdbdAQ-7wSeGqqj5VPwA"
        },
    },
    "SYMMETRIC128": {
        raw: new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]),
    },
    "SYMMETRIC256": {
        raw: new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32]),
    }
}

let toWrapKeyData = {
    "RSA": { //modulusLength: 1024, hash: "SHA-256"
        spki: new Uint8Array([48, 129, 159, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1, 5, 0, 3, 129, 141, 0, 48, 129, 137, 2, 129, 129, 0, 205, 153, 248, 177, 17, 159, 141, 10, 44, 231, 172, 139, 253, 12, 181, 71, 211, 72, 249, 49, 204, 156, 92, 167, 159, 222, 32, 229, 28, 64, 235, 1, 171, 38, 30, 1, 37, 61, 241, 232, 143, 113, 208, 134, 233, 75, 122, 190, 119, 131, 145, 3, 164, 118, 190, 224, 204, 135, 199, 67, 21, 26, 253, 68, 49, 250, 93, 143, 160, 81, 39, 28, 245, 78, 73, 207, 117, 0, 216, 169, 149, 126, 192, 155, 157, 67, 239, 112, 9, 140, 87, 241, 13, 3, 191, 211, 23, 72, 175, 86, 59, 136, 22, 135, 114, 13, 60, 123, 16, 161, 205, 85, 58, 199, 29, 41, 107, 110, 222, 236, 165, 185, 156, 138, 251, 54, 221, 151, 2, 3, 1, 0, 1]),
        pkcs8: new Uint8Array([48, 130, 2, 120, 2, 1, 0, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1, 5, 0, 4, 130, 2, 98, 48, 130, 2, 94, 2, 1, 0, 2, 129, 129, 0, 205, 153, 248, 177, 17, 159, 141, 10, 44, 231, 172, 139, 253, 12, 181, 71, 211, 72, 249, 49, 204, 156, 92, 167, 159, 222, 32, 229, 28, 64, 235, 1, 171, 38, 30, 1, 37, 61, 241, 232, 143, 113, 208, 134, 233, 75, 122, 190, 119, 131, 145, 3, 164, 118, 190, 224, 204, 135, 199, 67, 21, 26, 253, 68, 49, 250, 93, 143, 160, 81, 39, 28, 245, 78, 73, 207, 117, 0, 216, 169, 149, 126, 192, 155, 157, 67, 239, 112, 9, 140, 87, 241, 13, 3, 191, 211, 23, 72, 175, 86, 59, 136, 22, 135, 114, 13, 60, 123, 16, 161, 205, 85, 58, 199, 29, 41, 107, 110, 222, 236, 165, 185, 156, 138, 251, 54, 221, 151, 2, 3, 1, 0, 1, 2, 129, 128, 98, 162, 10, 252, 103, 71, 243, 145, 126, 25, 102, 93, 129, 248, 38, 191, 94, 77, 19, 191, 32, 57, 162, 249, 135, 104, 56, 191, 176, 222, 51, 223, 137, 11, 176, 57, 60, 116, 139, 40, 214, 39, 243, 177, 197, 25, 192, 184, 190, 253, 15, 4, 128, 81, 183, 32, 128, 254, 98, 73, 124, 70, 134, 88, 228, 85, 8, 229, 210, 6, 149, 141, 122, 147, 24, 166, 42, 57, 218, 125, 240, 230, 232, 249, 81, 145, 44, 6, 118, 237, 101, 205, 4, 181, 104, 85, 23, 96, 46, 169, 174, 213, 110, 34, 171, 89, 196, 20, 18, 1, 8, 241, 93, 32, 19, 144, 248, 183, 32, 96, 240, 101, 239, 247, 222, 249, 117, 1, 2, 65, 0, 244, 26, 192, 131, 146, 245, 205, 250, 134, 62, 229, 137, 14, 224, 194, 5, 127, 147, 154, 214, 93, 172, 226, 55, 98, 206, 25, 104, 223, 178, 48, 249, 83, 143, 5, 146, 16, 243, 180, 170, 119, 227, 17, 151, 48, 217, 88, 23, 30, 2, 73, 153, 181, 92, 163, 164, 241, 114, 66, 66, 152, 70, 42, 121, 2, 65, 0, 215, 158, 227, 12, 157, 88, 107, 153, 230, 66, 244, 207, 110, 18, 128, 60, 7, 140, 90, 136, 49, 11, 38, 144, 78, 64, 107, 167, 125, 41, 16, 167, 122, 152, 100, 129, 223, 206, 97, 170, 190, 1, 34, 79, 44, 221, 254, 204, 117, 122, 76, 249, 68, 169, 105, 152, 20, 161, 62, 40, 255, 101, 68, 143, 2, 65, 0, 169, 215, 127, 65, 76, 220, 104, 31, 186, 142, 66, 168, 213, 72, 62, 215, 18, 136, 2, 0, 203, 22, 194, 35, 37, 69, 31, 90, 223, 226, 28, 191, 45, 139, 98, 165, 217, 211, 167, 77, 192, 178, 166, 7, 155, 62, 110, 83, 79, 86, 234, 28, 223, 154, 128, 102, 0, 116, 174, 115, 165, 125, 148, 137, 2, 65, 0, 132, 212, 95, 192, 228, 169, 148, 215, 225, 46, 252, 75, 80, 222, 218, 218, 160, 55, 201, 137, 190, 212, 196, 179, 255, 80, 214, 64, 254, 236, 174, 82, 206, 70, 85, 28, 96, 248, 109, 216, 86, 102, 178, 113, 30, 13, 192, 42, 202, 112, 70, 61, 5, 28, 108, 109, 128, 191, 248, 96, 31, 61, 142, 103, 2, 65, 0, 205, 186, 73, 64, 8, 98, 158, 188, 82, 109, 82, 177, 5, 13, 132, 100, 97, 84, 15, 103, 183, 88, 37, 219, 0, 148, 88, 166, 79, 7, 85, 14, 64, 3, 157, 142, 132, 164, 226, 112, 236, 158, 218, 17, 7, 158, 184, 41, 20, 172, 194, 242, 44, 231, 78, 192, 134, 220, 83, 36, 191, 7, 35, 225]),
        jwk: {
            kty: "RSA",
            n: "zZn4sRGfjQos56yL_Qy1R9NI-THMnFynn94g5RxA6wGrJh4BJT3x6I9x0IbpS3q-d4ORA6R2vuDMh8dDFRr9RDH6XY-gUScc9U5Jz3UA2KmVfsCbnUPvcAmMV_ENA7_TF0ivVjuIFodyDTx7EKHNVTrHHSlrbt7spbmcivs23Zc",
            e: "AQAB",
            d: "YqIK_GdH85F-GWZdgfgmv15NE78gOaL5h2g4v7DeM9-JC7A5PHSLKNYn87HFGcC4vv0PBIBRtyCA_mJJfEaGWORVCOXSBpWNepMYpio52n3w5uj5UZEsBnbtZc0EtWhVF2Auqa7VbiKrWcQUEgEI8V0gE5D4tyBg8GXv9975dQE",
            p: "9BrAg5L1zfqGPuWJDuDCBX-TmtZdrOI3Ys4ZaN-yMPlTjwWSEPO0qnfjEZcw2VgXHgJJmbVco6TxckJCmEYqeQ",
            q: "157jDJ1Ya5nmQvTPbhKAPAeMWogxCyaQTkBrp30pEKd6mGSB385hqr4BIk8s3f7MdXpM-USpaZgUoT4o_2VEjw",
            dp: "qdd_QUzcaB-6jkKo1Ug-1xKIAgDLFsIjJUUfWt_iHL8ti2Kl2dOnTcCypgebPm5TT1bqHN-agGYAdK5zpX2UiQ",
            dq: "hNRfwOSplNfhLvxLUN7a2qA3yYm-1MSz_1DWQP7srlLORlUcYPht2FZmsnEeDcAqynBGPQUcbG2Av_hgHz2OZw",
            qi: "zbpJQAhinrxSbVKxBQ2EZGFUD2e3WCXbAJRYpk8HVQ5AA52OhKTicOye2hEHnrgpFKzC8iznTsCG3FMkvwcj4Q"
        }
    },
    "EC": {
        spki: new Uint8Array([48, 89, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 3, 66, 0, 4, 210, 16, 176, 166, 249, 217, 240, 18, 134, 128, 88, 180, 63, 164, 244, 113, 1, 133, 67, 187, 160, 12, 146, 80, 223, 146, 87, 194, 172, 174, 93, 209, 206, 3, 117, 82, 212, 129, 69, 12, 227, 155, 77, 16, 149, 112, 27, 23, 91, 250, 179, 75, 142, 108, 9, 158, 24, 241, 193, 152, 53, 131, 97, 232]),
        pkcs8: new Uint8Array([48, 129, 135, 2, 1, 0, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 4, 109, 48, 107, 2, 1, 1, 4, 32, 19, 211, 58, 45, 90, 191, 156, 249, 235, 178, 31, 248, 96, 212, 174, 254, 110, 86, 231, 119, 144, 244, 222, 233, 180, 8, 132, 235, 211, 53, 68, 234, 161, 68, 3, 66, 0, 4, 210, 16, 176, 166, 249, 217, 240, 18, 134, 128, 88, 180, 63, 164, 244, 113, 1, 133, 67, 187, 160, 12, 146, 80, 223, 146, 87, 194, 172, 174, 93, 209, 206, 3, 117, 82, 212, 129, 69, 12, 227, 155, 77, 16, 149, 112, 27, 23, 91, 250, 179, 75, 142, 108, 9, 158, 24, 241, 193, 152, 53, 131, 97, 232]),
        jwk: {
            kty: "EC",
            crv: "P-256",
            x: "0hCwpvnZ8BKGgFi0P6T0cQGFQ7ugDJJQ35JXwqyuXdE",
            y: "zgN1UtSBRQzjm00QlXAbF1v6s0uObAmeGPHBmDWDYeg",
            d: "E9M6LVq_nPnrsh_4YNSu_m5W53eQ9N7ptAiE69M1ROo"
        }
    },
    "ED25519": okpKeyData.Ed25519,
    "ED448": okpKeyData.Ed448,
    "X25519": okpKeyData.X25519,
    "X448": okpKeyData.X448,
    "SYMMETRIC128": {
        raw: new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]),
    },
    "SYMMETRIC256": {
        raw: new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32]),
    }
}

function toWrapKeyDataFromAlg(alg) {
    switch(alg) {
    case "RSASSA-PKCS1-v1_5":
    case "RSA-PSS":
    case "RSA-OAEP":
        return toWrapKeyData["RSA"];
    case "ECDSA":
    case "ECDH":
        return toWrapKeyData["EC"];
    case "Ed25519":
        return toWrapKeyData["ED25519"];
    case "X25519":
        return toWrapKeyData["X25519"];
    case "Ed448":
        return toWrapKeyData["ED448"];
    case "X448":
        return toWrapKeyData["X448"];
    default:
        return undefined;
    }
}

// ---- end wrapKey_unwrapKey/wrapKey_unwrapKey_vectors.js ----
// META: title=WebCryptoAPI: wrapKey() and unwrapKey()
// META: timeout=long
// META: script=../util/helpers.js
// META: script=../util/okp_key_fixtures.js
// META: script=wrapKey_unwrapKey_vectors.js

// Tests for wrapKey and unwrapKey round tripping

    var subtle = self.crypto.subtle;

    var wrappers = {};  // Things we wrap (and upwrap) keys with
    var keys = {};      // Things to wrap and unwrap

    // There are five algorithms that can be used for wrapKey/unwrapKey.
    // Generate one key with typical parameters for each kind.
    //
    // Note: we don't need cryptographically strong parameters for things
    // like IV - just any legal value will do.
    var wrappingKeysParameters = [
        {
            name: "RSA-OAEP",
            importParameters: {name: "RSA-OAEP", hash: "SHA-256"},
            wrapParameters: {name: "RSA-OAEP", label: new Uint8Array(8)}
        },
        {
            name: "AES-CTR",
            importParameters: {name: "AES-CTR", length: 128},
            wrapParameters: {name: "AES-CTR", counter: new Uint8Array(16), length: 64}
        },
        {
            name: "AES-CBC",
            importParameters: {name: "AES-CBC", length: 128},
            wrapParameters: {name: "AES-CBC", iv: new Uint8Array(16)}
        },
        {
            name: "AES-GCM",
            importParameters: {name: "AES-GCM", length: 128},
            wrapParameters: {name: "AES-GCM", iv: new Uint8Array(16), additionalData: new Uint8Array(16), tagLength: 128}
        },
        {
            name: "AES-KW",
            importParameters: {name: "AES-KW", length: 128},
            wrapParameters: {name: "AES-KW"}
        },
        {
            name: 'ChaCha20-Poly1305',
            importParameters: {name: "ChaCha20-Poly1305"},
            wrapParameters: {name: "ChaCha20-Poly1305", iv: new Uint8Array(12), additionalData: new Uint8Array(16)}
        }
    ];

    var keysToWrapParameters = [
        {algorithm: {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, privateUsages: ["sign"], publicUsages: ["verify"]},
        {algorithm: {name: "RSA-PSS", hash: "SHA-256"}, privateUsages: ["sign"], publicUsages: ["verify"]},
        {algorithm: {name: "RSA-OAEP", hash: "SHA-256"}, privateUsages: ["decrypt"], publicUsages: ["encrypt"]},
        {algorithm: {name: "ECDSA", namedCurve: "P-256"}, privateUsages: ["sign"], publicUsages: ["verify"]},
        {algorithm: {name: "ECDH", namedCurve: "P-256"}, privateUsages: ["deriveBits"], publicUsages: []},
        {algorithm: {name: "Ed25519" }, privateUsages: ["sign"], publicUsages: ["verify"]},
        {algorithm: {name: "Ed448" }, privateUsages: ["sign"], publicUsages: ["verify"]},
        {algorithm: {name: "X25519" }, privateUsages: ["deriveBits"], publicUsages: []},
        {algorithm: {name: "X448" }, privateUsages: ["deriveBits"], publicUsages: []},
        {algorithm: {name: "AES-CTR", length: 128}, usages: ["encrypt", "decrypt"]},
        {algorithm: {name: "AES-CBC", length: 128}, usages: ["encrypt", "decrypt"]},
        {algorithm: {name: "AES-GCM", length: 128}, usages: ["encrypt", "decrypt"]},
        {algorithm: {name: "AES-KW", length: 128}, usages: ["wrapKey", "unwrapKey"]},
        {algorithm: {name: "HMAC", length: 128, hash: "SHA-256"}, usages: ["sign", "verify"]},
        {algorithm: {name: "ChaCha20-Poly1305"}, usages: ['encrypt', 'decrypt']}
    ];

    // MODDABLE: only the symmetric algorithms are implemented
    wrappingKeysParameters = wrappingKeysParameters.filter(p => ["AES-CTR", "AES-CBC", "AES-GCM", "AES-KW", "RSA-OAEP"].includes(p.name));
    keysToWrapParameters = keysToWrapParameters.filter(p => ["AES-CTR", "AES-CBC", "AES-GCM", "AES-KW", "HMAC", "ECDSA", "ECDH", "X25519"].includes(p.algorithm.name));

    // Import all the keys needed, then iterate over all combinations
    // to test wrapping and unwrapping.
    promise_test(function() {
    return Promise.all([importWrappingKeys(), importKeysToWrap()])
    .then(function(results) {
        wrappingKeysParameters.filter((param) => Object.keys(wrappers).includes(param.name)).forEach(function(wrapperParam) {
            var wrapper = wrappers[wrapperParam.name];
            keysToWrapParameters.filter((param) => Object.keys(keys).includes(param.algorithm.name)).forEach(function(toWrapParam) {
                var keyData = keys[toWrapParam.algorithm.name];
                ["raw", "raw-secret", "spki", "pkcs8"].filter((fmt) => Object.keys(keyData).includes(fmt)).forEach(function(keyDataFormat) {
                    var toWrap = keyData[keyDataFormat];
                    [keyDataFormat, "jwk"].forEach(function(format) {
                        if (wrappingIsPossible(toWrap.originalExport[format], wrapper.parameters.name)) {
                            testWrapping(wrapper, toWrap, format);
                            if (canCompareNonExtractableKeys(toWrap.key)) {
                                testWrappingNonExtractable(wrapper, toWrap, format);
                                if (format === "jwk") {
                                    testWrappingNonExtractableAsExtractable(wrapper, toWrap);
                                }
                            }
                        }
                    });
                });
            });
        });
        return Promise.resolve("setup done");
    }, function(err) {
        return Promise.reject("setup failed: " + err.name + ': "' + err.message + '"');
    });
    }, "setup");

    function importWrappingKeys() {
        // Using allSettled to skip unsupported test cases.
        var promises = [];
        wrappingKeysParameters.forEach(function(params) {
            if (params.name === "RSA-OAEP") { // we have a key pair, not just a key
                var algorithm = {name: "RSA-OAEP", hash: "SHA-256"};
                wrappers[params.name] = {wrappingKey: undefined, unwrappingKey: undefined, parameters: params};
                promises.push(subtle.importKey("spki", wrappingKeyData["RSA"].spki, algorithm, true, ["wrapKey"])
                              .then(function(key) {
                                  wrappers["RSA-OAEP"].wrappingKey = key;
                              }));
                promises.push(subtle.importKey("pkcs8", wrappingKeyData["RSA"].pkcs8, algorithm, true, ["unwrapKey"])
                              .then(function(key) {
                                  wrappers["RSA-OAEP"].unwrappingKey = key;
                              }));
            } else if (params.name === "ChaCha20-Poly1305") {
                var algorithm = {name: params.name};
                promises.push(subtle.importKey("raw-secret", wrappingKeyData["SYMMETRIC256"].raw, algorithm, true, ["wrapKey", "unwrapKey"])
                              .then(function(key) {
                                  wrappers[params.name] = {wrappingKey: key, unwrappingKey: key, parameters: params};
                              }));
            } else {
                var algorithm = {name: params.name};
                promises.push(subtle.importKey("raw", wrappingKeyData["SYMMETRIC128"].raw, algorithm, true, ["wrapKey", "unwrapKey"])
                              .then(function(key) {
                                  wrappers[params.name] = {wrappingKey: key, unwrappingKey: key, parameters: params};
                              }));
            }
        });
        // Using allSettled to skip unsupported test cases.
        return Promise.allSettled(promises);
    }

    async function importAndExport(format, keyData, algorithm, keyUsages, keyType) {
        var importedKey;
        try {
            importedKey = await subtle.importKey(format, keyData, algorithm, true, keyUsages);
            keys[algorithm.name][format] = { name: algorithm.name + " " + keyType, algorithm: algorithm, usages: keyUsages, key: importedKey, originalExport: {} };
        } catch (err) {
            delete keys[algorithm.name][format];
            throw("Error importing " + algorithm.name + " " + keyType + " key in '" + format + "' -" + err.name + ': "' + err.message + '"');
        };
        try {
            var exportedKey = await subtle.exportKey(format, importedKey);
            keys[algorithm.name][format].originalExport[format] = exportedKey;
        } catch (err) {
            delete keys[algorithm.name][format];
            throw("Error exporting " + algorithm.name + " '" + format + "' key -" + err.name + ': "' + err.message + '"');
        };
        try {
            var jwkExportedKey = await subtle.exportKey("jwk", importedKey);
            keys[algorithm.name][format].originalExport["jwk"] = jwkExportedKey;
        } catch (err) {
            delete keys[algorithm.name][format];
            throw("Error exporting " + algorithm.name + " '" + format + "' key to 'jwk' -" + err.name + ': "' + err.message + '"');
        };
    }

    function importKeysToWrap() {
        var promises = [];
        keysToWrapParameters.forEach(function(params) {
            if ("publicUsages" in params) {
                keys[params.algorithm.name] = {};
                var keyData = toWrapKeyDataFromAlg(params.algorithm.name);
                promises.push(importAndExport("spki", keyData.spki, params.algorithm, params.publicUsages, "public key "));
                promises.push(importAndExport("pkcs8", keyData.pkcs8, params.algorithm, params.privateUsages, "private key "));
            } else if (params.algorithm.name === "ChaCha20-Poly1305") {
                keys[params.algorithm.name] = {};
                promises.push(importAndExport("raw-secret", toWrapKeyData["SYMMETRIC256"].raw, params.algorithm, params.usages, ""));
            } else {
                keys[params.algorithm.name] = {};
                promises.push(importAndExport("raw", toWrapKeyData["SYMMETRIC128"].raw, params.algorithm, params.usages, ""));
            }
        });
        // Using allSettled to skip unsupported test cases.
        return Promise.allSettled(promises);
    }

    // Can we successfully "round-trip" (wrap, then unwrap, a key)?
    function testWrapping(wrapper, toWrap, fmt) {
        promise_test(async() => {
            try {
                var wrappedResult = await subtle.wrapKey(fmt, toWrap.key, wrapper.wrappingKey, wrapper.parameters.wrapParameters);
                var unwrappedResult = await subtle.unwrapKey(fmt, wrappedResult, wrapper.unwrappingKey, wrapper.parameters.wrapParameters, toWrap.algorithm, true, toWrap.usages);
                assert_goodCryptoKey(unwrappedResult, toWrap.algorithm, true, toWrap.usages, toWrap.key.type);
                var roundTripExport = await subtle.exportKey(fmt, unwrappedResult);
                assert_true(equalExport(toWrap.originalExport[fmt], roundTripExport), "Post-wrap export matches original export");
            } catch (err) {
                if (err instanceof AssertionError) {
                    throw err;
                }
                assert_unreached("Round trip for extractable key threw an error - " + err.name + ': "' + err.message + '"');
            }
        }, "Can wrap and unwrap " + toWrap.name + "keys using " + fmt + " and " + wrapper.parameters.name);
    }

    function testWrappingNonExtractable(wrapper, toWrap, fmt) {
        promise_test(async() => {
            try {
                var wrappedResult = await subtle.wrapKey(fmt, toWrap.key, wrapper.wrappingKey, wrapper.parameters.wrapParameters);
                var unwrappedResult = await subtle.unwrapKey(fmt, wrappedResult, wrapper.unwrappingKey, wrapper.parameters.wrapParameters, toWrap.algorithm, false, toWrap.usages);
                assert_goodCryptoKey(unwrappedResult, toWrap.algorithm, false, toWrap.usages, toWrap.key.type);
                var result = await equalKeys(toWrap.key, unwrappedResult);
                assert_true(result, "Unwrapped key matches original");
            } catch (err) {
                if (err instanceof AssertionError) {
                    throw err;
                }
                assert_unreached("Round trip for key unwrapped non-extractable threw an error - " + err.name + ': "' + err.message + '"');
            };
        }, "Can wrap and unwrap " + toWrap.name + "keys as non-extractable using " + fmt + " and " + wrapper.parameters.name);
    }

    function testWrappingNonExtractableAsExtractable(wrapper, toWrap) {
        promise_test(async() => {
            var wrappedKey;
            try {
                var wrappedResult = await wrapAsNonExtractableJwk(toWrap.key,wrapper);
                wrappedKey = wrappedResult;
                var unwrappedResult = await subtle.unwrapKey("jwk", wrappedKey, wrapper.unwrappingKey, wrapper.parameters.wrapParameters, toWrap.algorithm, false, toWrap.usages);
                assert_false(unwrappedResult.extractable, "Unwrapped key is non-extractable");
                var result = await equalKeys(toWrap.key,unwrappedResult);
                assert_true(result, "Unwrapped key matches original");
            } catch (err) {
                if (err instanceof AssertionError) {
                    throw err;
                }
                assert_unreached("Round trip for non-extractable key threw an error - " + err.name + ': "' + err.message + '"');
            };
            try {
                var unwrappedResult = await subtle.unwrapKey("jwk", wrappedKey, wrapper.unwrappingKey, wrapper.parameters.wrapParameters, toWrap.algorithm, true, toWrap.usages);
                assert_unreached("Unwrapping a non-extractable JWK as extractable should fail");
            } catch (err) {
                if (err instanceof AssertionError) {
                    throw err;
                }
                assert_equals(err.name, "DataError", "Unwrapping a non-extractable JWK as extractable fails with DataError");
            }
        }, "Can unwrap " + toWrap.name + "non-extractable keys using jwk and " + wrapper.parameters.name);
    }

    // Implement key wrapping by hand to wrap a key as non-extractable JWK
    async function wrapAsNonExtractableJwk(key, wrapper) {
        var wrappingKey = wrapper.wrappingKey,
            encryptKey;

        var jwkWrappingKey = await subtle.exportKey("jwk",wrappingKey);
        // Update the key generation parameters to work as key import parameters
        var params = Object.create(wrapper.parameters.importParameters);
        if(params.name === "AES-KW") {
            params.name = "AES-CBC";
            jwkWrappingKey.alg = "A"+params.length+"CBC";
        } else if (params.name === "RSA-OAEP") {
            params.modulusLength = undefined;
            params.publicExponent = undefined;
        }
        jwkWrappingKey.key_ops = ["encrypt"];
        var importedWrappingKey = await subtle.importKey("jwk", jwkWrappingKey, params, true, ["encrypt"]);
        encryptKey = importedWrappingKey;
        var exportedKey = await subtle.exportKey("jwk",key);
        exportedKey.ext = false;
        var jwk = JSON.stringify(exportedKey)
        var result;
        if (wrappingKey.algorithm.name === "AES-KW") {
            result = await aeskw(encryptKey, str2ab(jwk.slice(0,-1) + " ".repeat(jwk.length%8 ? 8-jwk.length%8 : 0) + "}"));
        } else {
            result = await subtle.encrypt(wrapper.parameters.wrapParameters,encryptKey,str2ab(jwk));
        }
        return result;
    }


    // RSA-OAEP can only wrap relatively small payloads. AES-KW can only
    // wrap payloads a multiple of 8 bytes long.
    function wrappingIsPossible(exportedKey, algorithmName) {
        if ("byteLength" in exportedKey && algorithmName === "AES-KW") {
            return exportedKey.byteLength % 8 === 0;
        }

        if ("byteLength" in exportedKey && algorithmName === "RSA-OAEP") {
            // RSA-OAEP can only encrypt payloads with lengths shorter
            // than modulusLength - 2*hashLength - 1 bytes long. For
            // a 4096 bit modulus and SHA-256, that comes to
            // 4096/8 - 2*(256/8) - 1 = 512 - 2*32 - 1 = 447 bytes.
            return exportedKey.byteLength <= 446;
        }

        if ("kty" in exportedKey && algorithmName === "AES-KW") {
            return JSON.stringify(exportedKey).length % 8 === 0;
        }

        if ("kty" in exportedKey && algorithmName === "RSA-OAEP") {
            return JSON.stringify(exportedKey).length <= 478;
        }

        return true;
    }


    // Helper methods follow:

    // Are two exported keys equal
    function equalExport(originalExport, roundTripExport) {
        if ("byteLength" in originalExport) {
            return equalBuffers(originalExport, roundTripExport);
        } else {
            return equalJwk(originalExport, roundTripExport);
        }
    }

    // Can we compare key values by using them
    function canCompareNonExtractableKeys(key){
        if (key.usages.indexOf("decrypt") !== -1) {
            return true;
        }
        if (key.usages.indexOf("sign") !== -1) {
            return true;
        }
        if (key.usages.indexOf("wrapKey") !== -1) {
            return true;
        }
        if (key.usages.indexOf("deriveBits") !== -1) {
            return true;
        }
        return false;
    }

    // Compare two keys by using them (works for non-extractable keys)
    async function equalKeys(expected, got){
        if ( expected.algorithm.name !== got.algorithm.name ) {
            return false;
        }

        var cryptParams, signParams, wrapParams, deriveParams;
        switch(expected.algorithm.name){
            case "AES-CTR" :
                cryptParams = {name: "AES-CTR", counter: new Uint8Array(16), length: 64};
                break;
            case "AES-CBC" :
                cryptParams = {name: "AES-CBC", iv: new Uint8Array(16) };
                break;
            case "AES-GCM" :
                cryptParams = {name: "AES-GCM", iv: new Uint8Array(16) };
                break;
            case "RSA-OAEP" :
                cryptParams = {name: "RSA-OAEP", label: new Uint8Array(8) };
                break;
            case "RSASSA-PKCS1-v1_5" :
                signParams = {name: "RSASSA-PKCS1-v1_5"};
                break;
            case "RSA-PSS" :
                signParams = {name: "RSA-PSS", saltLength: 32 };
                break;
            case "ECDSA" :
                signParams = {name: "ECDSA", hash: "SHA-256"};
                break;
            case "Ed25519" :
                signParams = {name: "Ed25519"};
                break;
            case "Ed448" :
                signParams = {name: "Ed448"};
                break;
            case "X25519" :
                deriveParams = {name: "X25519"};
                break;
            case "X448" :
                deriveParams = {name: "X448"};
                break;
            case "HMAC" :
                signParams = {name: "HMAC"};
                break;
            case "AES-KW" :
                wrapParams = {name: "AES-KW"};
                break;
            case "ECDH" :
                deriveParams = {name: "ECDH"};
                break;
            case "ChaCha20-Poly1305":
                cryptParams = {name: "ChaCha20-Poly1305", iv: new Uint8Array(12)};
                break;
            default:
                throw new Error("Unsupported algorithm for key comparison");
        }

        if (cryptParams) {
            var jwkExpectedKey = await subtle.exportKey("jwk", expected);
            if (expected.algorithm.name === "RSA-OAEP") {
                ["d","p","q","dp","dq","qi","oth"].forEach(function(field){ delete jwkExpectedKey[field]; });
            }
            jwkExpectedKey.key_ops = ["encrypt"];
            var expectedEncryptKey = await subtle.importKey("jwk", jwkExpectedKey, expected.algorithm, true, ["encrypt"]);
            var encryptedData = await subtle.encrypt(cryptParams, expectedEncryptKey, new Uint8Array(32));
            var decryptedData = await subtle.decrypt(cryptParams, got, encryptedData);
            var result = new Uint8Array(decryptedData);
            return !result.some(x => x);
        } else if (signParams) {
            var verifyKey;
            var jwkExpectedKey = await subtle.exportKey("jwk",expected);
            if (expected.algorithm.name === "RSA-PSS" || expected.algorithm.name === "RSASSA-PKCS1-v1_5") {
                ["d","p","q","dp","dq","qi","oth"].forEach(function(field){ delete jwkExpectedKey[field]; });
            }
            if (expected.algorithm.name === "ECDSA" || expected.algorithm.name.startsWith("Ed")) {
                delete jwkExpectedKey["d"];
            }
            jwkExpectedKey.key_ops = ["verify"];
            var expectedVerifyKey = await subtle.importKey("jwk", jwkExpectedKey, expected.algorithm, true, ["verify"]);
            verifyKey = expectedVerifyKey;
            var signature = await subtle.sign(signParams, got, new Uint8Array(32));
            var result = await subtle.verify(signParams, verifyKey, signature, new Uint8Array(32));
            return result;
        } else if (wrapParams) {
            var aKeyToWrap, wrappedWithExpected;
            var key = await subtle.importKey("raw",new Uint8Array(16), "AES-CBC", true, ["encrypt"])
            aKeyToWrap = key;
            var wrapResult = await subtle.wrapKey("raw", aKeyToWrap, expected, wrapParams);
            wrappedWithExpected = Array.from((new Uint8Array(wrapResult)).values());
            wrapResult = await subtle.wrapKey("raw", aKeyToWrap, got, wrapParams);
            var wrappedWithGot = Array.from((new Uint8Array(wrapResult)).values());
            return wrappedWithGot.every((x,i) => x === wrappedWithExpected[i]);
        } else if (deriveParams) {
            var expectedDerivedBits;
            var key = await subtle.generateKey(expected.algorithm, true, ['deriveBits']);
            deriveParams.public = key.publicKey;
            var result = await subtle.deriveBits(deriveParams, expected, 128);
            expectedDerivedBits = Array.from((new Uint8Array(result)).values());
            result = await subtle.deriveBits(deriveParams, got, 128);
            var gotDerivedBits = Array.from((new Uint8Array(result)).values());
            return gotDerivedBits.every((x,i) => x === expectedDerivedBits[i]);
        }
    }

    // Raw AES encryption
    async function aes(k, p) {
        const ciphertext = await subtle.encrypt({ name: "AES-CBC", iv: new Uint8Array(16) }, k, p);
        return ciphertext.slice(0, 16);
    }

    // AES Key Wrap
    async function aeskw(key, data) {
        if (data.byteLength % 8 !== 0) {
            throw new Error("AES Key Wrap data must be a multiple of 8 bytes in length");
        }

        var A = Uint8Array.from([0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0, 0, 0, 0, 0, 0, 0, 0]),
            Av = new DataView(A.buffer),
            R = [],
            n = data.byteLength / 8;

        for(var i = 0; i<data.byteLength; i+=8) {
            R.push(new Uint8Array(data.slice(i,i+8)));
        }

        async function aeskw_step(j, i, final, B) {
            A.set(new Uint8Array(B.slice(0,8)));
            Av.setUint32(4,Av.getUint32(4) ^ (n*j+i+1));
            R[i] = new Uint8Array(B.slice(8,16));
            if (final) {
                R.unshift(A.slice(0,8));
                var result = new Uint8Array(R.length * 8);
                R.forEach(function(Ri,i){ result.set(Ri, i*8); });
                return result;
            } else {
                A.set(R[(i+1)%n],8);
                return aes(key,A);
            }
        }

        A.set(R[0], 8);
        let B = await aes(key, A);

        for(var j=0;j<6;++j) {
            for(var i=0;i<n;++i) {
                B = await aeskw_step(j, i, j === 5 && i === (n - 1), B);
            }
        }

        return B;
    }

    function str2ab(str)        { return Uint8Array.from( str.split(''), function(s){return s.charCodeAt(0)} ); }
    function ab2str(ab)         { return String.fromCharCode.apply(null, new Uint8Array(ab)); }
