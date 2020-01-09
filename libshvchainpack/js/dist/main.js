/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, { enumerable: true, get: getter });
/******/ 		}
/******/ 	};
/******/
/******/ 	// define __esModule on exports
/******/ 	__webpack_require__.r = function(exports) {
/******/ 		if(typeof Symbol !== 'undefined' && Symbol.toStringTag) {
/******/ 			Object.defineProperty(exports, Symbol.toStringTag, { value: 'Module' });
/******/ 		}
/******/ 		Object.defineProperty(exports, '__esModule', { value: true });
/******/ 	};
/******/
/******/ 	// create a fake namespace object
/******/ 	// mode & 1: value is a module id, require it
/******/ 	// mode & 2: merge all properties of value into the ns
/******/ 	// mode & 4: return value when already ns object
/******/ 	// mode & 8|1: behave like require
/******/ 	__webpack_require__.t = function(value, mode) {
/******/ 		if(mode & 1) value = __webpack_require__(value);
/******/ 		if(mode & 8) return value;
/******/ 		if((mode & 4) && typeof value === 'object' && value && value.__esModule) return value;
/******/ 		var ns = Object.create(null);
/******/ 		__webpack_require__.r(ns);
/******/ 		Object.defineProperty(ns, 'default', { enumerable: true, value: value });
/******/ 		if(mode & 2 && typeof value != 'string') for(var key in value) __webpack_require__.d(ns, key, function(key) { return value[key]; }.bind(null, key));
/******/ 		return ns;
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = 0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony import */ var _rpcmessage__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(1);
/* harmony import */ var _rpcvalue__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(2);
/* harmony import */ var _chainpack__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(5);
/* harmony import */ var _cpon__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(3);
/* harmony import */ var _cpcontext__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(4);
/* harmony import */ var _test__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(7);







window.RpcMessage = _rpcmessage__WEBPACK_IMPORTED_MODULE_0__["RpcMessage"]
window.RpcValue = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"]
window.ChainPackWriter = _chainpack__WEBPACK_IMPORTED_MODULE_2__["ChainPackWriter"]
window.Cpon = _cpon__WEBPACK_IMPORTED_MODULE_3__["Cpon"]
window.ChainPackReader = _chainpack__WEBPACK_IMPORTED_MODULE_2__["ChainPackReader"]
window.UnpackContext = _cpcontext__WEBPACK_IMPORTED_MODULE_4__["UnpackContext"]
window.CponReader = _cpon__WEBPACK_IMPORTED_MODULE_3__["CponReader"]
window.ChainPack = _chainpack__WEBPACK_IMPORTED_MODULE_2__["ChainPack"]
window.Test = _test__WEBPACK_IMPORTED_MODULE_5__["Test"]


/***/ }),
/* 1 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "RpcMessage", function() { return RpcMessage; });


function RpcMessage(rpc_val)
{
	if(typeof rpc_val === 'undefined')
		this.rpcValue = new RpcValue();
	else if(typeof rpc_val === 'null')
		this.rpcValue = null;
	else if(rpc_val && rpc_val.constructor.name === "RpcValue")
		this.rpcValue = rpc_val;
	else
		throw new TypeError("RpcMessage cannot be constructed with " + typeof rpc_val)

	if(this.rpcValue) {
		if(!this.rpcValue.meta)
			this.rpcValue.meta = {}
		if(!this.rpcValue.value)
			this.rpcValue.value = {}
		this.rpcValue.type = RpcValue.Type.IMap
	}
}

RpcMessage.TagRequestId = "8";
RpcMessage.TagShvPath = "9";
RpcMessage.TagMethod = "10";

RpcMessage.KeyParams = "1";
RpcMessage.KeyResult = "2";
RpcMessage.KeyError = "3";

RpcMessage.prototype.isValid = function() {return this.rpcValue? true: false; }
RpcMessage.prototype.isRequest = function() {return this.requestId() && this.method(); }
RpcMessage.prototype.isResponse = function() {return this.requestId() && !this.method(); }
RpcMessage.prototype.isSignal = function() {return !this.requestId() && this.method(); }

RpcMessage.prototype.requestId = function() {return this.isValid()? this.rpcValue.meta[RpcMessage.TagRequestId]: 0; }
RpcMessage.prototype.setRequestId = function(id) {return this.rpcValue.meta[RpcMessage.TagRequestId] = id; }

RpcMessage.prototype.shvPath = function() {return this.isValid()? this.rpcValue.meta[RpcMessage.TagShvPath]: null; }
RpcMessage.prototype.setShvPath = function(val) {return this.rpcValue.meta[RpcMessage.TagShvPath] = val; }

RpcMessage.prototype.method = function() {return this.isValid()? this.rpcValue.meta[RpcMessage.TagMethod]: null; }
RpcMessage.prototype.setMethod = function(val) {return this.rpcValue.meta[RpcMessage.TagMethod] = val; }

RpcMessage.prototype.params = function() {return this.isValid()? this.rpcValue.value[RpcMessage.KeyParams]: null; }
RpcMessage.prototype.setParams = function(params) {return this.rpcValue.value[RpcMessage.KeyParams] = params; }

RpcMessage.prototype.result = function() {return this.isValid()? this.rpcValue.value[RpcMessage.KeyResult]: null; }
RpcMessage.prototype.setResult = function(result) {return this.rpcValue.value[RpcMessage.KeyResult] = result; }

RpcMessage.prototype.error = function() {return this.isValid()? this.rpcValue.value[RpcMessage.KeyError]: null; }
RpcMessage.prototype.setError = function(err) {return this.rpcValue.value[RpcMessage.KeyError] = err; }

RpcMessage.prototype.toString = function() {return this.isValid() ? this.rpcValue.toString() : ""; }
RpcMessage.prototype.toCpon = function() {return this.isValid()? this.rpcValue.toCpon(): ""; }
RpcMessage.prototype.toChainPack = function() {return this.isValid()? this.rpcValue.toChainPack(): ""; }




/***/ }),
/* 2 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "RpcValue", function() { return RpcValue; });
/* harmony import */ var _cpon__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(3);




function RpcValue(value, meta, type)
{
	if(value)
		this.value = value;
	if(meta)
		this.meta = meta;
	if(type) {
		this.type = type;
	}
	else {
		if(typeof value == "null")
			this.type = RpcValue.Type.Null;
		else if(typeof value == "boolean")
			this.type = RpcValue.Type.Bool;
		else if(typeof value == "string") {
			this.value = _cpon__WEBPACK_IMPORTED_MODULE_0__["Cpon"].stringToUtf8(value);
			this.type = RpcValue.Type.String;
		}
		else if(Array.isArray(value))
			this.type = RpcValue.Type.List;
		else if(typeof value == "Object") {
			if(value.constructor.name === "Date") {
				this.value = {epochMsec: value.valueOf(), utcOffsetMin: -value.getTimezoneOffset()}
				this.type = RpcValue.Type.DateTime;
			}
			else {
				this.type = RpcValue.Type.Map;
			}
		}
		else if(Number.isInteger(value))
			this.type = RpcValue.Type.Int;
		else if(Number.isFinite(value))
			this.type = RpcValue.Type.Double;
	}
}

RpcValue.Type = Object.freeze({
	"Null": 1,
	"Bool": 2,
	"Int": 3,
	"UInt": 4,
	"Double": 5,
	"Decimal": 6,
	"String": 7,
	"DateTime": 8,
	"List": 9,
	"Map": 10,
	"IMap": 11,
	//"Meta": 11,
})

RpcValue.fromCpon = function(cpon)
{
	let unpack_context = null;
	if(typeof cpon === 'string') {
		unpack_context = new UnpackContext(_cpon__WEBPACK_IMPORTED_MODULE_0__["Cpon"].stringToUtf8(cpon));
	}
	if(unpack_context === null)
		throw new TypeError("Invalid input data type")
	let rd = new CponReader(unpack_context);
	return rd.read();
}

