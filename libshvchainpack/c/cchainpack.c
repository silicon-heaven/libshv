#include <shv/chainpack/cchainpack.h>

#include <stdlib.h>
#include <string.h>
#include <limits.h>

// UTC msec since 2.2. 2018 folowed by signed UTC offset in 1/4 hour
// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
static const int64_t SHV_EPOCH_MSEC = 1517529600000;

const char* cchainpack_packing_schema_name(int sch)
{
	switch (sch) {
	case CP_INVALID: return "INVALID";
	case CP_FALSE: return "FALSE";
	case CP_TRUE: return "TRUE";

	case CP_Null: return "Null";
	case CP_UInt: return "UInt";
	case CP_Int: return "Int";
	case CP_Double: return "Double";
	case CP_Bool: return "Bool";
	case CP_Blob: return "Blob";
	case CP_String: return "String";
	case CP_CString: return "CString";
	case CP_List: return "List";
	case CP_Map: return "Map";
	case CP_IMap: return "IMap";
	case CP_DateTimeEpoch_depr: return "DateTimeEpoch_depr";
	case CP_DateTime: return "DateTime";
	case CP_MetaMap: return "MetaMap";
	case CP_Decimal: return "Decimal";

	case CP_TERM: return "TERM";
	}
	return "";
}

static void copy_bytes_cstring(ccpcp_pack_context *pack_context, const void *str, size_t len)
{
	size_t i;
	for (i = 0; i < len; ++i) {
		if(pack_context->err_no != CCPCP_RC_OK)
			return;
		uint8_t ch = ((const uint8_t*)str)[i];
		switch(ch) {
		case '\0':
		case '\\':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, ch);
			break;
		default:
			ccpcp_pack_copy_byte(pack_context, ch);
		}
	}
}

#if defined(__GNUC__) && __GNUC__ >= 4

static int significant_bits_part_length(uint64_t n)
{
	int len = 0;
	int llbits = sizeof(long long) * CHAR_BIT;

	if (n == 0)
		return 0;

	if ((llbits < 64) && (n & 0xFFFFFFFF00000000)) {
		len += 32;
		n >>= 32;
	}

	len += llbits - __builtin_clzll(n);

	return len;
}

#else /* Fallback for generic compiler */

