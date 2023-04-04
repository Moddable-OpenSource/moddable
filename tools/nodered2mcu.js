/*
 * Copyright (c) 2022-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import { FILE, TOOL } from "tool";

//	Regular Expression by Mathias Bynens
//		https://github.com/mathiasbynens/mothereff.in/blob/master/js-properties/eff.js
//		https://github.com/mathiasbynens/mothereff.in/blob/master/LICENSE-MIT.txt
const regexIdentifierNameES6 = /^(?:[\$A-Z_a-z\xAA\xB5\xBA\xC0-\xD6\xD8-\xF6\xF8-\u02C1\u02C6-\u02D1\u02E0-\u02E4\u02EC\u02EE\u0370-\u0374\u0376\u0377\u037A-\u037D\u037F\u0386\u0388-\u038A\u038C\u038E-\u03A1\u03A3-\u03F5\u03F7-\u0481\u048A-\u052F\u0531-\u0556\u0559\u0561-\u0587\u05D0-\u05EA\u05F0-\u05F2\u0620-\u064A\u066E\u066F\u0671-\u06D3\u06D5\u06E5\u06E6\u06EE\u06EF\u06FA-\u06FC\u06FF\u0710\u0712-\u072F\u074D-\u07A5\u07B1\u07CA-\u07EA\u07F4\u07F5\u07FA\u0800-\u0815\u081A\u0824\u0828\u0840-\u0858\u08A0-\u08B2\u0904-\u0939\u093D\u0950\u0958-\u0961\u0971-\u0980\u0985-\u098C\u098F\u0990\u0993-\u09A8\u09AA-\u09B0\u09B2\u09B6-\u09B9\u09BD\u09CE\u09DC\u09DD\u09DF-\u09E1\u09F0\u09F1\u0A05-\u0A0A\u0A0F\u0A10\u0A13-\u0A28\u0A2A-\u0A30\u0A32\u0A33\u0A35\u0A36\u0A38\u0A39\u0A59-\u0A5C\u0A5E\u0A72-\u0A74\u0A85-\u0A8D\u0A8F-\u0A91\u0A93-\u0AA8\u0AAA-\u0AB0\u0AB2\u0AB3\u0AB5-\u0AB9\u0ABD\u0AD0\u0AE0\u0AE1\u0B05-\u0B0C\u0B0F\u0B10\u0B13-\u0B28\u0B2A-\u0B30\u0B32\u0B33\u0B35-\u0B39\u0B3D\u0B5C\u0B5D\u0B5F-\u0B61\u0B71\u0B83\u0B85-\u0B8A\u0B8E-\u0B90\u0B92-\u0B95\u0B99\u0B9A\u0B9C\u0B9E\u0B9F\u0BA3\u0BA4\u0BA8-\u0BAA\u0BAE-\u0BB9\u0BD0\u0C05-\u0C0C\u0C0E-\u0C10\u0C12-\u0C28\u0C2A-\u0C39\u0C3D\u0C58\u0C59\u0C60\u0C61\u0C85-\u0C8C\u0C8E-\u0C90\u0C92-\u0CA8\u0CAA-\u0CB3\u0CB5-\u0CB9\u0CBD\u0CDE\u0CE0\u0CE1\u0CF1\u0CF2\u0D05-\u0D0C\u0D0E-\u0D10\u0D12-\u0D3A\u0D3D\u0D4E\u0D60\u0D61\u0D7A-\u0D7F\u0D85-\u0D96\u0D9A-\u0DB1\u0DB3-\u0DBB\u0DBD\u0DC0-\u0DC6\u0E01-\u0E30\u0E32\u0E33\u0E40-\u0E46\u0E81\u0E82\u0E84\u0E87\u0E88\u0E8A\u0E8D\u0E94-\u0E97\u0E99-\u0E9F\u0EA1-\u0EA3\u0EA5\u0EA7\u0EAA\u0EAB\u0EAD-\u0EB0\u0EB2\u0EB3\u0EBD\u0EC0-\u0EC4\u0EC6\u0EDC-\u0EDF\u0F00\u0F40-\u0F47\u0F49-\u0F6C\u0F88-\u0F8C\u1000-\u102A\u103F\u1050-\u1055\u105A-\u105D\u1061\u1065\u1066\u106E-\u1070\u1075-\u1081\u108E\u10A0-\u10C5\u10C7\u10CD\u10D0-\u10FA\u10FC-\u1248\u124A-\u124D\u1250-\u1256\u1258\u125A-\u125D\u1260-\u1288\u128A-\u128D\u1290-\u12B0\u12B2-\u12B5\u12B8-\u12BE\u12C0\u12C2-\u12C5\u12C8-\u12D6\u12D8-\u1310\u1312-\u1315\u1318-\u135A\u1380-\u138F\u13A0-\u13F4\u1401-\u166C\u166F-\u167F\u1681-\u169A\u16A0-\u16EA\u16EE-\u16F8\u1700-\u170C\u170E-\u1711\u1720-\u1731\u1740-\u1751\u1760-\u176C\u176E-\u1770\u1780-\u17B3\u17D7\u17DC\u1820-\u1877\u1880-\u18A8\u18AA\u18B0-\u18F5\u1900-\u191E\u1950-\u196D\u1970-\u1974\u1980-\u19AB\u19C1-\u19C7\u1A00-\u1A16\u1A20-\u1A54\u1AA7\u1B05-\u1B33\u1B45-\u1B4B\u1B83-\u1BA0\u1BAE\u1BAF\u1BBA-\u1BE5\u1C00-\u1C23\u1C4D-\u1C4F\u1C5A-\u1C7D\u1CE9-\u1CEC\u1CEE-\u1CF1\u1CF5\u1CF6\u1D00-\u1DBF\u1E00-\u1F15\u1F18-\u1F1D\u1F20-\u1F45\u1F48-\u1F4D\u1F50-\u1F57\u1F59\u1F5B\u1F5D\u1F5F-\u1F7D\u1F80-\u1FB4\u1FB6-\u1FBC\u1FBE\u1FC2-\u1FC4\u1FC6-\u1FCC\u1FD0-\u1FD3\u1FD6-\u1FDB\u1FE0-\u1FEC\u1FF2-\u1FF4\u1FF6-\u1FFC\u2071\u207F\u2090-\u209C\u2102\u2107\u210A-\u2113\u2115\u2118-\u211D\u2124\u2126\u2128\u212A-\u2139\u213C-\u213F\u2145-\u2149\u214E\u2160-\u2188\u2C00-\u2C2E\u2C30-\u2C5E\u2C60-\u2CE4\u2CEB-\u2CEE\u2CF2\u2CF3\u2D00-\u2D25\u2D27\u2D2D\u2D30-\u2D67\u2D6F\u2D80-\u2D96\u2DA0-\u2DA6\u2DA8-\u2DAE\u2DB0-\u2DB6\u2DB8-\u2DBE\u2DC0-\u2DC6\u2DC8-\u2DCE\u2DD0-\u2DD6\u2DD8-\u2DDE\u3005-\u3007\u3021-\u3029\u3031-\u3035\u3038-\u303C\u3041-\u3096\u309B-\u309F\u30A1-\u30FA\u30FC-\u30FF\u3105-\u312D\u3131-\u318E\u31A0-\u31BA\u31F0-\u31FF\u3400-\u4DB5\u4E00-\u9FCC\uA000-\uA48C\uA4D0-\uA4FD\uA500-\uA60C\uA610-\uA61F\uA62A\uA62B\uA640-\uA66E\uA67F-\uA69D\uA6A0-\uA6EF\uA717-\uA71F\uA722-\uA788\uA78B-\uA78E\uA790-\uA7AD\uA7B0\uA7B1\uA7F7-\uA801\uA803-\uA805\uA807-\uA80A\uA80C-\uA822\uA840-\uA873\uA882-\uA8B3\uA8F2-\uA8F7\uA8FB\uA90A-\uA925\uA930-\uA946\uA960-\uA97C\uA984-\uA9B2\uA9CF\uA9E0-\uA9E4\uA9E6-\uA9EF\uA9FA-\uA9FE\uAA00-\uAA28\uAA40-\uAA42\uAA44-\uAA4B\uAA60-\uAA76\uAA7A\uAA7E-\uAAAF\uAAB1\uAAB5\uAAB6\uAAB9-\uAABD\uAAC0\uAAC2\uAADB-\uAADD\uAAE0-\uAAEA\uAAF2-\uAAF4\uAB01-\uAB06\uAB09-\uAB0E\uAB11-\uAB16\uAB20-\uAB26\uAB28-\uAB2E\uAB30-\uAB5A\uAB5C-\uAB5F\uAB64\uAB65\uABC0-\uABE2\uAC00-\uD7A3\uD7B0-\uD7C6\uD7CB-\uD7FB\uF900-\uFA6D\uFA70-\uFAD9\uFB00-\uFB06\uFB13-\uFB17\uFB1D\uFB1F-\uFB28\uFB2A-\uFB36\uFB38-\uFB3C\uFB3E\uFB40\uFB41\uFB43\uFB44\uFB46-\uFBB1\uFBD3-\uFD3D\uFD50-\uFD8F\uFD92-\uFDC7\uFDF0-\uFDFB\uFE70-\uFE74\uFE76-\uFEFC\uFF21-\uFF3A\uFF41-\uFF5A\uFF66-\uFFBE\uFFC2-\uFFC7\uFFCA-\uFFCF\uFFD2-\uFFD7\uFFDA-\uFFDC]|\uD800[\uDC00-\uDC0B\uDC0D-\uDC26\uDC28-\uDC3A\uDC3C\uDC3D\uDC3F-\uDC4D\uDC50-\uDC5D\uDC80-\uDCFA\uDD40-\uDD74\uDE80-\uDE9C\uDEA0-\uDED0\uDF00-\uDF1F\uDF30-\uDF4A\uDF50-\uDF75\uDF80-\uDF9D\uDFA0-\uDFC3\uDFC8-\uDFCF\uDFD1-\uDFD5]|\uD801[\uDC00-\uDC9D\uDD00-\uDD27\uDD30-\uDD63\uDE00-\uDF36\uDF40-\uDF55\uDF60-\uDF67]|\uD802[\uDC00-\uDC05\uDC08\uDC0A-\uDC35\uDC37\uDC38\uDC3C\uDC3F-\uDC55\uDC60-\uDC76\uDC80-\uDC9E\uDD00-\uDD15\uDD20-\uDD39\uDD80-\uDDB7\uDDBE\uDDBF\uDE00\uDE10-\uDE13\uDE15-\uDE17\uDE19-\uDE33\uDE60-\uDE7C\uDE80-\uDE9C\uDEC0-\uDEC7\uDEC9-\uDEE4\uDF00-\uDF35\uDF40-\uDF55\uDF60-\uDF72\uDF80-\uDF91]|\uD803[\uDC00-\uDC48]|\uD804[\uDC03-\uDC37\uDC83-\uDCAF\uDCD0-\uDCE8\uDD03-\uDD26\uDD50-\uDD72\uDD76\uDD83-\uDDB2\uDDC1-\uDDC4\uDDDA\uDE00-\uDE11\uDE13-\uDE2B\uDEB0-\uDEDE\uDF05-\uDF0C\uDF0F\uDF10\uDF13-\uDF28\uDF2A-\uDF30\uDF32\uDF33\uDF35-\uDF39\uDF3D\uDF5D-\uDF61]|\uD805[\uDC80-\uDCAF\uDCC4\uDCC5\uDCC7\uDD80-\uDDAE\uDE00-\uDE2F\uDE44\uDE80-\uDEAA]|\uD806[\uDCA0-\uDCDF\uDCFF\uDEC0-\uDEF8]|\uD808[\uDC00-\uDF98]|\uD809[\uDC00-\uDC6E]|[\uD80C\uD840-\uD868\uD86A-\uD86C][\uDC00-\uDFFF]|\uD80D[\uDC00-\uDC2E]|\uD81A[\uDC00-\uDE38\uDE40-\uDE5E\uDED0-\uDEED\uDF00-\uDF2F\uDF40-\uDF43\uDF63-\uDF77\uDF7D-\uDF8F]|\uD81B[\uDF00-\uDF44\uDF50\uDF93-\uDF9F]|\uD82C[\uDC00\uDC01]|\uD82F[\uDC00-\uDC6A\uDC70-\uDC7C\uDC80-\uDC88\uDC90-\uDC99]|\uD835[\uDC00-\uDC54\uDC56-\uDC9C\uDC9E\uDC9F\uDCA2\uDCA5\uDCA6\uDCA9-\uDCAC\uDCAE-\uDCB9\uDCBB\uDCBD-\uDCC3\uDCC5-\uDD05\uDD07-\uDD0A\uDD0D-\uDD14\uDD16-\uDD1C\uDD1E-\uDD39\uDD3B-\uDD3E\uDD40-\uDD44\uDD46\uDD4A-\uDD50\uDD52-\uDEA5\uDEA8-\uDEC0\uDEC2-\uDEDA\uDEDC-\uDEFA\uDEFC-\uDF14\uDF16-\uDF34\uDF36-\uDF4E\uDF50-\uDF6E\uDF70-\uDF88\uDF8A-\uDFA8\uDFAA-\uDFC2\uDFC4-\uDFCB]|\uD83A[\uDC00-\uDCC4]|\uD83B[\uDE00-\uDE03\uDE05-\uDE1F\uDE21\uDE22\uDE24\uDE27\uDE29-\uDE32\uDE34-\uDE37\uDE39\uDE3B\uDE42\uDE47\uDE49\uDE4B\uDE4D-\uDE4F\uDE51\uDE52\uDE54\uDE57\uDE59\uDE5B\uDE5D\uDE5F\uDE61\uDE62\uDE64\uDE67-\uDE6A\uDE6C-\uDE72\uDE74-\uDE77\uDE79-\uDE7C\uDE7E\uDE80-\uDE89\uDE8B-\uDE9B\uDEA1-\uDEA3\uDEA5-\uDEA9\uDEAB-\uDEBB]|\uD869[\uDC00-\uDED6\uDF00-\uDFFF]|\uD86D[\uDC00-\uDF34\uDF40-\uDFFF]|\uD86E[\uDC00-\uDC1D]|\uD87E[\uDC00-\uDE1D])(?:[\$0-9A-Z_a-z\xAA\xB5\xB7\xBA\xC0-\xD6\xD8-\xF6\xF8-\u02C1\u02C6-\u02D1\u02E0-\u02E4\u02EC\u02EE\u0300-\u0374\u0376\u0377\u037A-\u037D\u037F\u0386-\u038A\u038C\u038E-\u03A1\u03A3-\u03F5\u03F7-\u0481\u0483-\u0487\u048A-\u052F\u0531-\u0556\u0559\u0561-\u0587\u0591-\u05BD\u05BF\u05C1\u05C2\u05C4\u05C5\u05C7\u05D0-\u05EA\u05F0-\u05F2\u0610-\u061A\u0620-\u0669\u066E-\u06D3\u06D5-\u06DC\u06DF-\u06E8\u06EA-\u06FC\u06FF\u0710-\u074A\u074D-\u07B1\u07C0-\u07F5\u07FA\u0800-\u082D\u0840-\u085B\u08A0-\u08B2\u08E4-\u0963\u0966-\u096F\u0971-\u0983\u0985-\u098C\u098F\u0990\u0993-\u09A8\u09AA-\u09B0\u09B2\u09B6-\u09B9\u09BC-\u09C4\u09C7\u09C8\u09CB-\u09CE\u09D7\u09DC\u09DD\u09DF-\u09E3\u09E6-\u09F1\u0A01-\u0A03\u0A05-\u0A0A\u0A0F\u0A10\u0A13-\u0A28\u0A2A-\u0A30\u0A32\u0A33\u0A35\u0A36\u0A38\u0A39\u0A3C\u0A3E-\u0A42\u0A47\u0A48\u0A4B-\u0A4D\u0A51\u0A59-\u0A5C\u0A5E\u0A66-\u0A75\u0A81-\u0A83\u0A85-\u0A8D\u0A8F-\u0A91\u0A93-\u0AA8\u0AAA-\u0AB0\u0AB2\u0AB3\u0AB5-\u0AB9\u0ABC-\u0AC5\u0AC7-\u0AC9\u0ACB-\u0ACD\u0AD0\u0AE0-\u0AE3\u0AE6-\u0AEF\u0B01-\u0B03\u0B05-\u0B0C\u0B0F\u0B10\u0B13-\u0B28\u0B2A-\u0B30\u0B32\u0B33\u0B35-\u0B39\u0B3C-\u0B44\u0B47\u0B48\u0B4B-\u0B4D\u0B56\u0B57\u0B5C\u0B5D\u0B5F-\u0B63\u0B66-\u0B6F\u0B71\u0B82\u0B83\u0B85-\u0B8A\u0B8E-\u0B90\u0B92-\u0B95\u0B99\u0B9A\u0B9C\u0B9E\u0B9F\u0BA3\u0BA4\u0BA8-\u0BAA\u0BAE-\u0BB9\u0BBE-\u0BC2\u0BC6-\u0BC8\u0BCA-\u0BCD\u0BD0\u0BD7\u0BE6-\u0BEF\u0C00-\u0C03\u0C05-\u0C0C\u0C0E-\u0C10\u0C12-\u0C28\u0C2A-\u0C39\u0C3D-\u0C44\u0C46-\u0C48\u0C4A-\u0C4D\u0C55\u0C56\u0C58\u0C59\u0C60-\u0C63\u0C66-\u0C6F\u0C81-\u0C83\u0C85-\u0C8C\u0C8E-\u0C90\u0C92-\u0CA8\u0CAA-\u0CB3\u0CB5-\u0CB9\u0CBC-\u0CC4\u0CC6-\u0CC8\u0CCA-\u0CCD\u0CD5\u0CD6\u0CDE\u0CE0-\u0CE3\u0CE6-\u0CEF\u0CF1\u0CF2\u0D01-\u0D03\u0D05-\u0D0C\u0D0E-\u0D10\u0D12-\u0D3A\u0D3D-\u0D44\u0D46-\u0D48\u0D4A-\u0D4E\u0D57\u0D60-\u0D63\u0D66-\u0D6F\u0D7A-\u0D7F\u0D82\u0D83\u0D85-\u0D96\u0D9A-\u0DB1\u0DB3-\u0DBB\u0DBD\u0DC0-\u0DC6\u0DCA\u0DCF-\u0DD4\u0DD6\u0DD8-\u0DDF\u0DE6-\u0DEF\u0DF2\u0DF3\u0E01-\u0E3A\u0E40-\u0E4E\u0E50-\u0E59\u0E81\u0E82\u0E84\u0E87\u0E88\u0E8A\u0E8D\u0E94-\u0E97\u0E99-\u0E9F\u0EA1-\u0EA3\u0EA5\u0EA7\u0EAA\u0EAB\u0EAD-\u0EB9\u0EBB-\u0EBD\u0EC0-\u0EC4\u0EC6\u0EC8-\u0ECD\u0ED0-\u0ED9\u0EDC-\u0EDF\u0F00\u0F18\u0F19\u0F20-\u0F29\u0F35\u0F37\u0F39\u0F3E-\u0F47\u0F49-\u0F6C\u0F71-\u0F84\u0F86-\u0F97\u0F99-\u0FBC\u0FC6\u1000-\u1049\u1050-\u109D\u10A0-\u10C5\u10C7\u10CD\u10D0-\u10FA\u10FC-\u1248\u124A-\u124D\u1250-\u1256\u1258\u125A-\u125D\u1260-\u1288\u128A-\u128D\u1290-\u12B0\u12B2-\u12B5\u12B8-\u12BE\u12C0\u12C2-\u12C5\u12C8-\u12D6\u12D8-\u1310\u1312-\u1315\u1318-\u135A\u135D-\u135F\u1369-\u1371\u1380-\u138F\u13A0-\u13F4\u1401-\u166C\u166F-\u167F\u1681-\u169A\u16A0-\u16EA\u16EE-\u16F8\u1700-\u170C\u170E-\u1714\u1720-\u1734\u1740-\u1753\u1760-\u176C\u176E-\u1770\u1772\u1773\u1780-\u17D3\u17D7\u17DC\u17DD\u17E0-\u17E9\u180B-\u180D\u1810-\u1819\u1820-\u1877\u1880-\u18AA\u18B0-\u18F5\u1900-\u191E\u1920-\u192B\u1930-\u193B\u1946-\u196D\u1970-\u1974\u1980-\u19AB\u19B0-\u19C9\u19D0-\u19DA\u1A00-\u1A1B\u1A20-\u1A5E\u1A60-\u1A7C\u1A7F-\u1A89\u1A90-\u1A99\u1AA7\u1AB0-\u1ABD\u1B00-\u1B4B\u1B50-\u1B59\u1B6B-\u1B73\u1B80-\u1BF3\u1C00-\u1C37\u1C40-\u1C49\u1C4D-\u1C7D\u1CD0-\u1CD2\u1CD4-\u1CF6\u1CF8\u1CF9\u1D00-\u1DF5\u1DFC-\u1F15\u1F18-\u1F1D\u1F20-\u1F45\u1F48-\u1F4D\u1F50-\u1F57\u1F59\u1F5B\u1F5D\u1F5F-\u1F7D\u1F80-\u1FB4\u1FB6-\u1FBC\u1FBE\u1FC2-\u1FC4\u1FC6-\u1FCC\u1FD0-\u1FD3\u1FD6-\u1FDB\u1FE0-\u1FEC\u1FF2-\u1FF4\u1FF6-\u1FFC\u200C\u200D\u203F\u2040\u2054\u2071\u207F\u2090-\u209C\u20D0-\u20DC\u20E1\u20E5-\u20F0\u2102\u2107\u210A-\u2113\u2115\u2118-\u211D\u2124\u2126\u2128\u212A-\u2139\u213C-\u213F\u2145-\u2149\u214E\u2160-\u2188\u2C00-\u2C2E\u2C30-\u2C5E\u2C60-\u2CE4\u2CEB-\u2CF3\u2D00-\u2D25\u2D27\u2D2D\u2D30-\u2D67\u2D6F\u2D7F-\u2D96\u2DA0-\u2DA6\u2DA8-\u2DAE\u2DB0-\u2DB6\u2DB8-\u2DBE\u2DC0-\u2DC6\u2DC8-\u2DCE\u2DD0-\u2DD6\u2DD8-\u2DDE\u2DE0-\u2DFF\u3005-\u3007\u3021-\u302F\u3031-\u3035\u3038-\u303C\u3041-\u3096\u3099-\u309F\u30A1-\u30FA\u30FC-\u30FF\u3105-\u312D\u3131-\u318E\u31A0-\u31BA\u31F0-\u31FF\u3400-\u4DB5\u4E00-\u9FCC\uA000-\uA48C\uA4D0-\uA4FD\uA500-\uA60C\uA610-\uA62B\uA640-\uA66F\uA674-\uA67D\uA67F-\uA69D\uA69F-\uA6F1\uA717-\uA71F\uA722-\uA788\uA78B-\uA78E\uA790-\uA7AD\uA7B0\uA7B1\uA7F7-\uA827\uA840-\uA873\uA880-\uA8C4\uA8D0-\uA8D9\uA8E0-\uA8F7\uA8FB\uA900-\uA92D\uA930-\uA953\uA960-\uA97C\uA980-\uA9C0\uA9CF-\uA9D9\uA9E0-\uA9FE\uAA00-\uAA36\uAA40-\uAA4D\uAA50-\uAA59\uAA60-\uAA76\uAA7A-\uAAC2\uAADB-\uAADD\uAAE0-\uAAEF\uAAF2-\uAAF6\uAB01-\uAB06\uAB09-\uAB0E\uAB11-\uAB16\uAB20-\uAB26\uAB28-\uAB2E\uAB30-\uAB5A\uAB5C-\uAB5F\uAB64\uAB65\uABC0-\uABEA\uABEC\uABED\uABF0-\uABF9\uAC00-\uD7A3\uD7B0-\uD7C6\uD7CB-\uD7FB\uF900-\uFA6D\uFA70-\uFAD9\uFB00-\uFB06\uFB13-\uFB17\uFB1D-\uFB28\uFB2A-\uFB36\uFB38-\uFB3C\uFB3E\uFB40\uFB41\uFB43\uFB44\uFB46-\uFBB1\uFBD3-\uFD3D\uFD50-\uFD8F\uFD92-\uFDC7\uFDF0-\uFDFB\uFE00-\uFE0F\uFE20-\uFE2D\uFE33\uFE34\uFE4D-\uFE4F\uFE70-\uFE74\uFE76-\uFEFC\uFF10-\uFF19\uFF21-\uFF3A\uFF3F\uFF41-\uFF5A\uFF66-\uFFBE\uFFC2-\uFFC7\uFFCA-\uFFCF\uFFD2-\uFFD7\uFFDA-\uFFDC]|\uD800[\uDC00-\uDC0B\uDC0D-\uDC26\uDC28-\uDC3A\uDC3C\uDC3D\uDC3F-\uDC4D\uDC50-\uDC5D\uDC80-\uDCFA\uDD40-\uDD74\uDDFD\uDE80-\uDE9C\uDEA0-\uDED0\uDEE0\uDF00-\uDF1F\uDF30-\uDF4A\uDF50-\uDF7A\uDF80-\uDF9D\uDFA0-\uDFC3\uDFC8-\uDFCF\uDFD1-\uDFD5]|\uD801[\uDC00-\uDC9D\uDCA0-\uDCA9\uDD00-\uDD27\uDD30-\uDD63\uDE00-\uDF36\uDF40-\uDF55\uDF60-\uDF67]|\uD802[\uDC00-\uDC05\uDC08\uDC0A-\uDC35\uDC37\uDC38\uDC3C\uDC3F-\uDC55\uDC60-\uDC76\uDC80-\uDC9E\uDD00-\uDD15\uDD20-\uDD39\uDD80-\uDDB7\uDDBE\uDDBF\uDE00-\uDE03\uDE05\uDE06\uDE0C-\uDE13\uDE15-\uDE17\uDE19-\uDE33\uDE38-\uDE3A\uDE3F\uDE60-\uDE7C\uDE80-\uDE9C\uDEC0-\uDEC7\uDEC9-\uDEE6\uDF00-\uDF35\uDF40-\uDF55\uDF60-\uDF72\uDF80-\uDF91]|\uD803[\uDC00-\uDC48]|\uD804[\uDC00-\uDC46\uDC66-\uDC6F\uDC7F-\uDCBA\uDCD0-\uDCE8\uDCF0-\uDCF9\uDD00-\uDD34\uDD36-\uDD3F\uDD50-\uDD73\uDD76\uDD80-\uDDC4\uDDD0-\uDDDA\uDE00-\uDE11\uDE13-\uDE37\uDEB0-\uDEEA\uDEF0-\uDEF9\uDF01-\uDF03\uDF05-\uDF0C\uDF0F\uDF10\uDF13-\uDF28\uDF2A-\uDF30\uDF32\uDF33\uDF35-\uDF39\uDF3C-\uDF44\uDF47\uDF48\uDF4B-\uDF4D\uDF57\uDF5D-\uDF63\uDF66-\uDF6C\uDF70-\uDF74]|\uD805[\uDC80-\uDCC5\uDCC7\uDCD0-\uDCD9\uDD80-\uDDB5\uDDB8-\uDDC0\uDE00-\uDE40\uDE44\uDE50-\uDE59\uDE80-\uDEB7\uDEC0-\uDEC9]|\uD806[\uDCA0-\uDCE9\uDCFF\uDEC0-\uDEF8]|\uD808[\uDC00-\uDF98]|\uD809[\uDC00-\uDC6E]|[\uD80C\uD840-\uD868\uD86A-\uD86C][\uDC00-\uDFFF]|\uD80D[\uDC00-\uDC2E]|\uD81A[\uDC00-\uDE38\uDE40-\uDE5E\uDE60-\uDE69\uDED0-\uDEED\uDEF0-\uDEF4\uDF00-\uDF36\uDF40-\uDF43\uDF50-\uDF59\uDF63-\uDF77\uDF7D-\uDF8F]|\uD81B[\uDF00-\uDF44\uDF50-\uDF7E\uDF8F-\uDF9F]|\uD82C[\uDC00\uDC01]|\uD82F[\uDC00-\uDC6A\uDC70-\uDC7C\uDC80-\uDC88\uDC90-\uDC99\uDC9D\uDC9E]|\uD834[\uDD65-\uDD69\uDD6D-\uDD72\uDD7B-\uDD82\uDD85-\uDD8B\uDDAA-\uDDAD\uDE42-\uDE44]|\uD835[\uDC00-\uDC54\uDC56-\uDC9C\uDC9E\uDC9F\uDCA2\uDCA5\uDCA6\uDCA9-\uDCAC\uDCAE-\uDCB9\uDCBB\uDCBD-\uDCC3\uDCC5-\uDD05\uDD07-\uDD0A\uDD0D-\uDD14\uDD16-\uDD1C\uDD1E-\uDD39\uDD3B-\uDD3E\uDD40-\uDD44\uDD46\uDD4A-\uDD50\uDD52-\uDEA5\uDEA8-\uDEC0\uDEC2-\uDEDA\uDEDC-\uDEFA\uDEFC-\uDF14\uDF16-\uDF34\uDF36-\uDF4E\uDF50-\uDF6E\uDF70-\uDF88\uDF8A-\uDFA8\uDFAA-\uDFC2\uDFC4-\uDFCB\uDFCE-\uDFFF]|\uD83A[\uDC00-\uDCC4\uDCD0-\uDCD6]|\uD83B[\uDE00-\uDE03\uDE05-\uDE1F\uDE21\uDE22\uDE24\uDE27\uDE29-\uDE32\uDE34-\uDE37\uDE39\uDE3B\uDE42\uDE47\uDE49\uDE4B\uDE4D-\uDE4F\uDE51\uDE52\uDE54\uDE57\uDE59\uDE5B\uDE5D\uDE5F\uDE61\uDE62\uDE64\uDE67-\uDE6A\uDE6C-\uDE72\uDE74-\uDE77\uDE79-\uDE7C\uDE7E\uDE80-\uDE89\uDE8B-\uDE9B\uDEA1-\uDEA3\uDEA5-\uDEA9\uDEAB-\uDEBB]|\uD869[\uDC00-\uDED6\uDF00-\uDFFF]|\uD86D[\uDC00-\uDF34\uDF40-\uDFFF]|\uD86E[\uDC00-\uDC1D]|\uD87E[\uDC00-\uDE1D]|\uDB40[\uDD00-\uDDEF])*$/;

// from Node-RED runtime/lib/flows/util.js
const EnvVarPropertyRE_old = /^\$\((\S+)\)$/;
const EnvVarPropertyRE = /^\${(\S+)}$/;

// nodes that don't generate errors
const NoErrorNodes = [
	"catch",
	"change",
	"complete",
	"debug",
	"inject",
	"link in",
	"link out",
	"switch",
	"status",
	"trigger",
	"rpi-neopixels"
];

// nodes that don't call done
const NoDoneNodes = [
	"switch",	// does call node.error()
];

// nodes that don't report status
const NoStatusNodes = [
	"catch",
	"change",
	"complete",
	"inject",
	"link call",
	"link in",
	"link out",
	"range",
	"split",
	"status",
	"switch"
];

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		this.sourcePath = null;
		this.outputDirectory = null;
		let argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {				
			case "-o":
				argi++;	
				if (argi >= argc)
					throw new Error("-o: no path!");
				name = argv[argi];
				if (this.outputDirectory)
					throw new Error("-o '" + name + "': too many output paths!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': path not found!");
				this.outputDirectory = path;
				break;

			default:
				name = argv[argi];
				if (this.sourcePath)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.sourcePath = path;
				break;
			}
		}
		if (!this.sourcePath)
			throw new Error("no input file!");
	}

	run() {
		const source = this.readFileString(this.sourcePath);
		let flows = JSON.parse(source);

		let credentials;
		if (this.sourcePath.toLowerCase().endsWith(".json")) {
			const path = this.sourcePath.slice(0, -5) + "_cred_mcu.json";
			if (this.resolveFilePath(path))
				credentials = JSON.parse(this.readFileString(path)).credentials;
		}

		flows = this.transformFlows(flows, credentials);

		const parts = this.splitPath(this.sourcePath);
		parts.extension = ".js";
		if (this.outputDirectory)
			parts.directory = this.outputDirectory;
		const outputPath = this.joinPath(parts);

		const output = new FILE(outputPath, "wb");
		output.writeString(flows);
		output.close();
	}

	transformFlows(flows, credentials) {
		const imports = new Map([["nodered", ""]]);		// ensure that globalThis.RED is available

		// must be an array with at least one element
		if (!Array.isArray(flows))
			throw new Error("JSON input is not an array");

		if (!flows.length)
			throw new Error("no nodes");

		// if no flows, create one. useful for running flow snippets (e.g. https://cookbook.nodered.org/).
		if (!flows.some(config => ("tab" === config.type) && config.z)) {
			const z = flows[0].z;
			if (flows.every(config => z === config.z)) {
				flows.unshift({
					type: "tab",
					name: "auto-generated by nodered2mcu",
					id: z
				});
			}
		}

		// add global configuration flow to start of list
		flows.unshift({
			type: "tab",
			id: "__config",
			name: "global config"
		});

		// set z for each node in global configuration flow
		flows.forEach(config => {
			if (("tab" !== config.type) && !config.z)
				config.z = "__config"; 
		});

		// map from node ID to config and delete disabled and Comment nodes
		const nodes = new Map;
		for (let i = 0; i < flows.length; i++) {
			const config = flows[i];
			if ("tab" === config.type)
				continue;
			if (config.d || ("comment" === config.type)) {
				flows.splice(i, 1);
				i -= 1;
			}
			else
				nodes.set(config.id, config);
		}

		// route around junction nodes
		while (true) {
			let changed;

			nodes.forEach(config => {
				config.wires?.forEach(output => {
					for (let i = 0; i < output.length; i++) {
						const wire = output[i];
						const target = nodes.get(wire);
						if ("junction" !== target?.type)
							continue;
						output.splice(i, 1, ...target.wires[0]);
						i += -1 + target.wires[0].length;
						changed = true;
					}
				});
			});
			if (!changed)
				break;
		}

		// remove junction nodes
		for (let i = 0; i < flows.length; i++) {
			const config = flows[i];
			if ("junction" === config.type) {
				nodes.delete(config.id);
				flows.splice(i, 1);
				i -= 1;
			}
		}

		// remove group nodes with empty environments
		//  (unobservable except for NR_GROUP_NAME & NR_GROUP_ID environment variables)
		for (let i = 0; i < flows.length; i++) {
			const config = flows[i];
			if (("group" === config.type) && !config.env?.length) {
				const g = config.g;
				const id = config.id;
				nodes.delete(config.id);
				flows.splice(i, 1);
				i -= 1;

				for (let j = 0; j < flows.length; j++) {
					const config = flows[j];
					if (config.g === id) {		// this node referenced deleted group
						if (g)
							config.g = g;		// reference enclosing group
						else
							delete config.g;	// no longer in a group
					}
				}
			}
		}
		
		// check for ui_base (required for all other ui_* nodes)
		nodes.forEach(config => {
			if ("ui_base" === config.type)
				nodes.ui_base = config;
		});

		// replace ${Environment} variables
		flows.forEach((config, i) => this.applyEnv(flows, i, flows[i], flows));

		// process each node
		let parts = [];
		flows.forEach(config => {
			if (("tab" !== config.type) || config.disabled)
				return;

			parts.push(`\tflow = flows.next().value;	// ${config.id}`);
			parts.push(``);
			parts.push(`\tnodes = flow.nodes();`);

			const z = config.id;
			flows.forEach(config => {
				if (config.z !== z)
					return;

				if (credentials?.[config.id])
					config.credentials = credentials[config.id];

				const name = config.name;
				let {type, id, /* name, */ ...c} = {...config};
				delete c.x;
				delete c.y;
				delete c.z;
				delete c.outputLabels;
				delete c._mcu;
				delete c.moddable_manifest;

				try {
					// remove wires that connect to missing nodes
					for (let i = 0, length = config.wires?.length ?? 0; i < length; i++)
						config.wires[i] = config.wires[i].filter(wire => nodes.has(wire));

					// enabled status nodes for this node
					const statuses = flows.filter(config => {
						if ((config.z !== z) || (config.type !== "status"))
							return;

						return !config.scope || config.scope.includes(id);
					}).map(config => config.id);

					// enabled completion nodes for this node
					const dones = flows.filter(config => {
						if ((config.z !== z) || (config.type !== "complete"))
							return;

						return config.scope.includes(id);
					}).map(config => config.id);

					// enabled catch nodes for this node
					let errors = flows.filter(config => {
						if ((config.z !== z) || (config.type !== "catch"))
							return;

						return (!config.scope && !config.uncaught) || config.scope?.includes(id);
					});
					if (0 === errors.length)
						errors = flows.filter(config => (config.z === z) && config.uncaught && (config.type === "catch"));
					errors = errors.map(config => config.id);

					this.prepareNode(type, c, dones.length ? dones : undefined, errors.length ? errors : undefined, statuses.length ? statuses : undefined, nodes, imports);

					if (c.type) {
						config.type = c.type;
						delete c.type;
					}

					parts.push(`\tnode = nodes.next().value;	// ${config.type} - ${id}`);

					let configuration = ["{"];
					for (const name in c) {
						const value = c[name];
						if (("string" === typeof value) && value.startsWith("function ("))
							configuration.push(`\t\t${name}: ${value},`);
						else
						if (("string" === typeof value) && value.startsWith("[[JSON]]"))
							configuration.push(`\t\t${name}: ${value.slice(8)},`);
						else
						if (value instanceof Uint8Array)
							configuration.push(`\t\t${name}: Uint8Array.of(${value.toString()}),`);
						else
							configuration.push(`\t\t${name}: ${JSON.stringify(value)},`);
					}
					
					configuration.push("\t}");
					configuration = configuration.join("\n");
					
					parts.push(`\tnode.onStart(${configuration});`) ;
					parts.push(``)
				}
				catch (e) {
					trace(`Error translating node ID "${id}, type "${type}", name "${name}": ${e}\n`);
					throw e;
				}
			});
		});

		const nodeParts = parts.splice(0);

		// header (after imports complete)
		parts.push(`// auto-generated by nodered2mcu on ${Date()}`);
		parts.push(``);

		imports.forEach((specifier, name) => {
			if (specifier)
				parts.push(`import ${name} from "${specifier}"`);
			else
				parts.push(`import "${name}"`);
		}); 
		parts.push(``);

		parts.push(`function build(flows, createFlow, createNode) {`);
		parts.push(`\tlet flow, node, nodes;`);

		flows.forEach(config => {
			if (("tab" !== config.type) || config.disabled)
				return;

			const createFlow = `\tflow = createFlow("${config.id}", ${JSON.stringify(config.label ?? "")}`;
			if (!config.env?.length)
				parts.push(`${createFlow});`);
			else {
				parts.push(`${createFlow}, {`);
				parts.push(...this.prepareEnv(config.env));
				parts.push(`\t});`);
			}

			const z = config.id;
			flows.forEach(config => {
				if (config.z !== z)
					return;

				parts.push(`\tcreateNode("${config.type}", "${config.id}", ${JSON.stringify(config.name ?? "")}, flow);`) 
			});
		});

		parts.push(``) 

		parts.push(`\tflows = flows.values();`);

		parts = parts.concat(nodeParts);

		parts.push(`}`) 
		parts.push(``) 

		parts.push(`export default Object.freeze({`);
		parts.push(`\tbuild,`);
		parts.push(`}, true);`);
		
		return parts.join("\n");
	}
	prepareNode(type, config, dones, errors, statuses, nodes, imports) {
		switch (type) {
			case "status": {
				delete config.scope;
			} break;

			case "catch": {
				delete config.uncaught;
				delete config.scope;
			} break;

			case "complete": {
				delete config.uncaught;
				delete config.scope;
			} break;

			case "group": {
				const env = this.prepareEnv(config.env);
				config.env = `[[JSON]]{\n` + env.join("\n") + "\n}"
			
				delete config.nodes;
				delete config.style;
				delete config.w;
				delete config.h;
			} break;

			case "debug": {
				if ("jsonata" === config.targetType)
					throw new Error("jsonata unimplemented");

				const getter = [];
				getter.push(`function (msg) {`);
				if ("true" === config.complete)
					getter.push(`\t\t\treturn msg;`);
				else {
					let name = config.complete;
					if ("false" === name)
						name = "payload";
					getter.push(`\t\t\treturn msg${this.prepareProp(name)};`);
				}
				getter.push(`\t\t}`);
				config.getter = getter.join("\n");

				if (config.tostatus) {
					const statusType = config.statusType ?? "auto";
					if ("auto" === statusType)
						config.statusVal = config.getter;
					else if ("msg" === statusType) {
						getter.length = 1;
						getter.push(`\t\t\treturn msg.${config.statusVal};`);
						getter.push(`\t\t}`);
						config.statusVal = getter.join("\n");
					}
					else
						throw new Error(`unimplemented statusType: ${config.statusType}`); 
				}

				config.active = !!config.active;
			} break;

			case "function": {
				const params = "node, context, flow, global, libs, env"

				let libs = "";
				if (config.libs?.length) {
					libs = [];
					for (let i = 0; i < config.libs.length; i++) {
						const item = config.libs[i];
						libs[i] = item.var;
						config.libs[i] = item.module;
					}
					libs = `const [${libs.join(", ")}] = libs;\n`
				}
				else
					delete config.libs;

				if (config.func) {
					config.func = `function (msg, ${params}) {\n${libs}${config.func}\n}`;

					if (dones && !/node\.done\s*\(\s*\)/.test(config.func))		// from 10-function.js.. first order approximiation of what it does
						config.doDone = true;
				}
				else 
					delete config.func;

				if (config.initialize)
					config.initialize = `function (${params}) {\n${libs}${config.initialize}\n}`;
				else
					delete config.initialize;

				delete config.finalize;
				delete config.outputs;
				delete config.noerr;
				} break;

			case "inject": {
				if (config.chrontab)
					throw new Error("chrontab unimplemented");

				const trigger = [];
				trigger.push(`function () {`);
				trigger.push(`\t\t\tconst msg = {};`);

				if (!config.props) {		// convert older style config
					config.props = [{
						p: "payload",
						payload: config.payload,
						payloadType: config.payloadType
					}];
					if (undefined !== config.topic) {
						config.props.push({
							p: "topic",
							v: config.topic,
							vt: "str"
						});
					}
				}
				config.props.forEach(property => {
					const name = property.p;
					const type = ("payload" === name) ? config.payloadType : property.vt;
					let value = property.v;
					if ("payload" === name)
						value = config.payload;
					else if ("topic" === name)
						value = config.topic;
					value = this.resolveValue(type, value);
					this.createPropPath(name, trigger, "\t\t\t");
					trigger.push(`\t\t\tmsg${this.prepareProp(name)} = ${value};`);
				});

				trigger.push(`\t\t\tthis.send(msg);`);
				if (dones)
					trigger.push(`\t\t\tthis.done(msg);`);
				trigger.push(`\t\t}`);
				config.trigger = trigger.join("\n");

				let delay;
				if (config.once)
					delay = config.once ? parseFloat(config.onceDelay) * 1000 : 0;
				const repeat = config.repeat ? parseFloat(config.repeat) * 1000 : 0;
				if ((undefined !== delay) || (0 !== repeat)) {
					const initialize = [];
					initialize.push(`function () {`);

					imports.set("Timer", "timer");
					if (repeat)
						initialize.push(`\t\t\tTimer.set(() => this.trigger(), ${delay ?? repeat}, ${repeat});`);
					else
						initialize.push(`\t\t\tTimer.set(() => this.trigger(), ${delay});`);

					initialize.push(`\t\t}`);
					config.initialize = initialize.join("\n");
				}

				delete config.props;
				delete config.topic;
				delete config.repeat;
				delete config.crontab;
				delete config.once;
				delete config.onceDelay;
				delete config.payload;
				delete config.payloadType;
				} break;

			case "change": {
				const change = [];
				if (dones)
					change.push(`function (msg, done) {`);
				else
					change.push(`function (msg) {`);

				let createdTemp = false;
				config.rules.forEach(rule => {
					let doDelete;

					if (("delete" === rule.t) || ("move" === rule.t)) {
						if ("msg" === rule.pt) {
							// moving a property to itself is no-op
							if (("move" === rule.t) && (rule.p === rule.to) && ("msg" === rule.tot))
								return;

							doDelete = `\t\t\tdelete msg${this.prepareProp(rule.p)}`;
						}
						else if ("flow" === rule.pt)
							doDelete = `\t\t\tthis.flow.delete(${this.makeStorageArgs(rule.p)});`;
						else if ("global" === rule.pt)
							doDelete = `\t\t\tglobalContext.delete(${this.makeStorageArgs(rule.p)});`;
						else
							throw new Error(`unexpected delete type: ${rule.pt}`);
					}

					if ("set" === rule.t) {
						let value = this.resolveValue(rule.tot, rule.to);
						if (rule.dc) {
							value = `structuredClone(${value})`
							imports.set("structuredClone", "structuredClone");
						}
						if ("msg" === rule.pt) {
							this.createPropPath(rule.p, change, "\t\t\t");
							change.push(`\t\t\tmsg${this.prepareProp(rule.p)} = ${value};`);
						}
						else if ("flow" === rule.pt)
							change.push(`\t\t\tthis.flow.set(${this.makeStorageArgs(rule.p, value)});`);
						else if ("global" === rule.pt)
							change.push(`\t\t\tglobalContext.set(${this.makeStorageArgs(rule.p, value)});`);
						else
							throw new Error(`unexpected set type: ${rule.pt}`);
					}
					else if ("change" === rule.t) {
						const from = this.resolveValue(rule.fromt, rule.from);
						const to = this.resolveValue(rule.tot, rule.to);
						if ("msg" === rule.pt) {
							this.createPropPath(rule.p, change, "\t\t\t");
							change.push(`\t\t\tmsg${this.prepareProp(rule.p)} = this.change(msg${this.prepareProp(rule.p)}, ${from}, ${JSON.stringify(rule.fromt)}, ${to});`);
						}
						else if (("flow" === rule.pt) || ("global" === rule.pt)) {
							const which = ("flow" === rule.pt) ? "flow" : "globalContext";
							const value = `this.change(this.${which}.get(${this.makeStorageArgs(rule.p)}), ${from}, ${JSON.stringify(rule.fromt)}, ${to})`;
							change.push(`\t\t\tthis.${which}.set(${this.makeStorageArgs(rule.p, value)});`);
						}
						else
							throw new Error(`unexpected change type: ${rule.pt}`);
					}
					else if ("move" === rule.t) {
						// GET, DELETE, SET
						const value = this.resolveValue(rule.pt, rule.p);
						if (createdTemp)
							change.push(`\t\t\ttemp = ${value};`);
						else {
							change.push(`\t\t\tlet temp = ${value};`);
							createdTemp = true;
						}
						change.push(doDelete);

						if ("msg" === rule.tot) {
							this.createPropPath(rule.to, change, "\t\t\t");
							change.push(`\t\t\tmsg${this.prepareProp(rule.to)} = temp;`);
						}
						else if ("flow" === rule.tot)
							change.push(`\t\t\tthis.flow.set(${this.makeStorageArgs(rule.to)}, temp);`);
						else if ("global" === rule.tot)
							change.push(`\t\t\tglobalContext.set(${this.makeStorageArgs(rule.to)}, temp);`);
						else
							throw new Error(`unexpected move type: ${rule.pt}`);
					}

					if ("delete" === rule.t)
						change.push(doDelete);
				});

				if (dones)
					change.push(`\t\t\tdone();`);
				change.push(`\t\t\treturn msg;`);
				change.push(`\t\t}`);
				config.onMessage = change.join("\n");

				delete config.rules;
				delete config.action;
				delete config.property;
				delete config.from;
				delete config.to;
				delete config.reg;
			} break;

			case "split": {
				if (("splt" in config) && !config.spltType)		// historic: https://cookbook.nodered.org/basic/split-text
					config.spltType = "str";
				if (!config.arraySpltType) {		// 		// historic: https://cookbook.nodered.org/basic/split-text
					config.arraySplt = 1;
					config.arraySpltType = "len";
				}

				config.arraySplt = parseInt(config.arraySplt);

                config.splt = (config.splt || "\\n").replace(/\\n/g,"\n").replace(/\\r/g,"\r").replace(/\\t/g,"\t").replace(/\\e/g,"\e").replace(/\\f/g,"\f").replace(/\\0/g,"\0");	// adapted from 17-split.js
				switch (config.spltType) {
					case "bin":
						if (config.splt.startsWith("["))
							config.splt = Uint8Array.from(JSON.parse(config.splt));
						else
							config.splt = new Uint8Array(ArrayBuffer.fromString(config.splt));
						break;
					case "str":
						break;
					case "len":
						config.splt = parseInt(config.splt);
						break;
					default:
						throw new Error("unrecognized spltType: " + config.spltType);
				}
			} break;
			
			case "join": {
				if ("reduce" === config.mode)
					throw new Error("reduce unimplemented");

				config.mode = config.mode||"auto";
				config.property = config.property||"payload";
				config.propertyType = config.propertyType||"msg";
				if (config.propertyType === 'full') {
					config.property = "payload";
				}
				config.key = config.key||"topic";
				config.count = Number(config.count || 0);
				config.joinerType = config.joinerType || "str";

				if (config.joinerType === "str")
					config.joiner = (config.joiner ?? "").replace(/\\n/g,"\n").replace(/\\r/g,"\r").replace(/\\t/g,"\t").replace(/\\e/g,"\e").replace(/\\f/g,"\f").replace(/\\0/g,"\0");	// adapted from 17-split.js

				config.getter = `function (msg) {return msg${this.prepareProp(config.property)};}`;
				config.setter = `function (msg, value) {msg${this.prepareProp(config.property)} = value;}`;
			} break;
			
			case "trigger": {
				config.op1 = config.op1 || "1";
				config.op2 = config.op2 || "0";
				config.op1type = config.op1type || "str";
				config.op2type = config.op2type || "str";

				if ((config.op1type === "num") && (!isNaN(config.op1))) { config.op1 = Number(config.op1); }
				if ((config.op2type === "num") && (!isNaN(config.op2))) { config.op2 = Number(config.op2); }

				if (config.op1type === 'val') {
					if (config.op1 === 'true' || config.op1 === 'false') {
						config.op1type = 'bool'
					} else if (config.op1 === 'null') {
						config.op1type = 'null';
						config.op1 = null;
					} else {
						config.op1type = 'str';
					}
				}
				if (config.op2type === 'val') {
					if (config.op2 === 'true' || config.op2 === 'false') {
						config.op2type = 'bool'
					} else if (config.op2 === 'null') {
						config.op2type = 'null';
						config.op2 = null;
					} else {
						config.op2type = 'str';
					}
				}

				config.op1Templated = (config.op1type === 'str' && config.op1.indexOf("{{") != -1);
				config.op2Templated = (config.op2type === 'str' && config.op2.indexOf("{{") != -1);

				if (config.op1type && ("pay" !== config.op1type) && ("payl" !== config.op1type) && (undefined !== config.op1))
					config.__op1 = `function () {return ${this.resolveValue(config.op1type, config.op1)}}`;
				if (config.op2type && ("pay" !== config.op2type) && ("payl" !== config.op2type) && (undefined !== config.op2))
					config.__op2 = `function () {return ${this.resolveValue(config.op2type, config.op2)}}`;
			} break;

			case "sort": {
				if (config.as_num)
					config.as_num = true;
				else
					delete config.as_num;

				let target_prop = config.target || "payload";
				let target_is_prop = (config.targetType === 'msg');
				let key_is_exp = target_is_prop ? (config.msgKeyType === "jsonata") : (config.seqKeyType === "jsonata");
				let key_prop = config.seqKey || "payload";
				let key_exp = target_is_prop ? config.msgKey : config.seqKey;
				config.dir = (config.order === "descending") ? -1 : +1;
				config.target_is_prop = target_is_prop;
				config.key_is_exp = key_is_exp;
				if (key_is_exp)  {
					config.getter = `function (msg) {return msg${this.prepareProp(key_exp)};}`;
					config.setter = `function (msg, value) {msg${this.prepareProp(key_exp)} = value;}`;
				}
				else {
					const prop = target_is_prop ? target_prop : key_prop;
					config.getter = `function (msg) {return msg${this.prepareProp(prop)};}`;
					config.setter = `function (msg, value) {msg${this.prepareProp(prop)} = value;}`;
				}
				
				delete config.order;
			} break;

			case "range": {
				const maxin = parseFloat(config.maxin);
				const maxout = parseFloat(config.maxout);
				const minin = parseFloat(config.minin);
				const minout = parseFloat(config.minout);
				const scale = (maxout - minout) / (maxin - minin);
				const action = config.action;
				const property = config.property ?? "payload";
				const round = config.round;

				const range = [];
				if (dones)
					range.push(`function (msg, done) {`);
				else
					range.push(`function (msg) {`);
				range.push(`\t\t\tlet value = msg${this.prepareProp(property)};`);
				if ("clamp" === action) {
					range.push(`\t\t\tif (value < ${minin})`);
					range.push(`\t\t\t\tvalue = ${minin};`);
					range.push(`\t\t\tif (value > ${maxin})`);
					range.push(`\t\t\t\tvalue = ${maxin};`);
				}
				else if ("roll" === action) {
					const divisor = maxin - minin;
					range.push(`\t\t\tvalue = ((value - ${minin}) % ${divisor} + ${divisor}) % ${divisor} + ${minin};`);
				}
				range.push(`\t\t\tvalue = ((value - ${minin}) * ${scale}) + ${minout};`);
				if (round)
					range.push(`\t\t\tmsg${this.prepareProp(property)} = Math.round(value);`);
				else				
					range.push(`\t\t\tmsg${this.prepareProp(property)} = value;`);
				if (dones)
					range.push(`\t\t\tdone();`);
				range.push(`\t\t\treturn msg;`);
				range.push(`\t\t}`);
				
				config.onMessage = range.join("\n");

				delete config.maxin;
				delete config.maxout;
				delete config.minin;
				delete config.minout;
				delete config.action;
				delete config.property;
				delete config.round;
			} break;

			case "switch": {
				const value = this.resolveValue(config.propertyType, config.property);
				const checkAll = "true" === config.checkall;
				const outputCount = config.wires?.length ?? 0;
				const hasElse = checkAll && config.rules.find(config => "else" === config.t);
				const usesPrevious = config.rules.find(config => ("prev" === config.v2t) || ("prev" == config.vt));

				const doSwitch = [];
				doSwitch.push(`function (msg) {`);
				doSwitch.push(`\t\t\tlet value = ${value};`);
				doSwitch.push(`\t\t\tconst result = [${"null, ".repeat(outputCount)}];`);
				if (usesPrevious) {
					doSwitch.push(`\t\t\tconst previous = this.previous;`);
					doSwitch.push(`\t\t\tthis.previous = value;`);
				}
				if (hasElse)
					doSwitch.push(`\t\t\tlet first = true;`);

				config.rules.map((config, index) => {
					let test;

					if ("istype" === config.t) {
						switch (config.v) {
							case "string":
							case "number":
							case "boolean":
							case "object":
							case "undefined":
							case "null":
								test = `"${config.v}" === typeof value`;
								break;
							case "array":
								test = `Array.isArray(value)`;
								break;
							case "json":
								doSwitch.push(`\t\t\ttry {`);
								doSwitch.push(`\t\t\t\tJSON.parse(value);`);
								doSwitch.push(`\t\t\t\tvalue = true;`);
								doSwitch.push(`\t\t\t}`);
								doSwitch.push(`\t\t\tcatch {`);
								doSwitch.push(`\t\t\t\tvalue = false;`);
								doSwitch.push(`\t\t\t}`);
								test = `value`;
								break;
							case "buffer":
								test = `value instanceof Uint8Array`;
								break;
							default:
								throw new Error(`unimplemented istype: ${config.v}`);
						}
					}
					else {
						const v = this.resolveSwitch(config.vt, config.v);
						const v2 = this.resolveSwitch(config.v2t, config.v2);

						switch (config.t) {
							case "btwn":
								test = `(${v} <= value) && (value <= ${v2})`;
								break;
							case "eq":
								test = `value == ${v}`;
								break;
							case "neq":
								test = `value != ${v}`;
								break;
							case "lt":
								test = `value < ${v}`;
								break;
							case "lte":
								test = `value <= ${v}`;
								break;
							case "gt":
								test = `value > ${v}`;
								break;
							case "gte":
								test = `value >= ${v}`;
								break;
							case "true":
								test = `true === value`;
								break;
							case "false":
								test = `false === value`;
								break;
							case "null":
								test = `((null === value) || (undefined === value))`;
								break;
							case "nnull":
								test = `!((null === value) || (undefined === value))`;
								break;							
							case "empty":
								test = `this.empty(value)`;
								break;							
							case "nempty":
								test = `!this.empty(value)`;
								break;							
							case "hask":
								test = `("object" === typeof value) && (${v} in value)`;
								break;							
							case "else":
								if (hasElse) {
									doSwitch.push(`\t\t\tif (first) {`);
									doSwitch.push(`\t\t\t\tresult[${index}] = msg;`);
								}
								else {
									doSwitch.push(`\t\t\t{`);
									doSwitch.push(`\t\t\tresult[${index}] = msg;`);	
								}
								break;
							default:
								throw new Error(`unimplemented rule: ${config.t}`);
						}
					}

					if (test) {
						doSwitch.push(`\t\t\tif (${test}) {`);
						doSwitch.push(`\t\t\t\tresult[${index}] = msg;`);
						if (!checkAll)
							doSwitch.push(`\t\t\t\treturn result;`);		//@@
						if (hasElse)
							doSwitch.push(`\t\t\t\tfirst = false;`);
					}
					doSwitch.push(`\t\t\t}`);
				});
				doSwitch.push(`\t\t\treturn result;`);
				doSwitch.push(`\t\t}`);
				config[config.repair ? "compare" : "onMessage"] = doSwitch.join("\n");

				delete config.property;
				delete config.propertyType;
				delete config.rules;
				delete config.checkall;
				delete config.repair;
				delete config.outputs;
			} break;

			case "link call":
			case "link out":
				config.links = config.links?.filter(link => nodes.has(link));	// remove broken links
				if ("link call" === type) {
					config.timeout = parseFloat(config.timeout || 30) * 1000;		// logic from 60_link.js
					if (isNaN(config.timeout))
						config.timeout = 30_000;
				}
				else if ("return" === config.mode)
					delete config.links;
				break;

			case "link in":
				delete config.links;
				break;

			case "json": {
				if (!config.action)
					delete config.action;

				const property = [];
				property.push(`function (msg, value) {`);
				property.push(`\t\t\tif (undefined === value)`);
				property.push(`\t\t\t\treturn msg.${config.property};`);
				property.push(`\t\t\tmsg.${config.property} = value;`);
				property.push(`\t\t}`);
				config.property = property.join("\n");
			} break;

			case "csv": {
				config.template = (config.temp || "");
				config.sep = (config.sep || ',').replaceAll("\\t","\t").replaceAll("\\n","\n").replaceAll("\\r","\r");
				config.quo = '"';
				config.ret = (config.ret || "\n").replaceAll("\\n","\n").replaceAll("\\r","\r");
				config.winflag = (config.ret === "\r\n");
				config.lineend = "\n";
				config.multi = config.multi || "one";
				config.hdrin = config.hdrin || false;
				config.hdrout = config.hdrout || "none";
				config.skip = parseInt(config.skip || 0);
				config.parsestrings = config.strings;
				config.include_empty_strings = config.include_empty_strings || false;
				config.include_null_values = config.include_null_values || false;
				if (config.parsestrings === undefined) { config.parsestrings = true; }
				if (config.hdrout === false) { config.hdrout = "none"; }
				if (config.hdrout === true) { config.hdrout = "all"; }

				delete config.temp;
			} break;

			case "template": {
				config.output ??= "str";
				if (("json" !== config.output) && ("str" !== config.output))
					throw new Error("unsupported output " + config.output);

				if (("msg" !== config.fieldType) || ("payload" !== config.field))
					throw new Error("unsupported property");
				
				delete config.format;
				delete config.fieldType;
				delete config.field;
			} break;

			case "http in": {
				if (config.upload)
					throw new Error("upload unimplemented");
				
				config.method = config.method.toUpperCase();

				delete config.swaggerDoc;
				delete config.upload;
			} break;

			case "http response": {
				config.statusCode = parseInt(config.statusCode);

				const headers = [];
				for (const name in config.headers)
					headers.push([name.toLowerCase(), config.headers[name]]);
				if (headers.length)
					config.headers = headers;
				else
					delete config.headers;
			} break;
		
			case "mqtt in": {
				if (!nodes.has(config.broker))
					throw new Error(`mqtt broker id "${config.broker}" not found`);
					
				config.qos = (undefined === config.qos) ? 0 : parseInt(config.qos); 
			} break;

			case "mqtt out": {
				if (!nodes.has(config.broker))
					throw new Error(`mqtt broker id "${config.broker}" not found`);

				if ("" === config.topic)
					delete config.topic;

				if ("" === config.retain)
					delete config.retain;
				else
					config.retain = (true === config.retain) || ("true" === config.retain);

				if ("" === config.qos)
					delete config.qos;
				else
					config.qos = (undefined === config.qos) ? 0 : parseInt(config.qos); 
			} break;
			
			case "mqtt-broker": {
				const index = config.broker.indexOf("://");
				if ("" === config.broker)
					config.broker = "localhost";
				else if (-1 !== index) {
					const scheme = config.broker.slice(0, index);
					switch (scheme) {
						case "ws":
						case "wss":
							throw new Error("MQTT websocket tunnel unimplemented")
							break;

						case "mqtt":
							break;

						case "mqtts":
							throw new Error("MQTT TLS unimplemented")
							break;
						
						default:
							// Node-RED ignores all unrecognized schemes.
							break;
					}
					
					config.broker = config.broker.slice(index + 3);
				}

				config.port = config.port ? parseInt(config.port) : 1883;
				config.keepalive = (parseInt(config.keepalive) || 60) * 1000;

			} break;

			case "tcp in": {
				if ((undefined === config.port) || ("" === config.port))
					delete config.port;
				else
					config.port = parseInt(config.port);
				if ("" === config.topic)
					delete config.topic;
				if ("" === config.tls)
					delete config.tls;
				if (!config.trim)
					delete config.trim;
				if (!config.base64)
					delete config.base64;
				if (config.newline)
					config.newline = (config.newline).replaceAll("\\n","\n").replaceAll("\\r","\r").replaceAll("\\t","\t");
			} break;

			case "tcp out": {
				if ((undefined === config.port) || ("" === config.port))
					delete config.port;
				else
					config.port = parseInt(config.port);
				if ("" === config.tls)
					delete config.tls;
				if (!config.base64)
					delete config.base64;
				if (config.newline)
					config.newline = (config.newline).replaceAll("\\n","\n").replaceAll("\\r","\r").replaceAll("\\t","\t");
			} break;

			case "rpi-gpio in": {
				config.type = "mcu_digital_in";
				config.edge = 3;

				if (config.read)
					config.initial = true;

				if ("up" === config.intype)
					config.mode = "InputPullUp";
				else if ("down" === config.intype)
					config.mode = "InputPullDown";
				else
					config.mode = "Input";

				if (!config.debounce)
					config.debounce = 25;		// matches GPIOInNode in 36-rpi-gpio.js

				delete config.read;
				delete config.intype;
				delete config.bcm;

				return this.prepareNode(config.type, config, dones, errors, statuses, nodes, imports);
			} break;

			case "rpi-gpio out": {
				if ("pwm" === config.out) {
					config.type = "mcu_pwm_out";
					
					if (config.freq && !isNaN(parseFloat(config.freq)))
						config.hz = parseFloat(config.freq);
				}
				else {
					config.type = "mcu_digital_out";
					config.mode = "Output";
					
					if (config.set)
						config.initial = parseInt(config.level) ? 1 : 0
				}

				delete config.freq;
				delete config.set;
				delete config.level;
				delete config.bcm;
				delete config.out;

				return this.prepareNode(config.type, config, dones, errors, statuses, nodes, imports);
			} break;

			case "rpi-neopixels": {	// from node-red-nodes/hardware/neopixel/neopixel.js 
				config.pixels = parseInt(config.pixels || 1);
				config.bgnd = config.bgnd || "0,0,0";
				config.fgnd = config.fgnd || "128,128,128";
				config.mode = config.mode || "pcent";
				config.rgb = config.rgb || "rgb";
				if (config.gamma === undefined) { config.gamma = true; }
				config.gpio = config.gpio || 18;
				config.channel = 0;
				if (config.gpio == 13 || config.gpio == 19) { config.channel = 1; }
				config.brightness = Number(config.brightness || 100);
				config.wipe = Number(config.wipe || 40);
				if (config.wipe < 0) { config.wipe = 0; }
				if (config.brightness < 0) { config.brightness = 0; }
				if (config.brightness > 100) { config.brightness = 100; }
				
				config.type = "mcu_neopixels";
				config.foreground = config.fgnd;
				config.background = config.bgnd;
//				config.pin = config.gpio;			// gpio was never really used on rpi-neopixels, so just ignore it. projects should use mcu_neopixels.
				config.length = config.pixels;
				config.order = config.rgb.toUpperCase();
				config.mode = ["", "pcent", "pixels", "pcentneedle", "pixelsneedle", "shiftu", "shiftd"].indexOf(config.mode);

				delete config.gpio;
				delete config.fgnd;
				delete config.bgnd;
				delete config.gamma;
				delete config.pixels;
				delete config.rgb;
				delete config.channel;

				return this.prepareNode(config.type, config, dones, errors, statuses, nodes, imports);
			} break;

			case "mcu_neopixels":
				if (parseInt(config.pin) == config.pin)
					config.pin = parseInt(config.pin);
				if ("" === config.pin)
					delete config.pin;
				if (parseInt(config.length) == config.length)
					config.length = parseInt(config.length);
				if (!config.length)
					config.length = 1;
				if (config.brightness)
					config.brightness = parseFloat(config.brightness);
				if (config.wipe)
					config.wipe = parseFloat(config.wipe);
				if (!config.background)
					config.background = "#000000";
				if (!config.foreground)
					config.foreground = "#ffffff";
				config.mode = ["pcent", "pcent", "pixels", "pcentneedle", "pixelsneedle", "shiftu", "shiftd"][config.mode];
				break;

			case "mcu_analog":
				if (parseInt(config.pin) == config.pin)
					config.pin = parseInt(config.pin);
				if (config.resolution)
					config.resolution = parseFloat(config.resolution);
				else
					delete config.resolution;
				break;

			case "mcu_pwm_out":
				if (parseInt(config.pin) == config.pin)
					config.pin = parseInt(config.pin);
				if (config.hz)
					config.hz = parseFloat(config.hz);
				else
					delete config.hz;
				break;

			case "mcu_digital_in":
				if (parseInt(config.pin) == config.pin)
					config.pin = parseInt(config.pin);
				config.edge = parseInt(config.edge);
				config.debounce = config.debounce ? parseFloat(config.debounce) : 0;
				if (!config.debounce)
					delete config.debounce;
				if (!config.initial)
					delete config.initial;
				if (!config.invert)
					delete config.invert;
				break;

			case "mcu_digital_out":
				if (parseInt(config.pin) == config.pin)
					config.pin = parseInt(config.pin);
				if (1 == parseInt(config.initial))
					config.initial = 1;
				else if (0 == parseInt(config.initial))
					config.initial = 0;
				else
					delete config.initial;
				if (!config.invert)
					delete config.invert;
				break;

			case "mcu_pulse_width":
				if (parseInt(config.pin) == config.pin)
					config.pin = parseInt(config.pin);
				break;

			case "mcu_pulse_count":
				if (parseInt(config.signal) == config.signal)
					config.signal = parseInt(config.signal);
				if (parseInt(config.control) == config.control)
					config.control = parseInt(config.control);
				break;

			case "mcu_i2c_in":
			case "mcu_i2c_out":
				if (undefined != config.options.clock) {
					if (parseInt(config.options.clock) == config.options.clock)
						config.options.clock = parseInt(config.options.clock);
				}
				if (undefined != config.options.data) {
					if (parseInt(config.options.data) == config.options.data)
						config.options.data = parseInt(config.options.data);
				}
				if (isNaN(config.options.hz))
					config.options.hz = 100_000;
				else
					config.options.hz = parseInt(config.options.hz);
				if (isNaN(config.options.address))
					delete config.options.address; 
				else
					config.options.address = parseInt(config.options.address);
				if (isNaN(config.command) || ("" === config.command))
					delete config.command; 
				else
					config.command = parseInt(config.command);
				if (config.bytes)
					config.bytes = parseInt(config.bytes);
				else
					delete config.bytes;		//@@ check default behavior in RPi code
				
				if ("mcu_i2c_out" === type)
					config.getter = `function (msg) {return ${this.resolveValue(config.payloadType, config.payload)}}`;

				delete config.payload;
				delete config.payloadType;
				break;

			case "mcu_sensor":
				type = config.type = "sensor";
				// fall through
			case "mcu_clock":
			case "sensor": {
				const kinds = {
					mcu_clock: ["RTC", "rtc"],
					sensor: ["Sensor", "sensor"],
				}[type];

				if (config.io && !config.options) {		// convert original sensor config to new
					config.options = {
						sensor: {
							io: config.io,
							bus: config.bus ?? "default"
						}
					};
					delete config.io;
				}
			
				let construct;
				const initialize = [];
				initialize.push(`function () {`);

				if (config.options.reference)
					construct = config.options.reference;
				else {
					imports.set("Modules", "modules");
					construct = kinds[0];
					initialize.push(`\t\t\tconst ${construct} = Modules.importNow("${config.module}")`);
				}
				initialize.push(`\t\t\tconst ${kinds[1]} = new ${construct}({`);

				if (config.options) {
					for (const name in config.options) {
						const option = config.options[name];
						if (undefined === option.io)
							continue;

						switch (option.io) {
							case "Analog": {
								let pin = parseInt(option.pin);
								if (Number.isNaN(pin))
									pin = option.pin;
								initialize.push(`\t\t\t\t${name}: {`);
								initialize.push(`\t\t\t\t\tio: device.io.Analog,`);
								initialize.push(`\t\t\t\t\tpin: ${pin},`);
								initialize.push(`\t\t\t\t},`);
								} break;
							case "Digital":
							case "PulseWidth": {
								let pin = parseInt(option.pin);
								if (Number.isNaN(pin))
									pin = option.pin;
								initialize.push(`\t\t\t\t${name}: {`);
								initialize.push(`\t\t\t\t\tio: device.io.${option.io},`);
								initialize.push(`\t\t\t\t\tmode: device.io.${option.io}.${option.mode},`);
								initialize.push(`\t\t\t\t\tpin: ${pin},`);
								initialize.push(`\t\t\t\t},`);
								} break;
							case "I2C":
							case "SMBus": {
								initialize.push(`\t\t\t\t${name}: {`);
								if (option.bus || ((undefined === option.data) && (undefined === option.clock))) {
									initialize.push(`\t\t\t\t\t...device.I2C.${option.bus || "default"},`);
									if ("SMBus" === option.io)
										initialize.push(`\t\t\t\t\tio: device.io.SMBus,`);
								}
								else {
									initialize.push(`\t\t\t\t\tio: device.io.${option.io},`);

									let data = parseInt(option.data), clock = parseInt(option.clock);
									if (Number.isNaN(data))
										data = option.data;
									if (Number.isNaN(clock))
										clock = option.clock;

									initialize.push(`\t\t\t\t\tdata: ${data},`);
									initialize.push(`\t\t\t\t\tclock: ${clock},`);
								}

								if (undefined !== option.address)
									initialize.push(`\t\t\t\t\taddress: ${option.address},`);

								if (undefined !== option.hz)
									initialize.push(`\t\t\t\t\thz: ${option.hz},`);

								initialize.push(`\t\t\t\t},`);
								} break;
							case "Serial": {
								initialize.push(`\t\t\t\t${name}: {`);
								initialize.push(`\t\t\t\t\tio: device.io.${option.io},`);

								let receive = parseInt(option.receive), transmit = parseInt(option.transmit);
								if (Number.isNaN(receive))
									receive = option.receive;
								if (Number.isNaN(transmit))
									transmit = option.transmit;

								initialize.push(`\t\t\t\t\ttransmit: ${transmit},`);
								initialize.push(`\t\t\t\t\treceive: ${receive},`);
								
								if ((undefined !== option.port) && ("" !== option.port)) {
									let port = parseInt(option.port);
									if (Number.isNaN(port))
										port = option.port;
									initialize.push(`\t\t\t\t\tport: ${port},`);
								}

								if (undefined !== option.baud)
									initialize.push(`\t\t\t\t\tbaud: ${parseInt(option.baud)},`);

								initialize.push(`\t\t\t\t},`);
								} break;
							default:
								throw new Error(`Unknown io ${option.io}`);
						}
					}

					for (const name in config.options) {
						const option = config.options[name];
						if (option.callback)
							initialize.push(`\t\t\t\t${option.callback}: () => {const msg = this.onMessage({callback: "${name}"}); if (msg) this.send(msg);},`);
					}
				}

				initialize.push(`\t\t\t});`);

				if (config.configuration && ("{}" !== config.configuration)) {
					const configuration = JSON.parse(config.configuration);
					initialize.push(`\t\t\t${kinds[1]}.configure(${JSON.stringify(configuration)});`);
				}

				initialize.push(`\t\t\treturn ${kinds[1]};`);
				initialize.push(`\t\t}`);
				config.initialize = initialize.join("\n");

				delete config.module;
				delete config.platform;
				delete config.options;
				delete config.configuration;
				delete config.include;
			} break;

			case "random": {
				let low = config.low ? Number(config.low) : undefined;
				let high = config.high ? Number(config.high) : undefined;
				const integer = "true" === config.inte;
				const property = config.property || "payload";
				const random = [];

				if (((undefined !== low) && isNaN(low)) || ((undefined !== high) && isNaN(high)))
					throw new Error("invalid value for low and/or high");

				if (integer) {
					if (undefined !== low)
						low = Math.ceil(low);
					if (undefined !== high)
						high = Math.floor(high);
				}

				let value;
				random.push(`function (msg) {`);
				if ((undefined !== low) && (undefined !== high)) {
					if (low > high) {
						const t = high;
						high = low;
						low = t;
					}

					if (integer)
						value = `Math.round(Math.random() * ${high - low + 1} + ${low - 0.5})`;
					else
						value = `(Math.random() * ${high - low}) + ${low}`;
				}
				else {
					if (undefined !== low)
						random.push(`\t\t\tlet low = ${low};`);
					else {
						random.push(`\t\t\tlet low = Number(msg.from ?? 1);`);
						random.push(`\t\t\tif (isNaN(low)) return this.error("invalid low");`);
					}

					if (undefined !== high)
						random.push(`\t\t\tlet high = ${high};`);
					else {
						random.push(`\t\t\tlet high = Number(msg.to ?? 10);`);
						random.push(`\t\t\tif (isNaN(high)) return this.error("invalid high");`);
					}

					random.push(`\t\t\tif (low > high) {`);
					random.push(`\t\t\t\tconst t = high;`);
					random.push(`\t\t\t\thigh = low;`);
					random.push(`\t\t\t\tlow = t;`);
					random.push(`\t\t\t}`);
					
					if (integer) {
						random.push(`\t\t\tlow  = Math.ceil(low);`);
						value = `Math.round(Math.random() * (Math.floor(high) - low + 1) + low - 0.5)`;
					}
					else
						value = `(Math.random() * (high - low)) + low`;
				}
				random.push(`\t\t\tmsg${this.prepareProp(property)} = ${value};`);

				random.push(`\t\t\treturn msg;`);
				random.push(`\t\t}`);
				config.onMessage = random.join("\n");

				delete config.low;
				delete config.high;
				delete config.inte;
				delete config.property;
			} break;
			
			case "ui_button": {
				const setter = [];
				setter.push(`function (target, msg) {`);
				setter.push(`\t\t\ttarget.payload = ${this.resolveValue(config.payloadType, config.payload)};`);
				if (config.topic) {
					setter.push(`\t\t\tconst topic = ${this.resolveValue(config.topicType, config.topic)};`);
					setter.push(`\t\t\tif (undefined !== topic)`);
					setter.push(`\t\t\t\ttarget.topic = topic;`);
				}
				setter.push(`\t\t}`);
				config.setter = setter.join("\n");

				delete config.topic; 
				delete config.topicType; 
				delete config.payload; 
				delete config.payloadType; 

				this.prepareUI(config, nodes);
			} break;

			case "ui_switch": {
				const topic = config.topic ? `, topic: ${this.resolveValue(config.topicType, config.topic)}` : ""; 
				const options = [];
				options.push(`function (msg) {`);
				options.push(`\t\t\treturn [`);
				options.push(`\t\t\t\t{payload: ${this.resolveValue(config.offvalueType, config.offvalue)}${topic}},`);
				options.push(`\t\t\t\t{payload: ${this.resolveValue(config.onvalueType, config.onvalue)}${topic}}`);
				options.push(`\t\t\t];`);
				options.push(`\t\t}`);
				config.options = options.join("\n");
				
				delete config.topic; 
				delete config.topicType; 
				delete config.offvalue; 
				delete config.offvalueType; 
				delete config.onvalue; 
				delete config.onvalueType; 

				this.prepareUI(config, nodes);
			} break;

			case "ui_colour_picker":
			case "ui_text_input":
			case "ui_numeric":
			case "ui_slider": {
				const topic = [];
				topic.push(`function (msg) {`);
				topic.push(`\t\t\treturn ${config.topic ? this.resolveValue(config.topicType, config.topic) : ""};`); 
				topic.push(`\t\t}`);
				config.topic = topic.join("\n");
				delete config.topicType;
				
				this.prepareUI(config, nodes);
			} break;
			
			case "ui_chart":
			case "ui_gauge":
			case "ui_group":
			case "ui_spacer":
			case "ui_text":
			case "ui_template":
			case "ui_toast":
				this.prepareUI(config, nodes);
				break;
		}

		if (dones && !NoDoneNodes.includes(type))
			config.dones = dones;

		if (statuses && !NoStatusNodes.includes(type))
			config.statuses = statuses;

		if (errors && !NoErrorNodes.includes(type))
			config.errors = errors;

		// if no outputs, delete config.wires
		const wires = config.wires;
		if (wires) {
			if (wires.length) {
				if (!wires.some(wire => wire.length))
					delete config.wires;
			}
			else
				delete config.wires;
		}

		// name not needed in config, as passed to constructor
		delete config.name;
	}
	resolveValue(type, value) {
		switch (type) {
			case "bool":
				return "true" === value;
			case "nul":
			case "null":
				return null;
			case "date":
				return "Date.now()";
			case "json":
				if (!value)
					throw new Error("missing value");
				return value;
			case "num":
				if ("" === value)		// historical: https://cookbook.nodered.org/basic/join-streams
					return 0;
				return parseFloat(value);
			case "str":
				return `${JSON.stringify(value ?? "")}`;
			case "re":
				return `/${value}/`;
			case "bin":
				if (!value)
					return `Uint8Array.of()`; 
				return `Uint8Array.of(${value.slice(1, value.length - 1)})`;
			case "msg":
				return `msg${this.prepareProp(value)}`;
			case "flow":
			case "global": {
				if (!value)
					throw new Error(`missing name`);
				let suffix = "";
				let i = value.indexOf("[");
				let j = value.indexOf(".");
				if ((i > 0) || (j > 0)) {
					let first;
					if ((i > 0) && (j < 0))
						first = i;
					else if ((j > 0) && (i < 0))
						first = j;
					else
						first = Math.min(i, j);
					suffix = value.slice(first);
					value = value.slice(0, first);		//@@ if "." may need to check regexIdentifierNameES6
				}

				suffix = suffix.trim(suffix)
				if (suffix) {
					if (suffix.startsWith("["))
						suffix = "?." + suffix;
					else
						suffix = "?" + suffix;
				}
				if ("flow" === type)
					return `this.flow.get(${this.makeStorageArgs(value)})${suffix}`;
				return `globalContext.get(${this.makeStorageArgs(value)})${suffix}`;
				}
			case "env": {
				let offset = 0;
				do {
					offset = value.indexOf("${", offset);
					if (offset < 0)
						break;
					let end = value.indexOf("}", offset);
					if (end < 0)
						break;
					const name = value.slice(offset + 2, end);
					const substitute = `this.getSetting("${name}")`;
					value = value.slice(0, offset + 2) +
							substitute +
							value.slice(end);
					end += substitute.length - name.length;
					offset = end + 1;
				} while (true);
				
				if (value.indexOf("${", offset) < 0)
					return `this.getSetting("${value}")`;
				if (value.startsWith("${") && value.endsWith("}") && (value.indexOf("${", 2) < 0))
					return value.slice(2, -1);
				return "`" + value + "`";
				}
			default:
				throw new Error(`cannot resolve type "${type}"`);
		}
	}
	resolveSwitch(type, value) {
		if (!type)
			return;

		if ("prev" === type)
			return `previous`;

		return this.resolveValue(type, value);
	}
	splitProp(prop) {
		let parts = [], start = 0, depth = 0;
		for (let position = 0; position < prop.length; position++) {
			const c = prop[position];
			if (position === (prop.length - 1)) {
				parts.push(prop.slice(start));
				break;
			}

			if ((0 === depth) && ("." === c)) {
				if (start !== position)
					parts.push(prop.slice(start, position));
				start = position + 1;
			}
			else
			if ("[" === c) {
				if (0 === depth) {
					parts.push(prop.slice(start, position));
					start = position;
				}
				depth += 1;
			}
			else
			if ("]" === c) {
				depth -= 1;
				if (0 === depth) {
					parts.push(prop.slice(start, position + 1));
					start = position + 1;
				}
			}
		}

		return parts.map(part => {
			if (part.startsWith("["))
				return part;
			const identifier = regexIdentifierNameES6.test(part);
			if (identifier)
				return "." + part;
			return `["${part}"]`;
		});
	}
	prepareProp(prop) {
		return this.splitProp(prop).join("");
	}
	createPropPath(prop, code, indent) {
		const parts = this.splitProp(prop);
		if (parts.length > 1) {
			for (let i = 0, path = ""; i < parts.length - 1; i++) {
				path += parts[i];
				code.push(`${indent}msg${path} ??= {};`);
			}
		}
	}
	prepareUI(config, nodes) {
		if (!nodes.ui_base)
			throw new Error("ui_base node not found. required for dashboard nodes.")

		if (config.width)
			config.width = parseInt(config.width);
		if (config.height)
			config.height = parseInt(config.height);
		if (config.min)
			config.min = parseInt(config.min);
		if (config.max)
			config.max = parseInt(config.max);
		if (config.displayTime)
			config.displayTime = parseInt(config.displayTime);
		if ("" === config.className)
			delete config.className;
		delete config.tooltip;
	}
	prepareEnv(env) {
		const parts = [];

		env.forEach(env => {
			const value = this.resolveValue(env.type, env.value);
			const name = regexIdentifierNameES6.test(env.name) ? `${env.name}` : `["${env.name}"]`; 
			switch (env.type) {
				case "str":
				case "num":
				case "bool":
				case "bin":
					parts.push(`\t\t${name}: ${value},`);
					break;
				case "json": {
					let t = JSON.parse(value);
					t = JSON.stringify(t, null, "\t");
					t = t.split("\n").map((line, i) => (i ? "\t\t" : "") + line).join("\n");	// indent multi-line JSON
					parts.push(`\t\t${name}: ${t},`);
					} break;
			}					
		});
		
		return parts;
	}
	applyEnv(config, name, node, flows) {
		const value = config[name];

		if (value instanceof Uint8Array)
			return;

		if (Array.isArray(value)) {
			for (let i=0, length = value.length ; i < length; i++)
				this.applyEnv(value, i, node, flows);
			return;
		}

		if ("string" == typeof value) {
			if (("$" === value[0]) && (EnvVarPropertyRE_old.test(value) || EnvVarPropertyRE.test(value)) ) {
				const v = this.getSetting(value.substring(2, value.length - 1), node, flows);
				if (v)
					config[name] = v;
			}
			return;
		}

		for (let name in value) {
			if (Object.hasOwn(value, name))
				this.applyEnv(value, name, node, flows);
		}
	}
	getSetting(name, node, flows) {		//@@ subflows
		const type = node.type;

		if ("tab" == type) {
			if ("NR_FLOW_NAME" === name)
				return node.label;
			if (("NR_FLOW_ID" === name) || ("NR_NODE_PATH" === name))
				return node.id;
			return this.getEnv(node.env, name);
		}
		else if ("group" === type) {
			if ("NR_GROUP_NAME" == name)
				return node.label;
			if ("NR_GROUP_ID" == name)
				return node.id;

			const v = this.getEnv(node.env, name);
			if (v)
				return v;
		}
		else {		// regular node
			if ("NR_NODE_NAME" === name)
				return node.name;
			if ("NR_NODE_ID" === name)
				return node.id;
		}

		const id = node.g ?? node.z;		// parent
		if (id) {
			const parent = flows.find(config => config.id === id);

			if (("NR_NODE_PATH" == name) && ("tab" == type))
				return this.getSetting(name, parent, flows) + "/" + node.id; 

			return this.getSetting(name, parent, flows);
		}
	}
	getEnv(env, name) {
		env = env.find(e => e.name === name);
		if (!env) return;

		switch (env.type) {
			case "str":
			case "num":
			case "bool":
			case "json":
			case "bin":
				return env.value; 
			default:
				throw new Error(`environment type "${env.type}" unsupported on "${name}"`);
		}
	}
	makeStorageArgs(name, value) {
		if (name.startsWith("#:(") && name.includes("::")) {
			name = name.split("::");
			if ("#:(file)" !== name[0])
				throw new Error("unsupported storage " + name[0]);
			if (undefined !== value)
				return `"${name[1]}", ${value}, "file"`;
			return `"${name[1]}", "file"`;
		}
		if (undefined !== value)
			return `"${name}", ${value}`;
		return `"${name}"`;
	}
}