RpcValue.fromChainPack = function(data)
{
	let unpack_context = new UnpackContext(data);
	let rd = new ChainPackReader(unpack_context);
	return rd.read();
}

RpcValue.prototype.toInt = function()
{
	switch(this.type) {
		case RpcValue.Type.Int:
		case RpcValue.Type.UInt:
			return this.value;
		case RpcValue.Type.Decimal:
			return this.value.mantisa * (10 ** this.value.exponent);
		case RpcValue.Type.Double:
			return (this.value >> 0);
	}
	return 0;
}

RpcValue.prototype.toString = function()
{
	let ba = this.toCpon();
	return _cpon__WEBPACK_IMPORTED_MODULE_0__["Cpon"].utf8ToString(ba);
}

RpcValue.prototype.toCpon = function()
{
	let wr = new _cpon__WEBPACK_IMPORTED_MODULE_0__["CponWriter"]();
	wr.write(this);
	return wr.ctx.buffer();
}

RpcValue.prototype.toChainPack = function()
{
	let wr = new ChainPackWriter();
	wr.write(this);
	return wr.ctx.buffer();
}

RpcValue.prototype.isValid = function()
{
	return this.value && this.type;
}


/***/ }),
/* 3 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Cpon", function() { return Cpon; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "CponWriter", function() { return CponWriter; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "CponReader", function() { return CponReader; });
/* harmony import */ var _cpcontext__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(4);
/* harmony import */ var _rpcvalue__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(2);





function Cpon()
{
}

Cpon.ProtocolType = 2;

Cpon.utf8ToString = function(bytearray)
{
	let uint8_array = new Uint8Array(bytearray)
	var str = '';
	for (let i = 0; i < uint8_array.length; i++) {
		var value = uint8_array[i];

		if (value < 0x80) {
			str += String.fromCharCode(value);
		}
		else if (value > 0xBF && value < 0xE0) {
			str += String.fromCharCode((value & 0x1F) << 6 | uint8_array[i + 1] & 0x3F);
			i += 1;
		}
		else if (value > 0xDF && value < 0xF0) {
			str += String.fromCharCode((value & 0x0F) << 12 | (uint8_array[i + 1] & 0x3F) << 6 | uint8_array[i + 2] & 0x3F);
			i += 2;
		}
		else {
			// surrogate pair
			var char_code = ((value & 0x07) << 18 | (uint8_array[i + 1] & 0x3F) << 12 | (uint8_array[i + 2] & 0x3F) << 6 | data[i + 3] & 0x3F) - 0x010000;

			str += String.fromCharCode(char_code >> 10 | 0xD800, char_code & 0x03FF | 0xDC00);
			i += 3;
		}
	}
	return str;
}

Cpon.stringToUtf8 = function(str)
{
	let wr = new CponWriter();
	wr.ctx.writeStringUtf8(str);
	return wr.ctx.buffer();
}

function CponReader(unpack_context)
{
	this.ctx = unpack_context;
}

CponReader.prototype.skipWhiteIsignificant = function()
{
	const SPACE = ' '.charCodeAt(0);
	const SLASH = '/'.charCodeAt(0);
	const STAR = '*'.charCodeAt(0);
	const LF = '\n'.charCodeAt(0);
	const KEY_DELIM = ':'.charCodeAt(0);
	const FIELD_DELIM = ','.charCodeAt(0);
	while(true) {
		let b = this.ctx.peekByte();
		if(b < 1)
			return;
		if(b > SPACE) {
			if(b === SLASH) {
				this.ctx.getByte();
				b = this.ctx.getByte();
				if(b === STAR) {
					//multiline_comment_entered;
					while(true) {
						b = this.ctx.getByte();
						if(b === STAR) {
							b = this.ctx.getByte();
							if(b === SLASH)
								break;
						}
					}
				}
				else if(b === SLASH) {
					// to end of line comment entered;
					while(true) {
						b = this.ctx.getByte();
						if(b === LF)
							break;
					}
				}
				else {
					throw new TypeError("Malformed comment");
				}
			}
			else if(b === KEY_DELIM) {
				this.ctx.getByte();
				continue;
			}
			else if(b === FIELD_DELIM) {
				this.ctx.getByte();
				continue;
			}
			else {
				break;
			}
		}
		else {
			this.ctx.getByte();
		}
	}
}

CponReader.prototype.read = function()
{
	let ret = new _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"]();
	this.skipWhiteIsignificant();
	let b = this.ctx.peekByte();
	if(b == '<'.charCodeAt(0)) {
		let rv = new _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"]();
		this.readMap(rv, ">".charCodeAt(0));
		ret.meta = rv.value;
	}

	this.skipWhiteIsignificant();
	b = this.ctx.peekByte();
	//console.log("CHAR:", b, String.fromCharCode(b));
	// [0-9+-]
	if((b >= 48 && b <= 57) || b == 43 || b == 45) {
		this.readNumber(ret);
	}
	else if(b == '"'.charCodeAt(0)) {
		this.readCString(ret);
	}
	else if(b == "[".charCodeAt(0)) {
		this.readList(ret);
		ret.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.List;
	}
	else if(b == "{".charCodeAt(0)) {
		this.readMap(ret);
		ret.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Map;
	}
	else if(b == "i".charCodeAt(0)) {
		this.ctx.getByte();
		b = this.ctx.peekByte();
		if(b == "{".charCodeAt(0)) {
			this.readMap(ret);
			ret.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.IMap;
		}
		else {
			throw TypeError("Invalid IMap prefix.")
		}
	}
	else if(b == "d".charCodeAt(0)) {
		this.ctx.getByte();
		b = this.ctx.peekByte();
		if(b == '"'.charCodeAt(0)) {
			this.readDateTime(ret);
		}
		else {
			throw TypeError("Invalid DateTime prefix.")
		}
	}
	else if(b == 't'.charCodeAt(0)) {
		this.ctx.getBytes("true");
		ret.value = true;
		ret.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Bool;
	}
	else if(b == 'f'.charCodeAt(0)) {
		this.ctx.getBytes("false");
		ret.value = false;
		ret.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Bool;
	}
	else if(b == 'n'.charCodeAt(0)) {
		this.ctx.getBytes("null");
		ret.value = null;
		ret.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Null;
	}
	else {
		throw TypeError("Malformed Cpon input.")
	}
	return ret;
}
/*
// see http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
// see https://stackoverflow.com/questions/16647819/timegm-cross-platform
// see https://www.boost.org/doc/libs/1_62_0/boost/chrono/io/time_point_io.hpp
CponReader.isLeapYear = function(year)
{
	return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

CponReader.daysFromYear0 = function(year)
{
	year--;
	return 365 * year + ((year / 400) >> 0) - ((year/100) >> 0) + ((year / 4) >> 0);
}

CponReader.daysFrom1970 = function(year)
{
	return daysFromYear0(year) - days_from_0(1970);
}

CponReader.daysFromJan1st = function(year, month, mday)
{
	const days = [
		[ 0,31,59,90,120,151,181,212,243,273,304,334],
		[ 0,31,60,91,121,152,182,213,244,274,305,335]
	]

	return days[CponReader.isLeapYear(year)? 1: 0][month] + mday - 1;
}

CponReader.timegm = function(year, month, mday, hour, min, sec)
{
	// leap seconds are not part of Posix
	let res = 0;
	year = year + 1900;
	// month  0 - 11
	// mday  1 - 31
	res = CponReader.daysFrom1970(year);
	res += CponReader.daysFromJan1st(year, month, mday);
	res *= 24;
	res += hour;
	res *= 60;
	res += min;
	res *= 60;
	res += sec;
	return res;
}
*/
CponReader.prototype.readDateTime = function(rpc_val)
{

	let year = 0;
	let month = 0;
	let day = 1;
	let hour = 0;
	let min = 0;
	let sec = 0;
	let msec = 0;
	let utc_offset = 0;

	this.ctx.getByte(); // eat '"'
	let b = this.ctx.peekByte();
	if(b === '"'.charCodeAt(0)) {
		// d"" invalid data time
		this.ctx.getByte();
		rpc_val.value = null;
		rpc_val.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.DateTime;
		return;
	}

	year = this.readInt();

	b = this.ctx.getByte();
	if(b !== '-'.charCodeAt(0))
		throw new TypeError("Malformed year-month separator in DateTime");
	month = this.readInt();

	b = this.ctx.getByte();
	if(b !== '-'.charCodeAt(0))
		throw new TypeError("Malformed year-month separator in DateTime");
	day = this.readInt();

	b = this.ctx.getByte();
	if(b !== ' '.charCodeAt(0) && b !== 'T'.charCodeAt(0))
		throw new TypeError("Malformed date-time separator in DateTime");
	hour = this.readInt();

	b = this.ctx.getByte();
	if(b !== ':'.charCodeAt(0))
		throw new TypeError("Malformed year-month separator in DateTime");
	min = this.readInt();

	b = this.ctx.getByte();
	if(b !== ':'.charCodeAt(0))
		throw new TypeError("Malformed year-month separator in DateTime");
	sec = this.readInt();

	b = this.ctx.peekByte();
	if(b === '.'.charCodeAt(0)) {
		this.ctx.getByte();
		msec = this.readInt();
	}

	b = this.ctx.peekByte();
	if(b == 'Z'.charCodeAt(0)) {
		// zulu time
		this.ctx.getByte();
	}
	else if(b === '+'.charCodeAt(0) || b === '-'.charCodeAt(0)) {
		// UTC time offset
		this.ctx.getByte();
		let ix1 = this.ctx.index;
		let val = this.readInt();
		let n = this.ctx.index - ix1;
		if(!(n === 2 || n === 4))
			throw new TypeError("Malformed TS offset in DateTime.");
		if(n === 2)
			utc_offset = 60 * val;
		else if(n === 4)
			utc_offset = 60 * ((val / 100) >> 0) + (val % 100);
		if(b == '-'.charCodeAt(0))
			utc_offset = -utc_offset;
	}

	b = this.ctx.getByte();
	if(b !== '"'.charCodeAt(0))
		throw new TypeError("DateTime literal should be terminated by '\"'.");

	//let epoch_sec = CponReader.timegm(year, month, mday, hour, min, sec);
	let epoch_msec = Date.UTC(year, month - 1, day, hour, min, sec);
	epoch_msec -= utc_offset * 60000;
	rpc_val.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.DateTime;
	rpc_val.value = {epochMsec: epoch_msec + msec, utcOffsetMin: utc_offset};
}