// see https://en.wikipedia.org/wiki/Find_first_set#CLZ
static const uint8_t sig_table_4bit[16] =  { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

static int significant_bits_part_length(uint64_t n)
{
	int len = 0;

	if (n & 0xFFFFFFFF00000000) {
		len += 32;
		n >>= 32;
	}
	if (n & 0xFFFF0000) {
		len += 16;
		n >>= 16;
	}
	if (n & 0xFF00) {
		len += 8;
		n >>= 8;
	}
	if (n & 0xF0) {
		len += 4;
		n >>= 4;
	}
	len += sig_table_4bit[n];
	return len;
}

#endif /* end of significant_bits_part_length function */

// number of bytes needed to encode bit_len
static int bytes_needed(int bit_len)
{
	int cnt;
	if(bit_len <= 28)
		cnt = (bit_len - 1) / 7 + 1;
	else
		cnt = (bit_len - 1) / 8 + 2;
	return cnt;
}

/* UInt
 0 ...  7 bits  1  byte  |0|x|x|x|x|x|x|x|<-- LSB
 8 ... 14 bits  2  bytes |1|0|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
15 ... 21 bits  3  bytes |1|1|0|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
22 ... 28 bits  4  bytes |1|1|1|0|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
                    n ==  0 ->  4 bytes number (32 bit number)
                    n ==  1 ->  5 bytes number
                    n == 14 -> 18 bytes number
                    n == 15 -> for future (number of bytes will be specified in next byte)
*/

static void pack_uint_data_helper(ccpcp_pack_context* pack_context, uint64_t num, int bit_len)
{
	int byte_cnt = bytes_needed(bit_len);
	// 32 bytes should be enough for any number. Can't use a VLA for `bytes`, because MSVC doesn't support it.
	if (byte_cnt > 32) {
		abort();
	}
	uint8_t bytes[32];
	int i;
	for (i = byte_cnt-1; i >= 0; --i) {
		uint8_t r = num & 255;
		bytes[i] = r;
		num = num >> 8;
	}

	uint8_t *head = bytes;
	if(bit_len <= 28) {
		uint8_t mask = (uint8_t)(0xf0 << (4 - byte_cnt));
		*head = (uint8_t)(*head & ~mask);
		mask = (uint8_t)(mask << 1);
		*head = *head | mask;
	}
	else {
		*head = (uint8_t)(0xf0 | (byte_cnt - 5));
	}

	for ( i = 0; i < byte_cnt; ++i) {
		uint8_t r = bytes[i];
		ccpcp_pack_copy_byte(pack_context, r);
	}
}

void cchainpack_pack_uint_data(ccpcp_pack_context* pack_context, uint64_t num)
{
	const size_t UINT_BYTES_MAX = 18;
	if(sizeof(num) > UINT_BYTES_MAX) {
		pack_context->err_no = CCPCP_RC_LOGICAL_ERROR;//("writeData_UInt: value too big to pack!");
		return;
	}

	int bitlen = significant_bits_part_length(num);
	pack_uint_data_helper(pack_context, num, bitlen);
}

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
static int expand_bit_len(int bit_len)
{
	int ret;
	int byte_cnt = bytes_needed(bit_len);
	if(bit_len <= 28) {
		ret = byte_cnt * (8 - 1) - 1;
	}
	else {
		ret = (byte_cnt - 1) * 8 - 1;
	}
	return ret;
}

static void cchainpack_pack_int_data(ccpcp_pack_context* pack_context, int64_t snum)
{
	uint64_t num = (uint64_t)(snum < 0? -snum: snum);
	bool neg = (snum < 0);

	int bitlen = significant_bits_part_length(num);
	bitlen++; // add sign bit
	if(neg) {
		int sign_pos = expand_bit_len(bitlen);
		uint64_t sign_bit_mask = (uint64_t)1 << sign_pos;
		num |= sign_bit_mask;
	}
	pack_uint_data_helper(pack_context, num, bitlen);
}

void cchainpack_pack_uint(ccpcp_pack_context* pack_context, uint64_t i)
{
	if (pack_context->err_no)
		return;
	if(i < 64) {
		ccpcp_pack_copy_byte(pack_context, i % 64);
	}
	else {
		ccpcp_pack_copy_byte(pack_context, CP_UInt);
		cchainpack_pack_uint_data(pack_context, i);
	}
}

void cchainpack_pack_int(ccpcp_pack_context* pack_context, int64_t i)
{
	if (pack_context->err_no)
		return;
	if(i >= 0 && i < 64) {
		ccpcp_pack_copy_byte(pack_context, (uint8_t)((i % 64) + 64));
	}
	else {
		ccpcp_pack_copy_byte(pack_context, CP_Int);
		cchainpack_pack_int_data(pack_context, i);
	}
}

void cchainpack_pack_decimal(ccpcp_pack_context *pack_context, int64_t i, int exponent)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_Decimal);
	cchainpack_pack_int_data(pack_context, i);
	cchainpack_pack_int_data(pack_context, exponent);
}

void cchainpack_pack_double(ccpcp_pack_context* pack_context, double d)
{
	if (pack_context->err_no)
		return;

	ccpcp_pack_copy_byte(pack_context, CP_Double);

	const uint8_t*bytes = (const uint8_t*)&d;
	int len = sizeof(double);

	int n = 1;
	int i;
	if(*(char *)&n == 1) {
		// little endian if true
		for (i=0; i<len; i++)
			ccpcp_pack_copy_byte(pack_context, bytes[i]);
	}
	else {
		for (i=len-1; i>=0; i--)
			ccpcp_pack_copy_byte(pack_context, bytes[i]);
	}
}

void cchainpack_pack_date_time(ccpcp_pack_context *pack_context, int64_t epoch_msecs, int min_from_utc)
{
	if (pack_context->err_no)
		return;

	ccpcp_pack_copy_byte(pack_context, CP_DateTime);

	// some arguable optimizations when msec == 0 or TZ_offset == 0
	// this can save byte in packed date-time, but packing scheme is more complicated
	int64_t msecs = epoch_msecs - SHV_EPOCH_MSEC;
	int offset = (min_from_utc / 15) & 0x7F;
	int ms = (int)(msecs % 1000);
	if(ms == 0)
		msecs /= 1000;
	if(offset != 0) {
		msecs *= 128;
		msecs |= offset;
	}
	msecs *= 4;
	if(offset != 0)
		msecs |= 1;
	if(ms == 0)
		msecs |= 2;
	cchainpack_pack_int_data(pack_context, msecs);
}

void cchainpack_pack_null(ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;

	ccpcp_pack_copy_byte(pack_context, CP_Null);
}

