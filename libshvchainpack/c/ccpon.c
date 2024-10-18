#include <shv/chainpack/ccpon.h>

#include <string.h>
#include <math.h>

static inline uint8_t hexify(uint8_t b)
{
	if (b <= 9)
		return b + '0';
	if (b >= 10 && b <= 15)
		return (uint8_t)(b - 10 + 'a');
	return '?';
}

static inline int unhex(uint8_t b)
{
	if (b >= '0' && b <= '9')
		return b - '0';
	if (b >= 'a' && b <= 'f')
		return b - 'a' + 10;
	if (b >= 'A' && b <= 'F')
		return b - 'A' + 10;
	return -1;
}

static size_t uint_to_str(char *buff, size_t buff_len, uint64_t n)
{
	size_t len = 0;
	if(n == 0) {
		if(len < buff_len)
			buff[len] = '0';
		len++;
	}
	else while(n > 0) {
		char r = (char)(n % 10);
		n /= 10;
		if(len < buff_len)
			buff[len++] = '0' + r;
	}
	if(len < buff_len) {
		size_t i;
		for(i = 0; i < len/2; i++) {
			char c = buff[i];
			buff[i] = buff[len-i-1];
			buff[len-i-1] = c;
		}
	}
	return len;
}

static size_t uint_to_str_lpad(char *buff, size_t buff_len, uint64_t n, size_t width, char pad_char)
{
	size_t len = uint_to_str(buff, buff_len, n);
	if(len < width && width <= buff_len) {
		size_t i;
		for (i = 0; i < len; ++i) {
			buff[width-i-1] = buff[len-i-1];
			buff[len-i-1] = pad_char;
		}
		return width;
	}
	return len;
}

static size_t uint_dot_uint_to_str(char *buff, size_t buff_len, uint64_t n1, uint64_t n2)
{
	size_t len = uint_to_str(buff, buff_len, n1);
	if(len < buff_len)
		buff[len] = '.';
	size_t dot_pos = len;
	len++;
	if(n2 > 0) {
		len += uint_to_str(buff+len, (len < buff_len)? buff_len-len: 0, n2);
		if(len < buff_len) {
			// remove trailing zeros
			for(; len>dot_pos && buff[len-1] == '0'; len--);
		}
	}
	return len;
}

static size_t int_to_str(char *buff, size_t buff_len, int64_t n)
{
	size_t len = 0;
	char *buff2 = buff;
	if(n < 0) {
		buff2 = buff+1;
		if(len < buff_len)
			buff[len] = '-';
		len++;
		n = -n;
	}
	len += uint_to_str(buff2, buff_len, (uint64_t)n);
	return len;
}

static size_t double_to_str(char *buff, size_t buff_len, double d)
{
	const int prec = 6;
	unsigned prec_num = 1;
	int i;
	for (i = 0; i < prec; ++i)
		prec_num *= 10;

	size_t len = 0;
	if(d == 0) {
		if(len < buff_len)
			buff[len] = '0';
		len++;
		if(len < buff_len)
			buff[len] = '.';
		len++;
	}
	else {
		bool neg = d < 0;
		if(neg) {
			if(len < buff_len)
				buff[len] = '-';
			len++;
			d = -d;
		}
		if(d < 1e7 && d >= 0.1) {
			/// float point notation
			if(d < 1) {
				for ( i = 0; i < prec; ++i)
					d *= 10;
				unsigned ud = (unsigned)d;
				len += uint_dot_uint_to_str(buff + len, (len < buff_len)? buff_len-len: 0, 0, ud);
			}
			else {
				double myprec = 1;
				while(d >= 10) {
					d /= 10;
					myprec /= 10;
				}
				for ( i = 0; i < prec; ++i) {
					d *= 10;
					myprec *= 10;
				}
				unsigned ud = (unsigned)d;
				unsigned myprecnum = (unsigned)myprec;
				len += uint_dot_uint_to_str(buff + len, (len < buff_len)? buff_len-len: 0, ud/myprecnum, ud%myprecnum);
			}
		}
		else {
			/// exponential notation
			int exp = 0;
			if(d < 1) {
				while(d < 1) {
					d *= 10;
					exp--;
				}
				for ( i = 0; i < prec; ++i)
					d *= 10;
				unsigned ud = (unsigned)d;
				len += uint_dot_uint_to_str(buff + len, (len < buff_len)? buff_len-len: 0, ud/prec_num, ud%prec_num);
			}
			else {
				while(d >= 10) {
					d /= 10;
					exp++;
				}
				for ( i = 0; i < prec; ++i)
					d *= 10;
				unsigned ud = (unsigned)d;
				len += uint_dot_uint_to_str(buff + len, (len < buff_len)? buff_len-len: 0, ud/prec_num, ud%prec_num);
			}
			if(len < buff_len && buff[len-1] == '.') // remove trailing dot
				len--;
			if(len < buff_len)
				buff[len] = 'e';
			len++;
			len += int_to_str(buff + len, (len < buff_len)? buff_len-len: 0, exp);
		}
	}
	return len;
}