CponReader.prototype.readCString = function(rpc_val)
{
	let pctx = new _cpcontext__WEBPACK_IMPORTED_MODULE_0__["PackContext"]();
	this.ctx.getByte(); // eat '"'
	while(true) {
		let b = this.ctx.getByte();
		if(b == '\\'.charCodeAt(0)) {
			b = this.ctx.getByte();
			switch (b) {
			case '\\'.charCodeAt(0): pctx.putByte("\\"); break;
			case '"'.charCodeAt(0): pctx.putByte('"'); break;
			case 'b'.charCodeAt(0): pctx.putByte("\b"); break;
			case 'f'.charCodeAt(0): pctx.putByte("\f"); break;
			case 'n'.charCodeAt(0): pctx.putByte("\n"); break;
			case 'r'.charCodeAt(0): pctx.putByte("\r"); break;
			case 't'.charCodeAt(0): pctx.putByte("\t"); break;
			case '0'.charCodeAt(0): pctx.putByte(0); break;
			default: pctx.putByte(b); break;
			}
		}
		else {
			if (b == '"'.charCodeAt(0)) {
				// end of string
				break;
			}
			else {
				pctx.putByte(b);
			}
		}
	}
	rpc_val.value = pctx.buffer();
	rpc_val.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.String;
}

CponReader.prototype.readList = function(rpc_val)
{
	let lst = []
	this.ctx.getByte(); // eat '['
	while(true) {
		this.skipWhiteIsignificant();
		let b = this.ctx.peekByte();
		if(b == "]".charCodeAt(0)) {
			this.ctx.getByte();
			break;
		}
		let item = this.read()
		lst.push(item);
	}
	rpc_val.value = lst;
	rpc_val.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.List;
}

CponReader.prototype.readMap = function(rpc_val, terminator = "}".charCodeAt(0))
{
	let map = {}
	this.ctx.getByte(); // eat '{'
	while(true) {
		this.skipWhiteIsignificant();
		let b = this.ctx.peekByte();
		if(b == terminator) {
			this.ctx.getByte();
			break;
		}
		let key = this.read()
		if(!key.isValid())
			throw new TypeError("Malformed map, invalid key");
		this.skipWhiteIsignificant();
		let val = this.read()
		if(key.type === _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.String)
			map[key.toString().slice(1, -1)] = val;
		else
			map[key.toInt()] = val;
	}
	rpc_val.value = map;
}

CponReader.prototype.readInt = function()
{
	let base = 10;
	let val = 0;
	let neg = 0;
	let n = 0;
	for (; ; n++) {
		let b = this.ctx.peekByte();
		if(b < 0)
			break;
		if (b === 43 || b === 45) { // '+','-'
			if(n != 0)
				break;
			this.ctx.getByte();
			if(b === 45)
				neg = 1;
		}
		else if (b === 120) { // 'x'
			if(n === 1 && val !== 0)
				break;
			if(n !== 1)
				break;
			this.ctx.getByte();
			base = 16;
		}
		else if( b >= 48 && b <= 57) { // '0' - '9'
			this.ctx.getByte();
			val *= base;
			val += b - 48;
		}
		else if( b >= 65 && b <= 70) { // 'A' - 'F'
			if(base !== 16)
				break;
			this.ctx.getByte();
			val *= base;
			val += b - 65 + 10;
		}
		else if( b >= 97 && b <= 102) { // 'a' - 'f'
			if(base !== 16)
				break;
			this.ctx.getByte();
			val *= base;
			val += b - 97 + 10;
		}
		else {
			break;
		}
	}

	if(neg)
		val = -val;
	return val;

}

CponReader.prototype.readNumber = function(rpc_val)
{
	let mantisa = 0;
	let exponent = 0;
	let decimals = 0;
	let dec_cnt = 0;
	let is_decimal = false;
	let is_uint = false;
	let is_neg = false;

	let b = this.ctx.peekByte();
	if(b == 43) {// '+'
		is_neg = false
		b = this.ctx.getByte();
	}
	else if(b == 45) {// '-'
		is_neg = true
		b = this.ctx.getByte();
	}

	mantisa = this.readInt();
	b = this.ctx.peekByte();
	while(b > 0) {
		if(b == "u".charCodeAt(0)) {
			is_uint = 1;
			this.ctx.getByte();
			break;
		}
		if(b == ".".charCodeAt(0)) {
			is_decimal = 1;
			this.ctx.getByte();
			let ix1 = this.ctx.index;
			decimals = this.readInt();
			//if(n < 0)
			//	UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed number decimal part.")
			dec_cnt = this.ctx.index - ix1;
			b = this.ctx.peekByte();
			if(b < 0)
				break;
		}
		if(b == 'e'.charCodeAt(0) || b == 'E'.charCodeAt(0)) {
			is_decimal = 1;
			this.ctx.getByte();
			let ix1 = this.ctx.index;
			exponent = this.readInt();
			if(ix1 == this.ctx.index)
				throw "Malformed number exponetional part."
			break;
		}
		break;
	}
	if(is_decimal) {
		for (let i = 0; i < dec_cnt; ++i)
			mantisa *= 10;
		mantisa += decimals;
		rpc_val.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Decimal;
		mantisa = is_neg? -mantisa: mantisa;
		rpc_val.value = {"mantisa": mantisa, "exponent":  exponent - dec_cnt}
	}
	else if(is_uint) {
		rpc_val.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.UInt;
		rpc_val.value = mantisa;

	}
	else {
		rpc_val.type = _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Int;
		rpc_val.value = is_neg? -mantisa: mantisa;
	}
}