static void cchainpack_pack_true (ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_TRUE);
}

static void cchainpack_pack_false (ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_FALSE);
}

void cchainpack_pack_boolean(ccpcp_pack_context* pack_context, bool b)
{
	if (pack_context->err_no)
		return;
	if(b)
		cchainpack_pack_true(pack_context);
	else
		cchainpack_pack_false(pack_context);
}

void cchainpack_pack_list_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_List);
}

void cchainpack_pack_map_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_Map);
}

void cchainpack_pack_imap_begin(ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_IMap);
}

void cchainpack_pack_meta_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_MetaMap);
}

void cchainpack_pack_container_end(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_byte(pack_context, CP_TERM);
}

void cchainpack_pack_blob (ccpcp_pack_context* pack_context, const uint8_t* buff, size_t buff_len)
{
	cchainpack_pack_blob_start(pack_context, buff_len, buff, buff_len);
}

void cchainpack_pack_blob_start (ccpcp_pack_context* pack_context, size_t string_len, const uint8_t* buff, size_t buff_len)
{
	ccpcp_pack_copy_byte(pack_context, CP_Blob);
	cchainpack_pack_uint_data(pack_context, string_len);
	ccpcp_pack_copy_bytes(pack_context, buff, buff_len);
}

void cchainpack_pack_blob_cont (ccpcp_pack_context* pack_context, const uint8_t* buff, size_t buff_len)
{
	ccpcp_pack_copy_bytes(pack_context, buff, buff_len);
}

void cchainpack_pack_string (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len)
{
	cchainpack_pack_string_start(pack_context, buff_len, buff, buff_len);
}

void cchainpack_pack_string_start (ccpcp_pack_context* pack_context, size_t string_len, const char* buff, size_t buff_len)
{
	ccpcp_pack_copy_byte(pack_context, CP_String);
	cchainpack_pack_uint_data(pack_context, string_len);
	ccpcp_pack_copy_bytes(pack_context, buff, buff_len);
}

void cchainpack_pack_string_cont (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len)
{
	ccpcp_pack_copy_bytes(pack_context, buff, buff_len);
}

void cchainpack_pack_cstring (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len)
{
	cchainpack_pack_cstring_start(pack_context, buff, buff_len);
	cchainpack_pack_cstring_finish(pack_context);
}

void cchainpack_pack_cstring_terminated (ccpcp_pack_context* pack_context, const char* str)
{
	size_t len = strlen(str);
	cchainpack_pack_cstring(pack_context, str, len);
}

void cchainpack_pack_cstring_start (ccpcp_pack_context* pack_context, const char*buff, size_t buff_len)
{
	ccpcp_pack_copy_byte(pack_context, CP_CString);
	copy_bytes_cstring(pack_context, buff, buff_len);
}

void cchainpack_pack_cstring_cont (ccpcp_pack_context* pack_context, const char*buff, size_t buff_len)
{
	copy_bytes_cstring(pack_context, buff, buff_len);
}

void cchainpack_pack_cstring_finish (ccpcp_pack_context* pack_context)
{
	ccpcp_pack_copy_byte(pack_context, '\0');
}

//============================   U N P A C K   =================================

/// @pbitlen is used to enable same function usage for signed int unpacking
static void unpack_uint(ccpcp_unpack_context* unpack_context, int *pbitlen)
{
	if(pbitlen)
		*pbitlen = 0;
	uint64_t num = 0;
	int bitlen = 0;

	const char *p;
	UNPACK_TAKE_BYTE(p);
	uint8_t head = (uint8_t)(*p);

	int bytes_to_read_cnt;
	if     ((head & 128) == 0) {bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;}
	else if((head &  64) == 0) {bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;}
	else if((head &  32) == 0) {bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2*8;}
	else if((head &  16) == 0) {bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3*8;}
	else {
		bytes_to_read_cnt = (head & 0xf) + 4;
		bitlen = bytes_to_read_cnt * 8;
	}
	int i;
	for (i = 0; i < bytes_to_read_cnt; ++i) {
		UNPACK_TAKE_BYTE(p);
		uint8_t r = (uint8_t)(*p);
		num = (num << 8) + r;
	};
	if (bitlen > 64) {
		// numbers greater than 64 bits will be returned as UINT MAX
		unpack_context->item.as.UInt = UINT64_MAX;
		unpack_context->item.type = CCPCP_ITEM_UINT;
	}
	else {
		unpack_context->item.as.UInt = num;
		unpack_context->item.type = CCPCP_ITEM_UINT;
	}
	if(pbitlen)
		*pbitlen = bitlen;
}