// see http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
// see https://stackoverflow.com/questions/16647819/timegm-cross-platform
// see https://www.boost.org/doc/libs/1_62_0/boost/chrono/io/time_point_io.hpp
static int is_leap(int y)
{
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

static int32_t days_from_0(int32_t year)
{
	year--;
	return 365 * year + (year / 400) - (year/100) + (year / 4);
}

static int32_t days_from_1970(int32_t year)
{
	static int32_t days_from_0_to_1970 = 0;
	if(days_from_0_to_1970 == 0)
		days_from_0_to_1970 = days_from_0(1970);
	return days_from_0(year) - days_from_0_to_1970;
}

static int days_from_1jan(int year, int month, int mday)
{
	static const int days[2][12] =
	{
		{ 0,31,59,90,120,151,181,212,243,273,304,334},
		{ 0,31,60,91,121,152,182,213,244,274,305,335}
	};

	return days[is_leap(year)][month] + mday - 1;
}

int64_t ccpon_timegm(struct tm *tm)
{
	// leap seconds are not part of Posix
	int64_t res = 0;
	int year = tm->tm_year + 1900;
	int month = tm->tm_mon; // 0 - 11
	int mday = tm->tm_mday; // 1 - 31
	res = days_from_1970(year);
	res += days_from_1jan(year, month, mday);
	res *= 24;
	res += tm->tm_hour;
	res *= 60;
	res += tm->tm_min;
	res *= 60;
	res += tm->tm_sec;
	return res;
}

// Returns year/month/day triple in civil calendar
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(), numeric_limits<Int>::max()-719468].
static void civil_from_days(long long z, int *py, unsigned *pm, unsigned *pd)
{
	int y;
	unsigned m;
	unsigned d;
	z += 719468;
	const long long era = (z >= 0 ? z : z - 146096) / 146097;
	const unsigned doe = (const unsigned)(z - era * 146097);          // [0, 146096]
	const unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;  // [0, 399]
	y = (int)(((long)yoe) + era * 400);
	const unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);                // [0, 365]
	const unsigned mp = (5*doy + 2)/153;                                   // [0, 11]
	d = doy - (153*mp+2)/5 + 1;                             // [1, 31]
	if (mp < 10)
		m = mp + 3;
	else
		m = mp - 9;
	y += (m <= 2);
	--m;
	if(py)
		*py = y;
	if(pm)
		*pm = m;
	if(pd)
		*pd = d;
}

void ccpon_gmtime(int64_t epoch_sec, struct tm *tm)
{
	if (!tm)
		return;

	const long seconds_in_day = 3600 * 24;
	long long days_since_epoch = (epoch_sec / seconds_in_day);
	long long hms = epoch_sec - seconds_in_day * days_since_epoch;
	if (hms < 0) {
		days_since_epoch -= 1;
		hms = seconds_in_day + hms;
	}

	int y;
	unsigned m, d;
	civil_from_days(days_since_epoch, &y, &m, &d);
	tm->tm_year = y - 1900;
	tm->tm_mon = (int)m;
	tm->tm_mday = (int)d;

	tm->tm_hour = (int)(hms / 3600);
	const int ms = (int)(hms % 3600);
	tm->tm_min = ms / 60;
	tm->tm_sec = ms % 60;

	tm->tm_isdst = -1;
}

static const char CCPON_STR_NULL[] = "null";
static const char CCPON_STR_TRUE[] = "true";
static const char CCPON_STR_FALSE[] = "false";
static const char CCPON_STR_IMAP_BEGIN[] = "i{";
static const char CCPON_DATE_TIME_BEGIN[] = "d\"";

enum {
	CCPON_C_KEY_DELIM = ':',
	CCPON_C_FIELD_DELIM = ',',
	CCPON_C_LIST_BEGIN = '[',
	CCPON_C_LIST_END = ']',
	CCPON_C_ARRAY_END = ']',
	CCPON_C_MAP_BEGIN = '{',
	CCPON_C_MAP_END = '}',
	CCPON_C_META_BEGIN = '<',
	CCPON_C_META_END = '>',
	CCPON_C_UNSIGNED_END = 'u'
};

#ifdef FORCE_NO_LIBRARY

static void	*memcpy(void *dst, const void *src, size_t n)
{
    unsigned int i;
    uint8_t *d=(uint8_t*)dst, *s=(uint8_t*)src;
    for (i=0; i<n; i++)
    {
        *d++ = *s++;
    }
    return dst;
}

#endif

//============================   P A C K   =================================
void ccpon_pack_copy_str(ccpcp_pack_context *pack_context, const char *str)
{
	size_t len = strlen(str);
	ccpcp_pack_copy_bytes(pack_context, str, len);
}

static void start_block(ccpcp_pack_context* pack_context)
{
	pack_context->nest_count++;
}

static void indent_element(ccpcp_pack_context* pack_context, bool is_oneliner, bool is_first_field)
{
	if(pack_context->cpon_options.indent) {
		if(is_oneliner) {
			if(!is_first_field)
				ccpcp_pack_copy_byte(pack_context, ' ');
		}
		else {
			ccpcp_pack_copy_bytes(pack_context, "\n", 1);
			int i;
			for (i = 0; i < pack_context->nest_count; ++i) {
				ccpon_pack_copy_str(pack_context, pack_context->cpon_options.indent);
			}
		}
	}
}

static void end_block(ccpcp_pack_context* pack_context, bool is_oneliner)
{
	pack_context->nest_count--;
	if(pack_context->cpon_options.indent) {
		indent_element(pack_context, is_oneliner, true);
	}
}

#define INT_STRING_LEN 32u