function CponWriter()
{
	this.ctx = new _cpcontext__WEBPACK_IMPORTED_MODULE_0__["PackContext"]();
}

CponWriter.prototype.write = function(rpc_val)
{
	if(!(rpc_val && rpc_val.constructor.name === "RpcValue"))
		rpc_val = new _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"](rpc_val)
	if(rpc_val && rpc_val.constructor.name === "RpcValue") {
		if(rpc_val.meta) {
			this.writeMeta(rpc_val.meta);
		}
		switch (rpc_val.type) {
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Null: this.ctx.writeStringUtf8("null"); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Bool: this.writeBool(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.String: this.writeCString(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.UInt: this.writeUInt(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Int: this.writeInt(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Double: this.writeDouble(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Decimal: this.writeDecimal(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.List: this.writeList(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.Map: this.writeMap(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.IMap: this.writeIMap(rpc_val.value); break;
		case _rpcvalue__WEBPACK_IMPORTED_MODULE_1__["RpcValue"].Type.DateTime: this.writeDateTime(rpc_val.value); break;
		/*
		case RpcValue::Type::Invalid:
			if(WRITE_INVALID_AS_NULL) {
				write_p(nullptr);
			}
			break;
			*/
		}
	}
}
/*
CponWriter.prototype.writeStringUtf8 = function(str)
{
	for (let i=0; i < str.length; i++) {
		let charcode = str.charCodeAt(i);
		if (charcode < 0x80)
			this.ctx.putByte(charcode);
		else if (charcode < 0x800) {
			this.ctx.putByte(0xc0 | (charcode >> 6));
			this.ctx.putByte(0x80 | (charcode & 0x3f));
		}
		else if (charcode < 0xd800 || charcode >= 0xe000) {
			this.ctx.putByte(0xe0 | (charcode >> 12));
			this.ctx.putByte(0x80 | ((charcode>>6) & 0x3f));
			this.ctx.putByte(0x80 | (charcode & 0x3f));
		}
		// surrogate pair
		else {
			i++;
			charcode = ((charcode&0x3ff)<<10)|(str.charCodeAt(i)&0x3ff)
			this.ctx.putByte(0xf0 | (charcode >>18));
			this.ctx.putByte(0x80 | ((charcode>>12) & 0x3f));
			this.ctx.putByte(0x80 | ((charcode>>6) & 0x3f));
			this.ctx.putByte(0x80 | (charcode & 0x3f));
		}
	}
}
*/
CponWriter.prototype.writeCString = function(buffer)
{
	this.ctx.writeStringUtf8("\"");
	let data = new Uint8Array(buffer);
	for (let i=0; i < data.length; i++) {
		let b = data[i];
		switch(b) {
		case 0:
			this.ctx.writeStringUtf8("\\0");
			break;
		case '\\'.charCodeAt(0):
			this.ctx.writeStringUtf8("\\\\");
			break;
		case '\t'.charCodeAt(0):
			this.ctx.writeStringUtf8("\\t");
			break;
		case '\b'.charCodeAt(0):
			this.ctx.writeStringUtf8("\\b");
			break;
		case '\r'.charCodeAt(0):
			this.ctx.writeStringUtf8("\\r");
			break;
		case '\n'.charCodeAt(0):
			this.ctx.writeStringUtf8("\\n");
			break;
		case '"'.charCodeAt(0):
			this.ctx.writeStringUtf8("\\\"");
			break;
		default:
			this.ctx.putByte(b);
		}
	}
	this.ctx.writeStringUtf8("\"");
}

CponWriter.prototype.writeDateTime = function(dt)
{
	if(!dt) {
		this.ctx.writeStringUtf8('d""');
		return;
	}
	let epoch_msec = dt.epochMsec;
	let utc_offset = dt.utcOffsetMin;
	let msec = epoch_msec + 60000 * utc_offset;
	let s = new Date(msec).toISOString();
	let rtrim = (msec % 1000)? 1: 5;
	this.ctx.writeStringUtf8('d"');
	for (let i = 0; i < s.length-rtrim; i++)
		this.ctx.putByte(s.charCodeAt(i));
	if(!utc_offset) {
		this.ctx.writeStringUtf8('Z');
	}
	else {
		if(utc_offset < 0) {
			this.ctx.writeStringUtf8('-');
			utc_offset = -utc_offset;
		}
		else {
			this.ctx.writeStringUtf8('+');
		}
		s = ((utc_offset / 60) >> 0).toString().padStart(2, "0");
		if(utc_offset % 60)
			s += (utc_offset % 60).toString().padStart(2, "0");
		for (let i = 0; i < s.length; i++)
			this.ctx.putByte(s.charCodeAt(i));
	}
	this.ctx.writeStringUtf8('"');
}

CponWriter.prototype.writeBool = function(b)
{
	this.ctx.writeStringUtf8(b? "true": "false");
}

CponWriter.prototype.writeMeta = function(map)
{
	this.ctx.writeStringUtf8("<");
	this.writeMapContent(map);
	this.ctx.writeStringUtf8(">")
}

CponWriter.prototype.writeIMap = function(map)
{
	this.ctx.writeStringUtf8("i{");
	this.writeMapContent(map);
	this.ctx.writeStringUtf8("}")
}

CponWriter.prototype.writeMap = function(map)
{
	this.ctx.writeStringUtf8("{")
	this.writeMapContent(map);
	this.ctx.writeStringUtf8("}")
}

CponWriter.prototype.writeMapContent = function(map)
{
	let i = 0;
	for (let p in map) {
		if (map.hasOwnProperty(p)) {
			if(i++ > 0)
				this.ctx.putByte(",".charCodeAt(0))
			let c = p.charCodeAt(0);
			if(c >= 48 && c <= 57) {
				this.writeInt(parseInt(p))
			}
			else {
				this.ctx.putByte('"'.charCodeAt(0))
				this.ctx.writeStringUtf8(p);
				this.ctx.putByte('"'.charCodeAt(0))
			}
			this.ctx.writeStringUtf8(":")
			this.write(map[p]);
		}
	}
}

CponWriter.prototype.writeList = function(lst)
{
	this.ctx.putByte("[".charCodeAt(0))
	for(let i=0; i<lst.length; i++) {
		if(i > 0)
			this.ctx.putByte(",".charCodeAt(0))
		this.write(lst[i])
	}
	this.ctx.putByte("]".charCodeAt(0))
}

CponWriter.prototype.writeUInt = function(num)
{
	var s = num.toString();
	this.ctx.writeStringUtf8(s);
	this.ctx.putByte("u".charCodeAt(0))
}
CponWriter.prototype.writeInt = function(num)
{
	var s = num.toString();
	this.ctx.writeStringUtf8(s);
}
CponWriter.prototype.writeDouble = function(num)
{
	var s = num.toString();
	if(s.indexOf(".") < 0)
		s += "."
	this.ctx.writeStringUtf8(s);
}
CponWriter.prototype.writeDecimal = function(val)
{
	let mantisa = val.mantisa;
	let exponent = val.exponent;
	if(mantisa < 0) {
		mantisa = -mantisa;
		this.ctx.putByte("-".charCodeAt(0));
	}
	let str = mantisa.toString();
	let n = str.length;
	let dec_places = -exponent;
	if(dec_places > 0 && dec_places < n) {
		let dot_ix = n - dec_places;
		str = str.slice(0, dot_ix) + "." + str.slice(dot_ix)
	}
	else if(dec_places > 0 && dec_places <= 3) {
		let extra_0_cnt = dec_places - n;
		let str0 = "0.";
		for (let i = 0; i < extra_0_cnt; ++i)
			str0 += '0';
		str = str0 + str;
	}
	else if(dec_places < 0 && n + exponent <= 9) {
		for (let i = 0; i < exponent; ++i)
			str += '0';
		str += '.';
	}
	else if(dec_places == 0) {
		str += '.';
	}
	else {
		str += 'e' + exponent;
	}
	for (let i = 0; i < str.length; ++i) {
		this.ctx.putByte(str.charCodeAt(i));
	}
}




/***/ }),
/* 4 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "PackContext", function() { return PackContext; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "UnpackContext", function() { return UnpackContext; });


function UnpackContext(uint8_array)
{
	if(uint8_array.constructor.name === "ArrayBuffer")
		uint8_array = new Uint8Array(uint8_array)
	else if(uint8_array.constructor.name !== "Uint8Array")
		throw new TypeError("UnpackContext must be constructed with Uint8Array")
	this.data = uint8_array
	this.index = 0;
}

UnpackContext.prototype.getByte = function()
{
	if(this.index >= this.data.length)
		throw new RangeError("unexpected end of data")
	return this.data[this.index++]
}

UnpackContext.prototype.peekByte = function()
{
	if(this.index >= this.data.length)
		return -1
	return this.data[this.index]
}

UnpackContext.prototype.getBytes = function(str)
{
	for (var i = 0; i < str.length; i++) {
		if(str.charCodeAt(i) != this.getByte())
			throw new TypeError("'" + str + "'' expected");
	}
}

function PackContext()
{
	//this.buffer = new ArrayBuffer(PackContext.CHUNK_LEN)
	this.data = new Uint8Array(0)
	this.length = 0;
}

PackContext.CHUNK_LEN = 1024;

PackContext.transfer = function(source, length)
{
	if (!(source instanceof ArrayBuffer))
		throw new TypeError('Source must be an instance of ArrayBuffer');
	if (length <= source.byteLength)
		return source.slice(0, length);
	let source_view = new Uint8Array(source)
	let dest_view = new Uint8Array(new ArrayBuffer(length));
	dest_view.set(source_view);
	return dest_view.buffer;
}

PackContext.prototype.putByte = function(b)
{
	if(this.length >= this.data.length) {
		let buffer = PackContext.transfer(this.data.buffer, this.data.length + PackContext.CHUNK_LEN)
		this.data = new Uint8Array(buffer);
	}
	this.data[this.length++] = b;
}

PackContext.prototype.writeStringUtf8 = function(str)
{
	for (let i=0; i < str.length; i++) {
		let charcode = str.charCodeAt(i);
		if (charcode < 0x80)
			this.putByte(charcode);
		else if (charcode < 0x800) {
			this.putByte(0xc0 | (charcode >> 6));
			this.putByte(0x80 | (charcode & 0x3f));
		}
		else if (charcode < 0xd800 || charcode >= 0xe000) {
			this.putByte(0xe0 | (charcode >> 12));
			this.putByte(0x80 | ((charcode>>6) & 0x3f));
			this.putByte(0x80 | (charcode & 0x3f));
		}
		// surrogate pair
		else {
			i++;
			charcode = ((charcode&0x3ff)<<10)|(str.charCodeAt(i)&0x3ff)
			this.putByte(0xf0 | (charcode >>18));
			this.putByte(0x80 | ((charcode>>12) & 0x3f));
			this.putByte(0x80 | ((charcode>>6) & 0x3f));
			this.putByte(0x80 | (charcode & 0x3f));
		}
	}
}
/*
PackContext.prototype.bytes = function()
{
	return this.data.subarray(0, this.length)
}
*/
PackContext.prototype.buffer = function()
{
	return this.data.buffer.slice(0, this.length)
}




/***/ }),
/* 5 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ChainPack", function() { return ChainPack; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ChainPackReader", function() { return ChainPackReader; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ChainPackWriter", function() { return ChainPackWriter; });
/* harmony import */ var _cpcontext__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(4);
/* harmony import */ var _bint__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(6);





function ChainPack()
{
}

ChainPack.ProtocolType = 1;

ChainPack.CP_Null = 128;
ChainPack.CP_UInt = 129;
ChainPack.CP_Int = 130;
ChainPack.CP_Double = 131;
ChainPack.CP_Bool = 132;
//ChainPack.CP_Blob_depr; // deprecated
ChainPack.CP_String = 134;
//ChainPack.CP_DateTimeEpoch_depr; // deprecated
ChainPack.CP_List = 136;
ChainPack.CP_Map = 137;
ChainPack.CP_IMap = 138;
ChainPack.CP_MetaMap = 139;
ChainPack.CP_Decimal = 140;
ChainPack.CP_DateTime = 141;
ChainPack.CP_CString = 142;
ChainPack.CP_FALSE = 253;
ChainPack.CP_TRUE = 254;
ChainPack.CP_TERM = 255;

// UTC msec since 2.2. 2018
// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
ChainPack.SHV_EPOCH_MSEC = 1517529600000;
ChainPack.INVALID_MIN_OFFSET_FROM_UTC = (-64 * 15);

ChainPack.isLittleEndian = (function() {
	let buffer = new ArrayBuffer(2);
	new DataView(buffer).setInt16(0, 256, true /* littleEndian */);
	// Int16Array uses the platform's endianness.
	return new Int16Array(buffer)[0] === 256;
})();

function ChainPackReader(unpack_context)
{
	if(unpack_context.constructor.name === "ArrayBuffer")
		unpack_context = new UnpackContext(unpack_context)
	else if(unpack_context.constructor.name === "Uint8Array")
		unpack_context = new UnpackContext(unpack_context)
	if(unpack_context.constructor.name !== "UnpackContext")
		throw new TypeError("ChainpackReader must be constructed with UnpackContext")
	this.ctx = unpack_context;
}

ChainPackReader.prototype.read = function()
{
	let rpc_val = new RpcValue();
	let packing_schema = this.ctx.getByte();

	if(packing_schema == ChainPack.CP_MetaMap) {
		rpc_val.meta = this.readMap();
		packing_schema = this.ctx.getByte();
	}

	if(packing_schema < 128) {
		if(packing_schema & 64) {
			// tiny Int
			rpc_val.type = RpcValue.Type.Int;
			rpc_val.value = packing_schema & 63;
		}
		else {
			// tiny UInt
			rpc_val.type = RpcValue.Type.UInt;
			rpc_val.value = packing_schema & 63;
		}
	}
	else {
		switch(packing_schema) {
		case ChainPack.CP_Null: {
			rpc_val.type = RpcValue.Type.Null;
			rpc_val.value = null;
			break;
		}
		case ChainPack.CP_TRUE: {
			rpc_val.type = RpcValue.Type.Bool;
			rpc_val.value = true;
			break;
		}
		case ChainPack.CP_FALSE: {
			rpc_val.type = RpcValue.Type.Bool;
			rpc_val.value = false;
			break;
		}
		case ChainPack.CP_Int: {
			rpc_val.value = this.readIntData()
			rpc_val.type = RpcValue.Type.Int;
			break;
		}
		case ChainPack.CP_UInt: {
			rpc_val.value = this.readUIntData()
			rpc_val.type = RpcValue.Type.UInt;
			break;
		}
		case ChainPack.CP_Double: {
			let data = new Uint8Array(8);
			for (var i = 0; i < 8; i++)
				data[i] = this.ctx.getByte();
			rpc_val.value = new DataView(dat.buffer).getFloat64(0, true); //little endian
			rpc_val.type = RpcValue.Type.Double;
			break;
		}
		case ChainPack.CP_Decimal: {
			let mant = this.readIntData();
			let exp = this.readIntData();
			rpc_val.value = {mantisa: mant, exponent: exp};
			rpc_val.type = RpcValue.Type.Decimal;
			break;
		}
		case ChainPack.CP_DateTime: {
			let bi = this.readUIntDataHelper();
			let lsb = bi.val[bi.val.length - 1]
			let has_tz_offset = lsb & 1;
			let has_not_msec = lsb & 2;
			bi.signedRightShift(2);
			lsb = bi.val[bi.val.length - 1]

			let offset = 0;
			if(has_tz_offset) {
				offset = lsb & 0x7F;
				if(offset & 0x40) {
					// sign extension
					offset = offset - 128;
				}
				bi.signedRightShift(7);
			}
			offset *= 15;
			if(offset == ChainPack.INVALID_MIN_OFFSET_FROM_UTC) {
				rpc_val.value = null;
			}
			else {
				let msec = bi.toNumber();
				if(has_not_msec)
					msec *= 1000;
				msec += ChainPack.SHV_EPOCH_MSEC;

				rpc_val.value = {epochMsec: msec, utcOffsetMin: offset};
			}
			rpc_val.type = RpcValue.Type.DateTime;
			break;
		}
		case ChainPack.CP_Map: {
			rpc_val.value = this.readMap();
			rpc_val.type = RpcValue.Type.Map;
			break;
		}
		case ChainPack.CP_IMap: {
			rpc_val.value = this.readMap();
			rpc_val.type = RpcValue.Type.IMap;
			break;
		}
		case ChainPack.CP_List: {
			rpc_val.value = this.readList();
			rpc_val.type = RpcValue.Type.List;
			break;
		}
		case ChainPack.CP_String: {
			let str_len = this.readUIntData();
			let arr = new Uint8Array(str_len)
			for (var i = 0; i < str_len; i++)
				arr[i] = this.ctx.getByte()
			rpc_val.value = arr.buffer;
			rpc_val.type = RpcValue.Type.String;
			break;
		}
		case ChainPack.CP_CString:
		{
			// variation of CponReader.readCString()
			let pctx = new _cpcontext__WEBPACK_IMPORTED_MODULE_0__["PackContext"]();
			while(true) {
				let b = this.ctx.getByte();
				if(b == '\\'.charCodeAt(0)) {
					b = this.ctx.getByte();
					switch (b) {
					case '\\'.charCodeAt(0): pctx.putByte("\\"); break;
					case '0'.charCodeAt(0): pctx.putByte(0); break;
					default: pctx.putByte(b); break;
					}
				}
				else {
					if (b == 0) {
						// end of string
						break;
					}
					else {
						pctx.putByte(b);
					}
				}
			}
			rpc_val.value = pctx.buffer();
			rpc_val.type = RpcValue.Type.String;

			break;
		}
		default:
			throw new TypeError("ChainPack - Invalid type info: " + packing_schema);
		}
	}
	return rpc_val;
}

ChainPackReader.prototype.readUIntDataHelper = function()
{
	let num = 0;
	let head = this.ctx.getByte();
	let bytes_to_read_cnt;
	if     ((head & 128) === 0) {bytes_to_read_cnt = 0; num = head & 127;}
	else if((head &  64) === 0) {bytes_to_read_cnt = 1; num = head & 63;}
	else if((head &  32) === 0) {bytes_to_read_cnt = 2; num = head & 31;}
	else if((head &  16) === 0) {bytes_to_read_cnt = 3; num = head & 15;}
	else {
		bytes_to_read_cnt = (head & 0xf) + 4;
	}
	let bytes = new Uint8Array(bytes_to_read_cnt + 1)
	bytes[0] = num;
	for (let i=0; i < bytes_to_read_cnt; i++) {
		let r = this.ctx.getByte();
		bytes[i + 1] = r;
	}
	return new _bint__WEBPACK_IMPORTED_MODULE_1__["BInt"](bytes)
}

ChainPackReader.prototype.readUIntData = function()
{
	let bi = this.readUIntDataHelper();
	return bi.toNumber();
}

ChainPackReader.prototype.readIntData = function()
{
	let bi = this.readUIntDataHelper();
	let is_neg;
	if(bi.byteCount() < 5) {
		let sign_mask = 0x80 >> bi.byteCount();
		is_neg = bi.val[0] & sign_mask;
		bi.val[0] &= ~sign_mask;
	}
	else {
		is_neg = bi.val[1] & 128;
		bi.val[1] &= ~128;
	}
	let num = bi.toNumber();
	if(is_neg)
		num = -num;
	return num;
}

ChainPackReader.prototype.readList = function()
{
	let lst = []
	while(true) {
		let b = this.ctx.peekByte();
		if(b == ChainPack.CP_TERM) {
			this.ctx.getByte();
			break;
		}
		let item = this.read()
		lst.push(item);
	}
	return lst;
}

ChainPackReader.prototype.readMap = function()
{
	let map = {}
	while(true) {
		let b = this.ctx.peekByte();
		if(b == ChainPack.CP_TERM) {
			this.ctx.getByte();
			break;
		}
		let key = this.read()
		if(!key.isValid())
			throw new TypeError("Malformed map, invalid key");
		let val = this.read()
		if(key.type === RpcValue.Type.String)
			map[key.toString().slice(1, -1)] = val;
		else
			map[key.toInt()] = val;
	}
	return map;
}

function ChainPackWriter()
{
	this.ctx = new _cpcontext__WEBPACK_IMPORTED_MODULE_0__["PackContext"]();
}

ChainPackWriter.prototype.write = function(rpc_val)
{
	if(!(rpc_val && rpc_val.constructor.name === "RpcValue"))
		rpc_val = new RpcValue(rpc_val)
	if(rpc_val && rpc_val.constructor.name === "RpcValue") {
		if(rpc_val.meta) {
			this.writeMeta(rpc_val.meta);
		}
		switch (rpc_val.type) {
		case RpcValue.Type.Null: this.ctx.putByte(ChainPack.CP_Null); break;
		case RpcValue.Type.Bool: this.ctx.putByte(rpc_val.value? ChainPack.CP_TRUE: ChainPack.CP_FALSE); break;
		case RpcValue.Type.String: this.writeString(rpc_val.value); break;
		case RpcValue.Type.UInt: this.writeUInt(rpc_val.value); break;
		case RpcValue.Type.Int: this.writeInt(rpc_val.value); break;
		case RpcValue.Type.Double: this.writeDouble(rpc_val.value); break;
		case RpcValue.Type.Decimal: this.writeDecimal(rpc_val.value); break;
		case RpcValue.Type.List: this.writeList(rpc_val.value); break;
		case RpcValue.Type.Map: this.writeMap(rpc_val.value); break;
		case RpcValue.Type.IMap: this.writeIMap(rpc_val.value); break;
		case RpcValue.Type.DateTime: this.writeDateTime(rpc_val.value); break;
		default:
			// better to write null than create invalid chain-pack
			this.ctx.putByte(ChainPack.CP_Null);
			break;
		}
	}
}

//ChainPackWriter.MAX_BIT_LEN = Math.log(Number.MAX_SAFE_INTEGER) / Math.log(2);
// logcal operator in JS works on 32 bit only
ChainPackWriter.MAX_BIT_LEN = 32;
	/*
	 0 ...  7 bits  1  byte  |0|s|x|x|x|x|x|x|<-- LSB
	 8 ... 14 bits  2  bytes |1|0|s|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
	15 ... 21 bits  3  bytes |1|1|0|s|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
	22 ... 28 bits  4  bytes |1|1|1|0|s|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
	29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |s|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
											n ==  0 ->  4 bytes number (32 bit number)
											n ==  1 ->  5 bytes number
											n == 14 -> 18 bytes number
											n == 15 -> for future (number of bytes will be specified in next byte)
	*/

	// return max bit length >= bit_len, which can be encoded by same number of bytes

// number of bytes needed to encode bit_len
ChainPackWriter.bytesNeeded = function(bit_len)
{
	let cnt;
	if(bit_len <= 28)
		cnt = ((bit_len - 1) / 7 | 0) + 1;
	else
		cnt = ((bit_len - 1) / 8 | 0) + 2;
	return cnt;
}

ChainPackWriter.expandBitLen = function(bit_len)
{
	let ret;
	let byte_cnt = ChainPackWriter.bytesNeeded(bit_len);
	if(bit_len <= 28) {
		ret = byte_cnt * (8 - 1) - 1;
	}
	else {
		ret = (byte_cnt - 1) * 8 - 1;
	}
	return ret;
}

ChainPackWriter.prototype.writeUIntDataHelper = function(bint)
{
	let bytes = bint.val;
	//let byte_cnt = bint.byteCount();

	let head = bytes[0];
	if(bytes.length < 5) {
		let mask = (0xf0 << (4 - bytes.length)) & 0xff;
		head = head & ~mask;
		mask <<= 1;
		mask &= 0xff;
		head = head | mask;
	}
	else {
		head = 0xf0 | (bytes.length - 5);
	}
	this.ctx.putByte(head);
	for (let i = 1; i < bytes.length; ++i) {
		let r = bytes[i];
		this.ctx.putByte(r);
	}
}

ChainPackWriter.prototype.writeUIntData = function(num)
{
	let bi = new _bint__WEBPACK_IMPORTED_MODULE_1__["BInt"](num)
	let bitcnt = bi.significantBitsCount();
	bi.resize(ChainPackWriter.bytesNeeded(bitcnt));
	this.writeUIntDataHelper(bi);
}

ChainPackWriter.prototype.writeIntData = function(snum)
{
	let neg = (snum < 0);
	let num = neg? -snum: snum;
	let bi = new _bint__WEBPACK_IMPORTED_MODULE_1__["BInt"](num)
	let bitcnt = bi.significantBitsCount() + 1;
	bi.resize(ChainPackWriter.bytesNeeded(bitcnt));
	if(neg) {
		if(bi.byteCount() < 5) {
			let sign_mask = 0x80 >> bi.byteCount();
			bi.val[0] |= sign_mask;
		}
		else {
			bi.val[1] |= 128;
		}
	}
	this.writeUIntDataHelper(bi);
}

ChainPackWriter.prototype.writeUInt = function(n)
{
	if(n < 64) {
		this.ctx.putByte(n % 64);
	}
	else {
		this.ctx.putByte(ChainPack.CP_UInt);
		this.writeUIntData(n);
	}
}

ChainPackWriter.prototype.writeInt = function(n)
{
	if(n >= 0 && n < 64) {
		this.ctx.putByte((n % 64) + 64);
	}
	else {
		this.ctx.putByte(ChainPack.CP_Int);
		this.writeIntData(n);
	}
}

ChainPackWriter.prototype.writeDecimal = function(val)
{
	this.ctx.putByte(ChainPack.CP_Decimal);
	this.writeIntData(val.mantisa);
	this.writeIntData(val.exponent);
}

ChainPackWriter.prototype.writeList = function(lst)
{
	this.ctx.putByte(ChainPack.CP_List);
	for(let i=0; i<lst.length; i++)
		this.write(lst[i])
	this.ctx.putByte(ChainPack.CP_TERM);
}

ChainPackWriter.prototype.writeMapData = function(map)
{
	for (let p in map) {
		if (map.hasOwnProperty(p)) {
			let c = p.charCodeAt(0);
			if(c >= 48 && c <= 57) {
				this.writeInt(parseInt(p))
			}
			else {
				this.writeJSString(p);
			}
			this.write(map[p]);
		}
	}
	this.ctx.putByte(ChainPack.CP_TERM);
}

ChainPackWriter.prototype.writeMap = function(map)
{
	this.ctx.putByte(ChainPack.CP_Map);
	this.writeMapData(map);
}

ChainPackWriter.prototype.writeIMap = function(map)
{
	this.ctx.putByte(ChainPack.CP_IMap);
	this.writeMapData(map);
}

ChainPackWriter.prototype.writeMeta = function(map)
{
	this.ctx.putByte(ChainPack.CP_MetaMap);
	this.writeMapData(map);
}

ChainPackWriter.prototype.writeString = function(str)
{
	this.ctx.putByte(ChainPack.CP_String);
	let arr = new Uint8Array(str)
	this.writeUIntData(arr.length)
	for (let i=0; i < arr.length; i++)
		this.ctx.putByte(arr[i])
}

ChainPackWriter.prototype.writeJSString = function(str)
{
	this.ctx.putByte(ChainPack.CP_String);
	let pctx = new _cpcontext__WEBPACK_IMPORTED_MODULE_0__["PackContext"]();
	pctx.writeStringUtf8(str);
	this.writeUIntData(pctx.length)
	for (let i=0; i < pctx.length; i++)
		this.ctx.putByte(pctx.data[i])
}

ChainPackWriter.prototype.writeDateTime = function(dt)
{
	if(!dt || typeof(dt) !== "object" || dt.utcOffsetMin == ChainPack.INVALID_MIN_OFFSET_FROM_UTC) {
		// invalid datetime
		dt = {epochMsec: ChainPack.SHV_EPOCH_MSEC, utcOffsetMin: ChainPack.INVALID_MIN_OFFSET_FROM_UTC}
	}

	this.ctx.putByte(ChainPack.CP_DateTime);

	let msecs = dt.epochMsec;
	msecs = msecs - ChainPack.SHV_EPOCH_MSEC;
	if(msecs < 0)
		throw new RangeError("DateTime prior to 2018-02-02 are not supported in current ChainPack implementation.");

	let offset = (dt.utcOffsetMin / 15) & 0x7F;

	let ms = msecs % 1000;
	if(ms == 0)
		msecs /= 1000;
	let bi = new _bint__WEBPACK_IMPORTED_MODULE_1__["BInt"](msecs);
	if(offset != 0) {
		bi.leftShift(7);
		bi.val[bi.val.length - 1] |= offset;
	}
	bi.leftShift(2);
	if(offset != 0)
		bi.val[bi.val.length - 1] |= 1;
	if(ms == 0)
		bi.val[bi.val.length - 1] |= 2;

	// save as signed int
	let bitcnt = bi.significantBitsCount() + 1;
	bi.resize(ChainPackWriter.bytesNeeded(bitcnt));
	this.writeUIntDataHelper(bi);
}


/***/ }),
/* 6 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "BInt", function() { return BInt; });


function BInt(n)
{
	if(Number.isInteger(n)) {
		this.val = BInt.parseInt(n);
	}
	else if(n.constructor.name === "Uint8Array") {
		this.val = n;
	}
	else {
		throw TypeError(n + " is not convertible to BInt");
	}
}

BInt.divInt = function(n, d)
{
	let r = n % d;
	if(!Number.isInteger(r))
		throw new RangeError("Number too big for current implementation of DIV function: " + n + " DIV " + d)
	return [(n - r) / d, r];
}

BInt.parseInt = function(num)
{
	let bytes = new Uint8Array(8);
	let len = 0;
	while(true) {
		[num, bytes[len++]] = BInt.divInt(num, 256)
		if(num == 0)
			break;
	}
	bytes = bytes.subarray(0, len)
	bytes.reverse();
	return bytes
}

BInt.prototype.byteCount = function()
{
	if(this.val)
		return this.val.length;
	return 0;
}

BInt.prototype.resize = function(byte_cnt)
{
	if(this.val.constructor.name !== "Uint8Array")
		throw TypeError(n + " cannot be resized");
	if(byte_cnt < this.val.length) {
		this.val = this.val.subarray(this.val.length - byte_cnt)
	}
	else if(byte_cnt > this.val.length) {
		let nbytes = new Uint8Array(byte_cnt)
		nbytes.set(this.val, byte_cnt - this.val.length)
		this.val = nbytes
	}
}

BInt.prototype.resizeSigned = function(byte_cnt)
{
	let old_len = this.val.length;
	this.resize(byte_cnt);
	if(byte_cnt > old_len) {
		if(this.val[byte_cnt - old_len] & 128) {
			// extend sign
			for(let i = 0; i < byte_cnt - old_len; i++)
				this.val[i] = 0xff;
		}
	}
}

BInt.prototype.significantBitsCount = function()
{
	let n = this.val[0];
	const mask = 128;
	let len = 8;
	for (; n && !(n & mask); --len) {
		n <<= 1;
	}
	let cnt = n? len: 0;
	cnt += 8 * (this.val.length - 1)
	return cnt;
}

BInt.prototype.leftShift = function(cnt)
{
	let nbytes = new Uint8Array(this.val.length)
	nbytes.set(this.val)
	let is_neg = nbytes[0] & 128;

	for(let j=0; j<cnt; j++) {
		let cy = 0;
		for(let i=nbytes.length - 1; i >= 0; i--) {
			let cy1 = nbytes[i] & 128;
			nbytes[i] <<= 1;
			if(cy)
				nbytes[i] |= 1
			cy = cy1
		}
		if(cy) {
			// prepend byte
			let nbytes2 = new Uint8Array(nbytes.length + 1)
			nbytes2.set(nbytes, 1);
			nbytes = nbytes2
			nbytes[0] = 1
		}
	}
	if(is_neg) for(let i=0; i<cnt; i++) {
		let mask = 128;
		for(let j = 0; j < 8; j++) {
			if(nbytes[i] & mask) {
				this.val = nbytes;
				return;
			}
			nbytes[i] |= mask;
			mask >>= 1;
		}
	}
	this.val = nbytes;
}

BInt.prototype.signedRightShift = function(cnt)
{
	let bytes = this.val;
	for(let j=0; j<cnt; j++) {
		let cy = 0;
		for(let i=0; i < bytes.length; i++) {
			let cy1 = bytes[i] & 1;
			if(i == 0) {
				bytes[i] >>>= 1;
			}
			else {
				bytes[i] >>= 1;
				if(cy)
					bytes[i] |= 128
			}
			cy = cy1
		}
	}
}

BInt.prototype.toNumber = function()
{
	let num = 0;
	for (let i=0; i < this.val.length; i++) {
		num = (num * 256) + this.val[i];
	}
	return num;
}


/***/ }),
/* 7 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Test", function() { return Test; });
/* harmony import */ var _rpcvalue__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(2);