static void unpack_int(ccpcp_unpack_context* unpack_context)
{
	int64_t snum = 0;
	int bitlen;
	unpack_uint(unpack_context, &bitlen);

	if(unpack_context->err_no == CCPCP_RC_OK) {
		unpack_context->item.type = CCPCP_ITEM_INT;
		if (bitlen > 64) {
			// numbers greater than 64 bits (with sign) will be returned as INT MAX
			unpack_context->item.as.Int = INT64_MAX;
		}
		else {
			const uint64_t sign_bit_mask = (uint64_t)1 << (bitlen - 1);
			uint64_t num = unpack_context->item.as.UInt;
			snum = (int64_t)num;
			// Note: masked value assignment to bool variable would be undefined on some platforms.
			if(num & sign_bit_mask) {
				snum &= ~(int64_t)sign_bit_mask;
				snum = -snum;
			}
			unpack_context->item.as.Int = snum;
		}
	}
}

void unpack_string(ccpcp_unpack_context* unpack_context)
{
	const char *p;
	ccpcp_string *it = &unpack_context->item.as.String;

	bool is_cstr = it->string_size < 0;
	if(is_cstr) {
		for(it->chunk_size = 0; it->chunk_size < it->chunk_buff_len; ) {
			UNPACK_TAKE_BYTE(p);
			if(*p == '\\') {
				UNPACK_TAKE_BYTE(p);
				if(!p)
					return;
				switch (*p) {
				case '\\': (it->chunk_start)[it->chunk_size++] = '\\'; break;
				case '0': (it->chunk_start)[it->chunk_size++] = '\0'; break;
				default: (it->chunk_start)[it->chunk_size++] = *p; break;
				}
				break;
			}
			if (*p == '\0') {
				// end of string
				it->last_chunk = 1;
				break;
			}

			(it->chunk_start)[it->chunk_size++] = *p;
		}
	}
	else {
		it->chunk_size = 0;
		while(it->size_to_load > 0 && it->chunk_size < it->chunk_buff_len) {
			UNPACK_TAKE_BYTE(p);
			(it->chunk_start)[it->chunk_size++] = *p;
			it->size_to_load--;
		}
		it->last_chunk = (it->size_to_load == 0);
	}
	it->chunk_cnt++;
	unpack_context->item.type = CCPCP_ITEM_STRING;
}

void unpack_blob(ccpcp_unpack_context* unpack_context)
{
	unpack_string(unpack_context);
	unpack_context->item.type = CCPCP_ITEM_BLOB;
}