void ccpon_pack_uint(ccpcp_pack_context* pack_context, uint64_t i)
{
	if (pack_context->err_no)
		return;

	// at least 21 characters for 64-bit types.
	char str[INT_STRING_LEN];
	size_t n = uint_to_str(str, INT_STRING_LEN, i);
	if(n >= INT_STRING_LEN) {
		pack_context->err_no = CCPCP_RC_LOGICAL_ERROR;
		return;
	}
	ccpcp_pack_copy_bytes(pack_context, str, n);
	if(!pack_context->cpon_options.json_output)
		ccpcp_pack_copy_byte(pack_context, 'u');
}

void ccpon_pack_int(ccpcp_pack_context* pack_context, int64_t i)
{
	if (pack_context->err_no)
		return;

	// at least 21 characters for 64-bit types.
	char str[INT_STRING_LEN];
	size_t n = int_to_str(str, INT_STRING_LEN, i);
	if(n >= INT_STRING_LEN) {
		pack_context->err_no = CCPCP_RC_LOGICAL_ERROR;
		return;
	}
	ccpcp_pack_copy_bytes(pack_context, str, n);
}

#define DECIMAL_STRING_LEN 64u

void ccpon_pack_decimal(ccpcp_pack_context *pack_context, int64_t mantisa, int exponent)
{
	// at least 21 characters for 64-bit types.
	char buff[DECIMAL_STRING_LEN];
	size_t n = ccpcp_decimal_to_string(buff, DECIMAL_STRING_LEN, mantisa, exponent);
	if(n == 0) {
		pack_context->err_no = CCPCP_RC_LOGICAL_ERROR;
		return;
	}

	ccpcp_pack_copy_bytes(pack_context, buff, (size_t)n);
}

#define DOUBLE_STRING_LEN 32u

void ccpon_pack_double(ccpcp_pack_context* pack_context, double d)
{
	if (pack_context->err_no)
		return;

	// at least 21 characters for 64-bit types.
	char str[DOUBLE_STRING_LEN];
	size_t n = double_to_str(str, DOUBLE_STRING_LEN, d);
	if(n >= DOUBLE_STRING_LEN) {
		pack_context->err_no = CCPCP_RC_LOGICAL_ERROR;
		return;
	}
	ccpcp_pack_copy_bytes(pack_context, str, n);
}

void ccpon_pack_date_time(ccpcp_pack_context *pack_context, int64_t epoch_msecs, int min_from_utc)
{
	/// ISO 8601 with msecs extension
	ccpcp_pack_copy_bytes(pack_context, CCPON_DATE_TIME_BEGIN, sizeof (CCPON_DATE_TIME_BEGIN) - 1);
	ccpon_pack_date_time_str(pack_context, epoch_msecs, min_from_utc, CCPON_Auto, true);
	ccpcp_pack_copy_bytes(pack_context, "\"", 1);
}

#define DATETIME_STRING_LEN 32u

void ccpon_pack_date_time_str(ccpcp_pack_context *pack_context, int64_t epoch_msecs, int min_from_utc, ccpon_msec_policy msec_policy, bool with_tz)
{
	struct tm tm;
	ccpon_gmtime(epoch_msecs / 1000 + min_from_utc * 60, &tm);
	char str[DATETIME_STRING_LEN];
	size_t len = uint_to_str_lpad(str, DATETIME_STRING_LEN, (unsigned)tm.tm_year + 1900, 2, '0');
	if(len < DATETIME_STRING_LEN)
		str[len] = '-';
	len++;
	len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)tm.tm_mon + 1, 2, '0');
	if(len < DATETIME_STRING_LEN)
		str[len] = '-';
	len++;
	len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)tm.tm_mday, 2, '0');
	if(len < DATETIME_STRING_LEN)
		str[len] = 'T';
	len++;
	len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)tm.tm_hour, 2, '0');
	if(len < DATETIME_STRING_LEN)
		str[len] = ':';
	len++;
	len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)tm.tm_min, 2, '0');
	if(len < DATETIME_STRING_LEN)
		str[len] = ':';
	len++;
	len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)tm.tm_sec, 2, '0');
	if(len < DATETIME_STRING_LEN)
		ccpcp_pack_copy_bytes(pack_context, str, len);
	int msec = (int)(epoch_msecs % 1000);
	if((msec > 0 && msec_policy == CCPON_Auto) || msec_policy == CCPON_Always) {
		ccpcp_pack_copy_bytes(pack_context, ".", 1);
		len = uint_to_str_lpad(str, DATETIME_STRING_LEN, (unsigned)msec, 3, '0');
		if(len < DATETIME_STRING_LEN)
			ccpcp_pack_copy_bytes(pack_context, str, len);
	}
	if(with_tz) {
		if(min_from_utc == 0) {
			ccpcp_pack_copy_bytes(pack_context, "Z", 1);
		}
		else {
			if(min_from_utc < 0) {
				ccpcp_pack_copy_bytes(pack_context, "-", 1);
				min_from_utc = -min_from_utc;
			}
			else {
				ccpcp_pack_copy_bytes(pack_context, "+", 1);
			}
			if(min_from_utc%60) {
				len = 0;
				len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)min_from_utc/60, 2, '0');
				len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)min_from_utc%60, 2, '0');
			}
			else {
				len = 0;
				len += uint_to_str_lpad(str + len, (len < DATETIME_STRING_LEN)? DATETIME_STRING_LEN - len: 0, (unsigned)min_from_utc/60, 2, '0');
			}
			if(len < DATETIME_STRING_LEN)
				ccpcp_pack_copy_bytes(pack_context, str, len);
		}
	}
}