class Test
{
	checkEq(e1, e2, msg)
	{
		//console.log((e1 === e2)? "OK": "ERROR", ":", e1, "vs.", e2)
		if(e1 === e2)
			return;
		if(msg)
			throw msg;
		else
			throw "test check error: " + e1 + " === " + e2
	}

	testConversions()
	{
		for(const lst of [
			[(2**31 - 1) + "u", null],
			//[(2**32 - 1) + "u", null],  // too big for JS bitwise operations
			["" + (2**31 - 1), null],
			["" + (-(2**30 - 1)), null],
			["" + (2**53 - 1), null], // Number.MAX_SAFE_INTEGER
			["" + (-(2**53 - 1)), null], // Number.MIN_SAFE_INTEGER
			//["" + (2**32 - 1), null], // too big for JS bitwise operations
			["true", null],
			["false", null],
			["null", null],
			["1u", null],
			["134", null],
			["7", null],
			["-2", null],
			["0xab", "171"],
			["-0xCD", "-205"],
			["0x1a2b3c4d", "439041101"],
			["223.", null],
			["2.30", null],
			["12.3e-10", "123e-11"],
			["-0.00012", "-12e-5"],
			["-1234567890.", "-1234567890."],
			["\"foo\"", null],
			["[]", null],
			["[1]", null],
			["[1,]", "[1]"],
			["[1,2,3]", null],
			["[[]]", null],
			["{\"foo\":\"bar\"}", null],
			["i{1:2}", null],
			["i{\n\t1: \"bar\",\n\t345u : \"foo\",\n}", "i{1:\"bar\",345:\"foo\"}"],
			["[1u,{\"a\":1},2.30]", null],
			["<1:2>3", null],
			["[1,<7:8>9]", null],
			["<>1", null],
			["<8:3u>i{2:[[\".broker\",<1:2>true]]}", null],
			["<1:2,\"foo\":\"bar\">i{1:<7:8>9}", null],
			["<1:2,\"foo\":<5:6>\"bar\">[1u,{\"a\":1},2.30]", null],
			["i{1:2 // comment to end of line\n}", "i{1:2}"],
			[`/*comment 1*/{ /*comment 2*/
			\t\"foo\"/*comment \"3\"*/: \"bar\", //comment to end of line
			\t\"baz\" : 1,
			/*
			\tmultiline comment
			\t\"baz\" : 1,
			\t\"baz\" : 1, // single inside multi
			*/
			}`, "{\"foo\":\"bar\",\"baz\":1}"],
			//["a[1,2,3]", "[1,2,3]"], // unsupported array type
			["<1:2>[3,<4:5>6]", null],
			["<4:\"svete\">i{2:<4:\"svete\">[0,1]}", null],
			['d"2019-05-03T11:30:00-0700"', 'd"2019-05-03T11:30:00-07"'],
			['d""', null],
			['d"2018-02-02T00:00:00Z"', null],
			['d"2027-05-03T11:30:12.345+01"', null],
			])
		{
			let cpon1 = lst[0]
			let cpon2 = lst[1]? lst[1]: cpon1;

			let rv1 = _rpcvalue__WEBPACK_IMPORTED_MODULE_0__["RpcValue"].fromCpon(cpon1);
			let cpn1 = rv1.toString();
			log(cpon1, "\t--cpon------>\t", cpn1)
			this.checkEq(cpn1, cpon2);

			let cpk1 = rv1.toChainPack();
			let rv2 = _rpcvalue__WEBPACK_IMPORTED_MODULE_0__["RpcValue"].fromChainPack(cpk1);
			let cpn2 = rv2.toString();
			log(cpn1, "\t--chainpack->\t", cpn2, "\n")
			this.checkEq(cpn1, cpn2);
		}
	}