void cchainpack_unpack_next (ccpcp_unpack_context* unpack_context)
{
	if (unpack_context->err_no)
		return;

	if(unpack_context->item.type == CCPCP_ITEM_STRING || unpack_context->item.type == CCPCP_ITEM_BLOB) {
		ccpcp_string *str_it = &unpack_context->item.as.String;
		if(!str_it->last_chunk) {
			if(unpack_context->item.type == CCPCP_ITEM_STRING)
				unpack_string(unpack_context);
			else
				unpack_blob(unpack_context);
			return;
		}
	}

	const char *p;
	UNPACK_TAKE_BYTE(p);

	uint8_t packing_schema = (uint8_t)(*p);

	unpack_context->item.type = CCPCP_ITEM_INVALID;
	if(packing_schema < 128) {
		if(packing_schema & 64) {
			// tiny Int
			unpack_context->item.type = CCPCP_ITEM_INT;
			unpack_context->item.as.Int = packing_schema & 63;
		}
		else {
			// tiny UInt
			unpack_context->item.type = CCPCP_ITEM_UINT;
			unpack_context->item.as.UInt = packing_schema & 63;
		}
	}
	else {
		switch(packing_schema) {
		case CP_Null: {
			unpack_context->item.type = CCPCP_ITEM_NULL;
			break;
		}
		case CP_TRUE: {
			unpack_context->item.type = CCPCP_ITEM_BOOLEAN;
			unpack_context->item.as.Bool = 1;
			break;
		}
		case CP_FALSE: {
			unpack_context->item.type = CCPCP_ITEM_BOOLEAN;
			unpack_context->item.as.Bool = 0;
			break;
		}
		case CP_Int: {
			unpack_int(unpack_context);
			break;
		}
		case CP_UInt: {
			unpack_uint(unpack_context, NULL);
			break;
		}
		case CP_Double: {
			unpack_context->item.type = CCPCP_ITEM_DOUBLE;
			uint8_t*bytes = (uint8_t*)&(unpack_context->item.as.Double);
			int len = sizeof(double);

			int n = 1;
			int i;
			if(*(char *)&n == 1) {
				// little endian if true
				for (i=0; i<len; i++) {
					UNPACK_TAKE_BYTE(p);
					bytes[i] = (uint8_t)(*p);
				}
			}
			else {
				for (i=len-1; i>=0; i--) {
					UNPACK_TAKE_BYTE(p);
					bytes[i] = (uint8_t)(*p);
				}
			}
			break;
		}
		case CP_Decimal: {
			unpack_int(unpack_context);
			int64_t mant = unpack_context->item.as.Int;
			unpack_int(unpack_context);
			int64_t exp = unpack_context->item.as.Int;
			unpack_context->item.type = CCPCP_ITEM_DECIMAL;
			unpack_context->item.as.Decimal.mantisa = mant;
			unpack_context->item.as.Decimal.exponent = (int)exp;
			break;
		}
		case CP_DateTime: {
			unpack_int(unpack_context);
			int64_t d = unpack_context->item.as.Int;
			int8_t offset = 0;
			bool has_tz_offset = d & 1;
			bool has_not_msec = d & 2;
			d >>= 2;
			if (has_tz_offset) {
				offset = d & 0x7F;
				offset = (int8_t)(offset << 1);
				offset >>= 1; // sign extension
				d >>= 7;
			}
			if (has_not_msec) {
				if (__builtin_mul_overflow(d, 1000, &d)) {
					UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "DateTime msec value overflow.");
				}
			}
			d += SHV_EPOCH_MSEC;

			unpack_context->item.type = CCPCP_ITEM_DATE_TIME;
			ccpcp_date_time *it = &unpack_context->item.as.DateTime;
			it->msecs_since_epoch = d;
			it->minutes_from_utc = offset * (int)15;
			break;
		}
		case CP_MetaMap: {
			unpack_context->item.type = CCPCP_ITEM_META;
			break;
		}
		case CP_Map: {
			unpack_context->item.type = CCPCP_ITEM_MAP;
			break;
		}
		case CP_IMap: {
			unpack_context->item.type = CCPCP_ITEM_IMAP;
			break;
		}
		case CP_List: {
			unpack_context->item.type = CCPCP_ITEM_LIST;
			break;
		}
		case CP_TERM: {
			unpack_context->item.type = CCPCP_ITEM_CONTAINER_END;
			break;
		}
		case CP_Blob: {
			ccpcp_string *it = &unpack_context->item.as.String;
			ccpcp_string_init(it, unpack_context);
			unpack_uint(unpack_context, NULL);
			if(unpack_context->err_no == CCPCP_RC_OK) {
				it->string_size = (long)(unpack_context->item.as.UInt);
				it->size_to_load = it->string_size;
				unpack_blob(unpack_context);
			}
			break;
		}
		case CP_String: {
			ccpcp_string *it = &unpack_context->item.as.String;
			ccpcp_string_init(it, unpack_context);
			unpack_uint(unpack_context, NULL);
			if(unpack_context->err_no == CCPCP_RC_OK) {
				it->string_size = (long)(unpack_context->item.as.UInt);
				it->size_to_load = it->string_size;
				unpack_string(unpack_context);
			}
			break;
		}
		case CP_CString: {
			ccpcp_string *it = &unpack_context->item.as.String;
			ccpcp_string_init(it, unpack_context);
			it->string_size = -1;
			it->size_to_load = it->string_size;
			unpack_string(unpack_context);
			break;
		}
		default:
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid type info.");
		}
	}

	ccpcp_unpack_context_update_container_state(unpack_context);
}

uint64_t cchainpack_unpack_uint_data(ccpcp_unpack_context *unpack_context, bool *ok)
{
	int err_code;
	uint64_t n = cchainpack_unpack_uint_data2(unpack_context, &err_code);
	if(ok)
		*ok = (err_code == CCPCP_RC_OK);
	return n;
}

uint64_t cchainpack_unpack_uint_data2(ccpcp_unpack_context *unpack_context, int *err_code)
{
	unpack_uint(unpack_context, NULL);
	if(err_code)
		*err_code = unpack_context->err_no;
	return unpack_context->item.as.UInt;
}