void ccpon_pack_null(ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_bytes(pack_context, CCPON_STR_NULL, sizeof(CCPON_STR_NULL) - 1);
}

static void ccpon_pack_true (ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_bytes(pack_context, CCPON_STR_TRUE, sizeof(CCPON_STR_TRUE) - 1);
}

static void ccpon_pack_false (ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;
	ccpcp_pack_copy_bytes(pack_context, CCPON_STR_FALSE, sizeof(CCPON_STR_FALSE) - 1);
}

void ccpon_pack_boolean(ccpcp_pack_context* pack_context, bool b)
{
	if (pack_context->err_no)
		return;

	if(b)
		ccpon_pack_true(pack_context);
	else
		ccpon_pack_false(pack_context);
}

void ccpon_pack_list_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

	if(ccpcp_pack_copy_byte(pack_context, CCPON_C_LIST_BEGIN) > 0)
		start_block(pack_context);
}

void ccpon_pack_list_end(ccpcp_pack_context *pack_context, bool is_oneliner)
{
	if (pack_context->err_no)
		return;

	end_block(pack_context, is_oneliner);
	ccpcp_pack_copy_byte(pack_context, CCPON_C_LIST_END);
}

void ccpon_pack_map_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

	if(ccpcp_pack_copy_byte(pack_context, CCPON_C_MAP_BEGIN))
		start_block(pack_context);
}

void ccpon_pack_map_end(ccpcp_pack_context *pack_context, bool is_oneliner)
{
	if (pack_context->err_no)
		return;

	end_block(pack_context, is_oneliner);
	ccpcp_pack_copy_byte(pack_context, CCPON_C_MAP_END);
}

void ccpon_pack_imap_begin(ccpcp_pack_context* pack_context)
{
	if (pack_context->err_no)
		return;

	ccpcp_pack_copy_bytes(pack_context, CCPON_STR_IMAP_BEGIN, sizeof(CCPON_STR_IMAP_BEGIN)-1);
	start_block(pack_context);
}

void ccpon_pack_imap_end(ccpcp_pack_context *pack_context, bool is_oneliner)
{
	ccpon_pack_map_end(pack_context, is_oneliner);
}

void ccpon_pack_meta_begin(ccpcp_pack_context *pack_context)
{
	if (pack_context->err_no)
		return;

	if(ccpcp_pack_copy_byte(pack_context, CCPON_C_META_BEGIN) > 0)
		start_block(pack_context);
}

void ccpon_pack_meta_end(ccpcp_pack_context *pack_context, bool is_oneliner)
{
	if (pack_context->err_no)
		return;

	end_block(pack_context, is_oneliner);
	ccpcp_pack_copy_byte(pack_context, CCPON_C_META_END);
}

static char* copy_data_escaped(ccpcp_pack_context* pack_context, const void* str, size_t len)
{
	size_t i;
	for (i = 0; i < len; ++i) {
		if(pack_context->err_no != CCPCP_RC_OK)
			return NULL;
		uint8_t ch = ((const uint8_t*)str)[i];
		switch(ch) {
		case '\0':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, '0');
			break;
		case '\\':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, '\\');
			break;
		case '\t':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, 't');
			break;
		case '\b':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, 'b');
			break;
		case '\r':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, 'r');
			break;
		case '\n':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, 'n');
			break;
		case '"':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, '"');
			break;
		default:
			ccpcp_pack_copy_byte(pack_context, ch);
		}
	}
	return pack_context->current;
}

static char* copy_blob_escaped(ccpcp_pack_context* pack_context, const void* str, size_t len)
{
	size_t i;
	for (i = 0; i < len; ++i) {
		if(pack_context->err_no != CCPCP_RC_OK)
			return NULL;
		uint8_t ch = ((const uint8_t*)str)[i];
		switch(ch) {
		case '\\':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, '\\');
			break;
		case '\t':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, 't');
			break;
		case '\r':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, 'r');
			break;
		case '\n':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, 'n');
			break;
		case '"':
			ccpcp_pack_copy_byte(pack_context, '\\');
			ccpcp_pack_copy_byte(pack_context, '"');
			break;
		default:
			if(ch < 32 || ch >= 127) {
				ccpcp_pack_copy_byte(pack_context, '\\');
				ccpcp_pack_copy_byte(pack_context, hexify(ch / 16));
				ccpcp_pack_copy_byte(pack_context, hexify(ch % 16));
			}
			else {
				ccpcp_pack_copy_byte(pack_context, ch);
			}
		}
	}
	return pack_context->current;
}

void ccpon_pack_blob(ccpcp_pack_context* pack_context, const uint8_t* buff, size_t buff_len)
{
	ccpon_pack_blob_start(pack_context, 0, 0);
	ccpon_pack_blob_cont(pack_context, buff, (unsigned int)buff_len);
	ccpon_pack_blob_finish(pack_context);
}

void ccpon_pack_blob_start (ccpcp_pack_context* pack_context, const uint8_t* buff, size_t buff_len)
{
	ccpcp_pack_copy_byte(pack_context, 'b');
	ccpcp_pack_copy_byte(pack_context, '"');
	copy_blob_escaped(pack_context, buff, buff_len);
}