	testDateTime()
	{
		// same points in time
		let v1 = _rpcvalue__WEBPACK_IMPORTED_MODULE_0__["RpcValue"].fromCpon('d"2017-05-03T18:30:00Z"');
		let v2 = _rpcvalue__WEBPACK_IMPORTED_MODULE_0__["RpcValue"].fromCpon('d"2017-05-03T22:30:00+04"');
		let v3 = _rpcvalue__WEBPACK_IMPORTED_MODULE_0__["RpcValue"].fromCpon('d"2017-05-03T11:30:00-0700"');
		let v4 = _rpcvalue__WEBPACK_IMPORTED_MODULE_0__["RpcValue"].fromCpon('d"2017-05-03T15:00:00-0330"');
		this.checkEq(v1.value.epochMsec, v2.value.epochMsec);
		this.checkEq(v2.value.epochMsec, v3.value.epochMsec);
		this.checkEq(v3.value.epochMsec, v4.value.epochMsec);
		this.checkEq(v4.value.epochMsec, v1.value.epochMsec);
	}

	static run()
	{
		//try {
			/*
			for(let i=0; i<7; i++) {
				log("---------", i, '---------------')
				for(const n of [1,255,256,65535, 65536, -1, -255, -65535, -65536]) {
					let bytes1 = ChainPack.uIntToBBE(n)
					let bytes2 = ChainPack.rotateLeftBBE(bytes1, i)
					log(n, "<<", i, '\t', bytes1, "->", bytes2)
				}
			}
			return
			*/
			let t = new Test();

			t.testConversions();
			t.testDateTime();

			log("PASSED")
		//}
		//catch(err) {
		//	log("FAILED:", err)
		//}
	}
}


/***/ })
/******/ ]);
//# sourceMappingURL=main.js.map