void ccpon_pack_blob_cont (ccpcp_pack_context* pack_context, const uint8_t* buff, unsigned buff_len)
{
	copy_blob_escaped(pack_context, buff, buff_len);
}

void ccpon_pack_blob_finish (ccpcp_pack_context* pack_context)
{
	ccpcp_pack_copy_byte(pack_context, '"');
}

void ccpon_pack_string(ccpcp_pack_context* pack_context, const char* s, size_t l)
{
	ccpcp_pack_copy_byte(pack_context, '"');
	copy_data_escaped(pack_context, s, l);
	ccpcp_pack_copy_byte(pack_context, '"');
}

void ccpon_pack_string_terminated (ccpcp_pack_context* pack_context, const char* s)
{
	size_t len = s? strlen(s): 0;
	ccpon_pack_string(pack_context, s, len);
}

void ccpon_pack_string_start (ccpcp_pack_context* pack_context, const char*buff, size_t buff_len)
{
	ccpcp_pack_copy_byte(pack_context, '"');
	copy_data_escaped(pack_context, buff, buff_len);
}

void ccpon_pack_string_cont (ccpcp_pack_context* pack_context, const char*buff, unsigned buff_len)
{
	copy_data_escaped(pack_context, buff, buff_len);
}

void ccpon_pack_string_finish (ccpcp_pack_context* pack_context)
{
	ccpcp_pack_copy_byte(pack_context, '"');
}

//============================   U N P A C K   =================================

const char* ccpon_unpack_skip_insignificant(ccpcp_unpack_context* unpack_context)
{
	while(1) {
		const char* p = ccpcp_unpack_take_byte(unpack_context);
		if(!p)
			return p;
		if(*p == 0) {
			continue;
		}
		if(*p < 0) {
			// skip all characters with ASCII > 128
			// because they cannot appear in CPON delimiters
			continue;
		}
		if(*p == '\n') {
			unpack_context->parser_line_no++;
		}
		else if(*p > ' ') {
			switch (*p) {
			case '/': {
				p = ccpcp_unpack_take_byte(unpack_context);
				if(!p) {
					unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
					unpack_context->err_msg = "Unfinished comment";
				}
				else if(*p == '*') {
					//multiline_comment_entered;
					while(1) {
						p = ccpcp_unpack_take_byte(unpack_context);
						if(!p)
							return p;
						if(*p == '\n')
							unpack_context->parser_line_no++;
						if(*p == '*') {
							p = ccpcp_unpack_take_byte(unpack_context);
							if(*p == '/')
								break;
						}
					}
				}
				else if(*p == '/') {
					// to end of line comment entered;
					while(1) {
						p = ccpcp_unpack_take_byte(unpack_context);
						if(!p)
							return p;
						if(*p == '\n') {
							unpack_context->parser_line_no++;
							break;
						}
					}
				}
				else {
					return NULL;
				}
				break;
			}
			case CCPON_C_KEY_DELIM:
			case CCPON_C_FIELD_DELIM:
				continue;
			default:
				return p;
			}
		}
	}
}

static int unpack_int(ccpcp_unpack_context* unpack_context, int64_t *p_val)
{
	int64_t val = 0;
	int neg = 0;
	int base = 10;
	int n = 0;
	for (; ; n++) {
		const char *p = ccpcp_unpack_take_byte(unpack_context);
		if(!p)
			goto eonumb;
		uint8_t b = (uint8_t)(*p);
		switch (b) {
		case '+':
		case '-':
			if(n != 0) {
				unpack_context->current--;
				goto eonumb;
			}
			if(b == '-')
				neg = 1;
			break;
		case 'x':
			if(n == 1 && val != 0) {
				unpack_context->current--;
				goto eonumb;
			}
			if(n != 1) {
				unpack_context->current--;
				goto eonumb;
			}
			base = 16;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			val *= base;
			val += b - '0';
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if(base != 16) {
				unpack_context->current--;
				goto eonumb;
			}
			val *= base;
			val += b - 'a' + 10;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if(base != 16) {
				unpack_context->current--;
				goto eonumb;
			}
			val *= base;
			val += b - 'A' + 10;
			break;
		default:
			unpack_context->current--;
			goto eonumb;
		}
	}
eonumb:
	if(neg)
		val = -val;
	if(p_val)
		*p_val = val;
	return n;
}

void ccpon_unpack_date_time(ccpcp_unpack_context *unpack_context, struct tm *tm, int *msec, int *utc_offset)
{
	tm->tm_year = 0;
	tm->tm_mon = 0;
	tm->tm_mday = 1;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	tm->tm_isdst = -1;

	*msec = 0;
	*utc_offset = 0;

	const char *p;

	int64_t val;
	int n = unpack_int(unpack_context, &val);
	if(n < 0) {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed year in DateTime";
		return;
	}
	tm->tm_year = (int)val - 1900;

	UNPACK_TAKE_BYTE(p);
	if(*p != '-') {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed year-month separator in DateTime";
		return;
	}

	n = unpack_int(unpack_context, &val);
	if(n <= 0) {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed month in DateTime";
		return;
	}
	tm->tm_mon = (int)val - 1;

	UNPACK_TAKE_BYTE(p);
	if(*p != '-') {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed month-day separator in DateTime";
		return;
	}

	n = unpack_int(unpack_context, &val);
	if(n <= 0) {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed day in DateTime";
		return;
	}
	tm->tm_mday = (int)val;

	UNPACK_TAKE_BYTE(p);
	if(!(*p == 'T' || *p == ' ')) {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed date-time separator in DateTime";
		return;
	}

	n = unpack_int(unpack_context, &val);
	if(n <= 0) {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed hour in DateTime";
		return;
	}
	tm->tm_hour = (int)val;

	UNPACK_TAKE_BYTE(p);

	n = unpack_int(unpack_context, &val);
	if(n <= 0) {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed minutes in DateTime";
		return;
	}
	tm->tm_min = (int)val;

	UNPACK_TAKE_BYTE(p);

	n = unpack_int(unpack_context, &val);
	if(n <= 0) {
		unpack_context->err_no = CCPCP_RC_MALFORMED_INPUT;
		unpack_context->err_msg = "Malformed seconds in DateTime";
		return;
	}
	tm->tm_sec = (int)val;

	p = ccpcp_unpack_take_byte(unpack_context);
	if(p) {
		if(*p == '.') {
			n = unpack_int(unpack_context, &val);
			if(n < 0)
				return;
			*msec = (int)val;
			p = ccpcp_unpack_take_byte(unpack_context);
		}
		if(p) {
			uint8_t b = (uint8_t)(*p);
			if(b == 'Z') {
				// UTC time
			}
			else if(b == '+' || b == '-') {
				// UTC time
				n = unpack_int(unpack_context, &val);
				if(!(n == 2 || n == 4))
					UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed TS offset in DateTime.");
				if(n == 2)
					*utc_offset = (int)(60 * val);
				else if(n == 4)
					*utc_offset = (int)(60 * (val / 100) + (val % 100));
				if(b == '-')
					*utc_offset = -*utc_offset;
			}
			else {
				// unget unused char
				unpack_context->current--;
			}
		}
	}
	unpack_context->err_no = CCPCP_RC_OK;
	unpack_context->item.type = CCPCP_ITEM_DATE_TIME;
	int64_t epoch_sec = ccpon_timegm(tm);
	epoch_sec -= *utc_offset * 60;
	int64_t epoch_msec = epoch_sec * 1000;
	ccpcp_date_time *it = &unpack_context->item.as.DateTime;
	epoch_msec += *msec;
	it->msecs_since_epoch = epoch_msec;
	it->minutes_from_utc = *utc_offset;
}

static void ccpon_unpack_blob_hex(ccpcp_unpack_context* unpack_context)
{
	if(unpack_context->item.type != CCPCP_ITEM_BLOB)
		UNPACK_ERROR(CCPCP_RC_LOGICAL_ERROR, "Unpack cpon blob internal error.");

	const char *p;
	ccpcp_string *it = &unpack_context->item.as.String;
	if(it->chunk_cnt == 0) {
		// must start with '"'
		UNPACK_TAKE_BYTE(p);
		if (*p != '"') {
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Blob should start with 'x\"' .");
		}
	}
	for(it->chunk_size = 0; it->chunk_size < it->chunk_buff_len; ) {
		do {
			UNPACK_TAKE_BYTE(p);
		} while(*p <= ' ');
		if (*p == '"') {
			// end of string
			it->last_chunk = 1;
			break;
		}
		int b1 = unhex((uint8_t)(*p));
		if(b1 < 0)
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid HEX char, first digit.");
		UNPACK_TAKE_BYTE(p);
		if(!p)
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid HEX char, second digit missing.");
		int b2 = unhex((uint8_t)(*p));
		if(b2 < 0)
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid HEX char, second digit.");
		(it->chunk_start)[it->chunk_size++] = (char)((uint8_t)(16 * b1 + b2));
	}
	it->chunk_cnt++;
}

static void ccpon_unpack_blob_esc(ccpcp_unpack_context* unpack_context)
{
	if(unpack_context->item.type != CCPCP_ITEM_BLOB)
		UNPACK_ERROR(CCPCP_RC_LOGICAL_ERROR, "Unpack cpon blob internal error.");

	const char *p;
	ccpcp_string *it = &unpack_context->item.as.String;
	if(it->chunk_cnt == 0) {
		// must start with '"'
		UNPACK_TAKE_BYTE(p);
		if (*p != '"') {
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Blob should start with 'b\"' .");
		}
	}
	for(it->chunk_size = 0; it->chunk_size < it->chunk_buff_len; ) {
		UNPACK_TAKE_BYTE(p);
		uint8_t b = (uint8_t)(*p);
		if (b == '"') {
			// end of string
			it->last_chunk = 1;
			break;
		}
		if(b == '\\') {
			UNPACK_TAKE_BYTE(p);
			switch((uint8_t)*p) {
			case 't': (it->chunk_start)[it->chunk_size++] = '\t'; break;
			case 'r': (it->chunk_start)[it->chunk_size++] = '\r'; break;
			case 'n': (it->chunk_start)[it->chunk_size++] = '\n'; break;
			case '"': (it->chunk_start)[it->chunk_size++] = '"'; break;
			case '\\': (it->chunk_start)[it->chunk_size++] = '\\'; break;
			default: {
				int hi = unhex((uint8_t)(*p));
				if(hi < 0)
					UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid HEX char.");
				UNPACK_TAKE_BYTE(p);
				int lo = unhex((uint8_t)(*p));
				if(lo < 0)
					UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid HEX char.");
				(it->chunk_start)[it->chunk_size++] = (char)((uint8_t)(16 * hi + lo));
				break;
			}
			};
		}
		else if(b < 128) {
			(it->chunk_start)[it->chunk_size++] = (char)b;
		}
		else {
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid blob char, code >= 128.");
		}
	}
	it->chunk_cnt++;
}

static void ccpon_unpack_string(ccpcp_unpack_context* unpack_context)
{
	if(unpack_context->item.type != CCPCP_ITEM_STRING)
		UNPACK_ERROR(CCPCP_RC_LOGICAL_ERROR, "Unpack cpon string internal error.");

	const char *p;
	ccpcp_string *it = &unpack_context->item.as.String;
	if(it->chunk_cnt == 0) {
		// must start with '"'
		UNPACK_TAKE_BYTE(p);
		if (*p != '"') {
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "String should start with '\"' character.");
		}
	}
	for(it->chunk_size = 0; it->chunk_size < it->chunk_buff_len; ) {
		UNPACK_TAKE_BYTE(p);
		if(*p == '\\') {
			UNPACK_TAKE_BYTE(p);
			if(!p)
				return;
			switch (*p) {
			case '\\': (it->chunk_start)[it->chunk_size++] = '\\'; break;
			case '"' : (it->chunk_start)[it->chunk_size++] = '"'; break;
			case 'b': (it->chunk_start)[it->chunk_size++] = '\b'; break;
			case 'f': (it->chunk_start)[it->chunk_size++] = '\f'; break;
			case 'n': (it->chunk_start)[it->chunk_size++] = '\n'; break;
			case 'r': (it->chunk_start)[it->chunk_size++] = '\r'; break;
			case 't': (it->chunk_start)[it->chunk_size++] = '\t'; break;
			case '0': (it->chunk_start)[it->chunk_size++] = '\0'; break;
			default: (it->chunk_start)[it->chunk_size++] = *p; break;
			}
		}
		else {
			if (*p == '"') {
				// end of string
				it->last_chunk = 1;
				break;
			}
			(it->chunk_start)[it->chunk_size++] = *p;
		}
	}
	it->chunk_cnt++;
}

void ccpon_unpack_next (ccpcp_unpack_context* unpack_context)
{
	if (unpack_context->err_no)
		return;

	const char *p;
	if(unpack_context->item.type == CCPCP_ITEM_STRING) {
		ccpcp_string *str_it = &unpack_context->item.as.String;
		if(!str_it->last_chunk) {
			ccpon_unpack_string(unpack_context);
			return;
		}
	}
	else if(unpack_context->item.type == CCPCP_ITEM_BLOB) {
		ccpcp_string *str_it = &unpack_context->item.as.String;
		if(!str_it->last_chunk) {
			if(str_it->blob_hex)
				ccpon_unpack_blob_hex(unpack_context);
			else
				ccpon_unpack_blob_esc(unpack_context);
			return;
		}
	}

	unpack_context->item.type = CCPCP_ITEM_INVALID;

	p = ccpon_unpack_skip_insignificant(unpack_context);
	if(!p)
		return;

	switch(*p) {
	case CCPON_C_LIST_END:
	case CCPON_C_MAP_END:
	case CCPON_C_META_END:  {
		unpack_context->item.type = CCPCP_ITEM_CONTAINER_END;
		if(unpack_context->container_stack) {
			ccpcp_container_state *top_cont_state = ccpcp_unpack_context_top_container_state(unpack_context);
			if(!top_cont_state)
				UNPACK_ERROR(CCPCP_RC_CONTAINER_STACK_UNDERFLOW, "Container stack underflow.")
		}
		break;
	}
	case CCPON_C_META_BEGIN:
		unpack_context->item.type = CCPCP_ITEM_META;
		break;
	case CCPON_C_MAP_BEGIN:
		unpack_context->item.type = CCPCP_ITEM_MAP;
		break;
	case CCPON_C_LIST_BEGIN:
		unpack_context->item.type = CCPCP_ITEM_LIST;
		break;
	case 'i': {
		UNPACK_TAKE_BYTE(p);
		if(*p != '{')
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "IMap should start with '{'.")
		unpack_context->item.type = CCPCP_ITEM_IMAP;
		break;
	}
	case 'a': {
		UNPACK_TAKE_BYTE(p);
		if(*p != '[')
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "List should start with '['.")
		// unpack unsupported ARRAY type as list
		unpack_context->item.type = CCPCP_ITEM_LIST;
		break;
	}
	case 'd': {
		UNPACK_TAKE_BYTE(p);
		if(!p || *p != '"')
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "DateTime should start with 'd'.")
		struct tm tm;
		int msec;
		int utc_offset;
		ccpon_unpack_date_time(unpack_context, &tm, &msec, &utc_offset);
		UNPACK_TAKE_BYTE(p);
		if(!p || *p != '"')
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "DateTime should start with 'd\"'.")
		break;
	}
	case 'n': {
		UNPACK_TAKE_BYTE(p);
		if(*p == 'u') {
			UNPACK_TAKE_BYTE(p);
			if(*p == 'l') {
				UNPACK_TAKE_BYTE(p);
				if(*p == 'l') {
					unpack_context->item.type = CCPCP_ITEM_NULL;
					break;
				}
			}
		}
		UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed 'null' literal.")
	}
	case 'f': {
		UNPACK_TAKE_BYTE(p);
		if(*p == 'a') {
			UNPACK_TAKE_BYTE(p);
			if(*p == 'l') {
				UNPACK_TAKE_BYTE(p);
				if(*p == 's') {
					UNPACK_TAKE_BYTE(p);
					if(*p == 'e') {
						unpack_context->item.type = CCPCP_ITEM_BOOLEAN;
						unpack_context->item.as.Bool = false;
						break;
					}
				}
			}
		}
		UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed 'false' literal.")
	}
	case 't': {
		UNPACK_TAKE_BYTE(p);
		if(*p == 'r') {
			UNPACK_TAKE_BYTE(p);
			if(*p == 'u') {
				UNPACK_TAKE_BYTE(p);
				if(*p == 'e') {
					unpack_context->item.type = CCPCP_ITEM_BOOLEAN;
					unpack_context->item.as.Bool = true;
					break;
				}
			}
		}
		UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed 'true' literal.")
	}
	case 'x': {
		UNPACK_TAKE_BYTE(p);
		if(*p != '"')
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "HEX string should start with 'x\"'.")
		unpack_context->item.type = CCPCP_ITEM_BLOB;
		ccpcp_string *str_it = &unpack_context->item.as.String;
		ccpcp_string_init(str_it, unpack_context);
		str_it->blob_hex = 1;
		unpack_context->current--;
		ccpon_unpack_blob_hex(unpack_context);
		break;
	}
	case 'b': {
		UNPACK_TAKE_BYTE(p);
		if(*p != '"')
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "BLOB string should start with 'b\"'.")
		unpack_context->item.type = CCPCP_ITEM_BLOB;
		ccpcp_string *str_it = &unpack_context->item.as.String;
		ccpcp_string_init(str_it, unpack_context);
		str_it->blob_hex = 0;
		unpack_context->current--;
		ccpon_unpack_blob_esc(unpack_context);
		break;
	}
	case '"': {
		unpack_context->item.type = CCPCP_ITEM_STRING;
		ccpcp_string *str_it = &unpack_context->item.as.String;
		ccpcp_string_init(str_it, unpack_context);
		unpack_context->current--;
		ccpon_unpack_string(unpack_context);
		break;
	}
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '+':
	case '-': {
		// number
		int64_t mantisa = 0;
		int64_t exponent = 0;
		int64_t decimals = 0;
		int dec_cnt = 0;
		struct {
			uint8_t is_decimal: 1;
			uint8_t is_uint: 1;
			uint8_t is_neg: 1;
		} flags;
		flags.is_decimal = 0;
		flags.is_uint = 0;
		flags.is_neg = 0;

		flags.is_neg = *p == '-';
		if(!flags.is_neg)
			unpack_context->current--;
		int n = unpack_int(unpack_context, &mantisa);
		if(n < 0)
			UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed number.")
		p = ccpcp_unpack_take_byte(unpack_context);
		while(p) {
			if(*p == CCPON_C_UNSIGNED_END) {
				flags.is_uint = 1;
				break;
			}
			if(*p == '.') {
				flags.is_decimal = 1;
				n = unpack_int(unpack_context, &decimals);
				if(n < 0)
					UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed number decimal part.")
				dec_cnt = n;
				p = ccpcp_unpack_take_byte(unpack_context);
				if(!p)
					break;
			}
			if(*p == 'e' || *p == 'E') {
				flags.is_decimal = 1;
				n = unpack_int(unpack_context, &exponent);
				if(n < 0)
					UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Malformed number exponetional part.")
				break;
			}
			if(*p != '.') {
				// unget char
				unpack_context->current--;
			}
			break;
		}
		if(flags.is_decimal) {
			int i;
			for (i = 0; i < dec_cnt; ++i)
				mantisa *= 10;
			mantisa += decimals;
			unpack_context->item.type = CCPCP_ITEM_DECIMAL;
			unpack_context->item.as.Decimal.mantisa = flags.is_neg? -mantisa: mantisa;
			unpack_context->item.as.Decimal.exponent = (int)(exponent - dec_cnt);
		}
		else if(flags.is_uint) {
			unpack_context->item.type = CCPCP_ITEM_UINT;
			unpack_context->item.as.UInt = (uint64_t)mantisa;

		}
		else {
			unpack_context->item.type = CCPCP_ITEM_INT;
			unpack_context->item.as.Int = flags.is_neg? -mantisa: mantisa;
		}
		unpack_context->err_no = CCPCP_RC_OK;
		break;
	}
	default:
		UNPACK_ERROR(CCPCP_RC_MALFORMED_INPUT, "Invalid character.");
	}

	ccpcp_unpack_context_update_container_state(unpack_context);
}

void ccpon_pack_field_delim(ccpcp_pack_context *pack_context, bool is_first_field, bool is_oneliner)
{
	if(!is_first_field)
		ccpcp_pack_copy_bytes(pack_context, ",", 1);
	indent_element(pack_context, is_oneliner, is_first_field);
}

void ccpon_pack_key_val_delim(ccpcp_pack_context *pack_context)
{
	ccpcp_pack_copy_bytes(pack_context, ":", 1);
}
