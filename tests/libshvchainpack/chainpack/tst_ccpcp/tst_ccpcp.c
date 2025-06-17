#include <ccpcp.h>
#include <ccpon.h>
#include <cchainpack.h>
#include <ccpcp_convert.h>

#define _XOPEN_SOURCE
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <math.h>
static bool o_silent = true;

static void binary_dump(const char *buff, ssize_t len)
{
	for (int i = 0; i < len; ++i) {
		char u = buff[i];
		//ret += std::to_string(u);
		if(i > 0)
			printf("|");
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			printf((u & (((uint8_t)128) >> j))? "1": "0");
		}
	}
}

/*
static inline char hex_nibble(char i)
{
	if(i < 10)
		return '0' + i;
	return 'A' + (i - 10);
}

static void hex_dump(const uint8_t *buff, int len)
{
	for (int i = 0; i < len; ++i) {
		char h = buff[i] / 16;
		char l = buff[i] % 16;
		printf("%c", hex_nibble(h));
		printf("%c", hex_nibble(l));
	}
}
*/
int test_pack_double(double d, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpcp_pack_context ctx;
	ccpcp_pack_context_init(&ctx, buff, BUFFLEN, NULL);
	ccpon_pack_double(&ctx, d);
	*ctx.current = '\0';
	if(strcmp(buff, res)) {
		printf("FAIL! pack double %lg have: '%s' expected: '%s'\n", d, buff, res);
		assert(false);
	}
	return 0;
}

int test_pack_int(long i, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpcp_pack_context ctx;
	ccpcp_pack_context_init(&ctx, buff, BUFFLEN, NULL);
	ccpon_pack_int(&ctx, i);
	*ctx.current = '\0';
	if(strcmp(buff, res)) {
		printf("FAIL! pack signed %ld have: '%s' expected: '%s'\n", i, buff, res);
		assert(false);
	}
	return 0;
}

int test_pack_uint(unsigned long i, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpcp_pack_context ctx;
	ccpcp_pack_context_init(&ctx, buff, BUFFLEN, NULL);
	ccpon_pack_uint(&ctx, i);
	*ctx.current = '\0';
	if(strcmp(buff, res)) {
		printf("FAIL! pack unsigned %lu have: '%s' expected: '%s'\n", i, buff, res);
		assert(false);
	}
	return 0;
}

void test_pack_decimal(int mantisa, int exponent, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpcp_pack_context ctx;
	ccpcp_pack_context_init(&ctx, buff, BUFFLEN, NULL);
	ccpon_pack_decimal(&ctx, mantisa, exponent);
	*ctx.current = '\0';
	if(!o_silent) {
		printf("pack decimal mantisa %d, exponent %d have: '%s' expected: '%s'\n", mantisa, exponent, buff, res);
	}
	if(strcmp(buff, res)) {
		printf("FAIL! pack decimal mantisa %d, exponent %d have: '%s' expected: '%s'\n", mantisa, exponent, buff, res);
		assert(false);
	}
}

int test_unpack_number(const char *str, int expected_type, double expected_val)
{
	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	ccpcp_container_stack stack;
	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	unsigned long n = strlen(str);
	ccpcp_unpack_context ctx;
	ccpcp_unpack_context_init(&ctx, (const uint8_t*)str, n, NULL, &stack);

	ccpon_unpack_next(&ctx);
	if(ctx.err_no != CCPCP_RC_OK) {
		printf("FAIL! unpack number str: '%s' error: %d\n", str, ctx.err_no);
		assert(false);
	}
	if((int)ctx.item.type != expected_type) {
		printf("FAIL! unpack number str: '%s' have type: %d expected type: %d\n", str, ctx.item.type, expected_type);
		assert(false);
	}
	switch(ctx.item.type) {
	case CCPCP_ITEM_DECIMAL: {
		double d = ccpcp_decimal_to_double(ctx.item.as.Decimal.mantisa, ctx.item.as.Decimal.exponent);
		if(fabs(d - expected_val) < 1e-16)
			return 0;
		printf("FAIL! unpack decimal number str: '%s' have: %lg expected: %lg difference: %lg\n", str, d, expected_val, d-expected_val);
		assert(false);
		break;
	}
	case CCPCP_ITEM_DOUBLE: {
		double d = ctx.item.as.Double;
		double epsilon = 1e-10;
		double diff = d - expected_val;
		if(diff > -epsilon && diff < epsilon)
			return 0;
		printf("FAIL! unpack double number str: '%s' have: %lg expected: %lg difference: %lg\n", str, d, expected_val, (d-expected_val));
		assert(false);
		break;
	}
	case CCPCP_ITEM_INT: {
		int64_t d = ctx.item.as.Int;
		if(d == (int64_t)expected_val)
			return 0;
		printf("FAIL! unpack int number str: '%s' have: %ld expected: %ld\n", str, d, (int64_t)expected_val);
		assert(false);
		break;
	}
	case CCPCP_ITEM_UINT: {
		uint64_t d = ctx.item.as.UInt;
		if(d == (uint64_t)expected_val)
			return 0;
		printf("FAIL! unpack int number str: '%s' have: %lu expected: %lu\n", str, d, (uint64_t)expected_val);
		assert(false);
		break;
	}
	default:
		printf("FAIL! unpack number str: '%s' unexpected type: %d\n", str, ctx.item.type);
		assert(false);
	}
}

int test_unpack_datetime(const char *str, int add_msecs, int expected_utc_offset_min)
{
#ifdef _WIN32
	return 0;
#else
	ccpcp_unpack_context ctx;
	unsigned long n = strlen(str);
	ccpcp_unpack_context_init(&ctx, (const uint8_t*)str, n, NULL, NULL);

	struct tm tm;
	int has_T = 0;
	for (unsigned long i = 0; i < n; ++i) {
		if(str[i] == 'T') {
			has_T = 1;
			break;
		}
	}
	const char *dt_format = has_T? "%Y-%m-%dT%H:%M:%S": "%Y-%m-%d %H:%M:%S";
	/*char *rest = */strftime(str+2, 100, dt_format, &tm);
	//printf("\tstr: '%s' year: %d month: %d day: %d rest: '%s'\n", str , tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday , rest);
	int64_t expected_epoch_msec = timegm(&tm);
	expected_epoch_msec *= 1000;
	expected_epoch_msec += add_msecs;

	ccpon_unpack_next(&ctx);
	if(ctx.err_no != CCPCP_RC_OK) {
		printf("FAIL! unpack date time str: '%s' error: %d\n", str, ctx.err_no);
		assert(false);
	}
	if(ctx.item.type != CCPCP_ITEM_DATE_TIME) {
		printf("FAIL! unpack number str: '%s' have type: %d expected type: %d\n", str, ctx.item.type, CCPCP_ITEM_DATE_TIME);
		assert(false);
	}
	ccpcp_date_time *dt = &ctx.item.as.DateTime;
	if(dt->msecs_since_epoch == (expected_epoch_msec - 60000 * expected_utc_offset_min) && dt->minutes_from_utc == expected_utc_offset_min)
		return 0;
	printf("FAIL! unpack datetime str: '%s' have: %ld msec + %d utc min expected: %ld msec + %d utc min\n"
		   , str
		   , dt->msecs_since_epoch, dt->minutes_from_utc
		   , expected_epoch_msec - 60000 * expected_utc_offset_min, expected_utc_offset_min);
	assert(false);
#endif
}

int64_t datetime_str_to_msec_utc(const char *str)
{
	ccpcp_unpack_context ctx;
	unsigned long n = strlen(str);
	ccpcp_unpack_context_init(&ctx, (uint8_t*)str, n, NULL, NULL);

	ccpon_unpack_next(&ctx);
	if(ctx.err_no != CCPCP_RC_OK) {
		printf("FAIL! unpack date time str: '%s' error: %d\n", str, ctx.err_no);
		assert(false);
	}
	if(ctx.item.type != CCPCP_ITEM_DATE_TIME) {
		printf("FAIL! unpack number str: '%s' have type: %d expected type: %d\n", str, ctx.item.type, CCPCP_ITEM_DATE_TIME);
		assert(false);
	}
	ccpcp_date_time *dt = &ctx.item.as.DateTime;
	printf("datetime str: '%s' have: %ld msec + %d utc min offset\n" , str , dt->msecs_since_epoch, dt->minutes_from_utc);
	return dt->msecs_since_epoch;
}

static void test_cpon_helper(const char *cpon, const char *ref_cpon, bool compare_chainpack, bool compare_cpon)
{


	if(!o_silent)
		printf("-------------------------------------------------\n");

	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	ccpcp_container_stack stack;

	const char *in_buff = cpon;
	ccpcp_unpack_context in_ctx;

	char out_buff2[1024];
	char out_buff1[1024];
	ccpcp_pack_context out_ctx;

	if(!o_silent)
		printf("Cpon: %s\n", in_buff);

	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context_init(&in_ctx, in_buff, strlen(in_buff), NULL, &stack);
	ccpcp_pack_context_init(&out_ctx, out_buff1, sizeof (out_buff1), NULL);

	//if(!strcmp(cpon, "123n"))
	//	printf("BREAK\n");
	ccpcp_convert(&in_ctx, CCPCP_Cpon, &out_ctx, CCPCP_Cpon);
	*out_ctx.current = 0;
	if(!o_silent)
		printf("1. Cpon->Cpon: %s\n", out_ctx.start);
	//printf("DDD in err: %d out err: %d\n", in_ctx.err_no, out_ctx.err_no);
	assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);
	//assert(!strcmp(in_buff, (const char *)out_ctx.start));

	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, &stack);
	ccpcp_pack_context_init(&out_ctx, out_buff2, sizeof (out_buff2), NULL);

	ccpcp_convert(&in_ctx, CCPCP_Cpon, &out_ctx, CCPCP_ChainPack);
	if(o_silent) {
		printf("%s -> len: %ld data: ", cpon, out_ctx.current - out_ctx.start);
		binary_dump(out_ctx.start, out_ctx.current - out_ctx.start);
		printf("\n");
	}
	else {
		printf("2. Cpon->CPack  len: %ld data: ", out_ctx.current - out_ctx.start);
		binary_dump(out_ctx.start, out_ctx.current - out_ctx.start);
		printf("\n");
	}
	assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);

	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context_init(&in_ctx, out_buff2, sizeof(out_buff2), NULL, &stack);
	ccpcp_pack_context_init(&out_ctx, out_buff1, sizeof (out_buff1), NULL);

	ccpcp_convert(&in_ctx, CCPCP_ChainPack, &out_ctx, CCPCP_ChainPack);
	if(!o_silent) {
		printf("3. CPack->CPack len: %ld data: ", out_ctx.current - out_ctx.start);
		binary_dump(out_ctx.start, out_ctx.current - out_ctx.start);
		printf("\n");
	}
	//printf("DDD %d %d\n", in_ctx.current - in_ctx.start, out_ctx.current - out_ctx.start);
	assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);
	if(compare_chainpack) {
		assert(in_ctx.current - in_ctx.start == out_ctx.current - out_ctx.start);
		assert(!memcmp(in_ctx.start, out_ctx.start, out_ctx.current - out_ctx.start));
	}

	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, &stack);
	ccpcp_pack_context_init(&out_ctx, out_buff2, sizeof (out_buff2), NULL);

	ccpcp_convert(&in_ctx, CCPCP_ChainPack, &out_ctx, CCPCP_Cpon);
	*out_ctx.current = 0;
	if(!o_silent)
		printf("4. CPack->Cpon: %s\n", out_ctx.start);
	assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);

	const char *ref_buff = ref_cpon;
	if(!ref_buff)
		ref_buff = cpon;

	if(compare_cpon) {
		if(strcmp(out_ctx.start, ref_buff)) {
			printf("FAIL! %s vs %s\n", out_ctx.start, ref_buff);
			assert(false);
		}
	}
}

static void test_cpon(const char *cpon, const char *ref_cpon)
{
	test_cpon_helper(cpon, ref_cpon, true, true);
}

static void test_cpon2(const char *cpon, const char *ref_cpon)
{
	test_cpon_helper(cpon, ref_cpon, false, true);
}
/*
static void test_cpon3(const char *cpon, const char *ref_cpon)
{
	test_cpon_helper(cpon, ref_cpon, true, false);
}
*/
#define INIT_OUT_BUFF() \
	char out_buff1[1024]; \
	ccpcp_pack_context out_ctx; \
	ccpcp_pack_context_init(&out_ctx, out_buff1, sizeof (out_buff1), NULL); \

#define INIT_OUT_BUFFS() \
	char out_buff1[1024]; \
	ccpcp_pack_context out_ctx; \
	ccpcp_pack_context_init(&out_ctx, out_buff1, sizeof (out_buff1), NULL); \
	char out_buff2[1024];

void test_vals()
{
	printf("------------- NULL \n");
	{
		INIT_OUT_BUFFS();
		ccpon_pack_null(&out_ctx);
		*out_ctx.current = 0;
		snprintf(out_buff2, sizeof (out_buff2), "null");
		test_cpon((const char *)out_ctx.start, out_buff2);
	}
	printf("------------- BOOL \n");
	for (unsigned n = 0; n < 2; ++n) {
		INIT_OUT_BUFFS();
		ccpon_pack_boolean(&out_ctx, n);
		*out_ctx.current = 0;
		snprintf(out_buff2, sizeof (out_buff2), n? "true": "false");
		test_cpon((const char *)out_ctx.start, out_buff2);
	}
	printf("------------- tiny uint \n");
	for (unsigned n = 0; n < 64; ++n) {
		INIT_OUT_BUFFS();
		ccpon_pack_uint(&out_ctx, n);
		*out_ctx.current = 0;
		snprintf(out_buff2, sizeof (out_buff2), "%du", n);
		test_cpon((const char *)out_ctx.start, out_buff2);
	}
	printf("------------- uint \n");
	for (unsigned i = 0; i < sizeof(uint64_t); ++i) {
		for (unsigned j = 0; j < 3; ++j) {
			uint64_t n = (uint64_t)1 << (i*8 + j*3+1);
			INIT_OUT_BUFFS();
			ccpon_pack_uint(&out_ctx, n);
			*out_ctx.current = 0;
			snprintf(out_buff2, sizeof (out_buff2), "%lluu", (unsigned long long)n);
			test_cpon((const char *)out_ctx.start, out_buff2);
		}
	}
	{
		uint64_t n = 0;
		n = ~n;
		INIT_OUT_BUFFS();
		ccpon_pack_uint(&out_ctx, n);
		*out_ctx.current = 0;
		snprintf(out_buff2, sizeof (out_buff2), "%lluu", (unsigned long long)n);
		test_cpon((const char *)out_ctx.start, out_buff2);
	}
	printf("------------- tiny int \n");
	for (int n = 0; n < 64; ++n) {
		INIT_OUT_BUFFS();
		ccpon_pack_int(&out_ctx, n);
		*out_ctx.current = 0;
		snprintf(out_buff2, sizeof (out_buff2), "%i", n);
		test_cpon((const char *)out_ctx.start, out_buff2);
	}
	printf("------------- int \n");
	for (int sig = 1; sig >= -1; sig-=2) {
		for (unsigned i = 0; i < sizeof(int64_t); ++i) {
			for (unsigned j = 0; j < 3; ++j) {
				int64_t n = sig * ((int64_t)1 << (i*8 + j*2+2));
				INIT_OUT_BUFFS();
				ccpon_pack_int(&out_ctx, n);
				*out_ctx.current = 0;
				snprintf(out_buff2, sizeof (out_buff2), "%lli", (long long)n);
				test_cpon((const char *)out_ctx.start, out_buff2);
			}
		}
	}
	for (int i = 0; i < 2; i++) {
		int64_t n = 1;
		n <<= sizeof (n) * 8 - 1; // -MIN INT
		if(i)
			n = ~n; // MAX INT
		else
			n = n + 1; // MIN INT + 1
		INIT_OUT_BUFFS();
		ccpon_pack_int(&out_ctx, n);
		*out_ctx.current = 0;
		snprintf(out_buff2, sizeof (out_buff2), "%lli", (long long)n);
		test_cpon((const char *)out_ctx.start, out_buff2);
	}
	printf("------------- decimal \n");
	{
		int mant = -123456;
		int exp_min = 1;
		int exp_max = 16;
		int step = 1;
		for (int exp = exp_min; exp <= exp_max; exp += step) {
			INIT_OUT_BUFF();
			ccpon_pack_decimal(&out_ctx, mant, exp);
			*out_ctx.current = 0;
			test_cpon((const char *)out_ctx.start, NULL);
		}
	}
	{
		printf("------------- double\n");
		{
			double n_max = 1000000.;
			double n_min = -1000000.;
			double step = (n_max - n_min) / 100.1;
			for (double n = n_min; n < n_max; n += step) {
				INIT_OUT_BUFF();
				cchainpack_pack_double(&out_ctx, n);
				assert(out_ctx.current - out_ctx.start == sizeof(double) + 1);
				ccpcp_unpack_context in_ctx;
				ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, NULL);
				cchainpack_unpack_next(&in_ctx);
				assert(in_ctx.current - in_ctx.start == sizeof(double) + 1);
				assert(n == in_ctx.item.as.Double);
			}
		}
		{
			double n_max = DBL_MAX;
			double n_min = DBL_MIN;
			double step = -1.23456789e10;
			//qDebug() << n_min << " - " << n_max << ": " << step << " === " << (n_max / step / 10);
			for (double n = n_min; n < n_max / -step / 10; n *= step) {
				INIT_OUT_BUFF();
				printf("%g\n", n);
				cchainpack_pack_double(&out_ctx, n);
				ccpcp_unpack_context in_ctx;
				ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, NULL);
				cchainpack_unpack_next(&in_ctx);
				//if(n > -100 && n < 100)
				//	qDebug() << n << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				assert(in_ctx.current - in_ctx.start == sizeof(double) + 1);
				assert(n == in_ctx.item.as.Double);
			}
		}
	}
	printf("------------- datetime \n");
	{
		const char *cpons[] = {
			//"d\"\"", NULL,
			"d\"2018-02-02T0:00:00.001\"", "d\"2018-02-02T00:00:00.001Z\"",
			"d\"2018-02-02 01:00:00.001+01\"", "d\"2018-02-02T01:00:00.001+01\"",
			"d\"2018-12-02 0:00:00\"", "d\"2018-12-02T00:00:00Z\"",
			"d\"2041-03-04 0:00:00-1015\"", "d\"2041-03-04T00:00:00-1015\"",
			"d\"2041-03-04T0:00:00.123-1015\"", "d\"2041-03-04T00:00:00.123-1015\"",
			"d\"1970-01-01T0:00:00\"", "d\"1970-01-01T00:00:00Z\"",
			"d\"2017-05-03T5:52:03\"", "d\"2017-05-03T05:52:03Z\"",
			"d\"2017-05-03T15:52:03.923Z\"", NULL,
			"d\"2017-05-03T15:52:03.000-0130\"", "d\"2017-05-03T15:52:03-0130\"",
			"d\"2017-05-03T15:52:03.923+00\"", "d\"2017-05-03T15:52:03.923Z\"",
		};
		for (size_t i = 0; i < sizeof (cpons) / sizeof(char*); i+=2) {
			const char *cpon = cpons[i];
			INIT_OUT_BUFF();
			test_cpon(cpon, cpons[i+1]);
		}
	}
	printf("------------- cstring \n");
	{
		const char *cpons[] = {
			"", NULL,
			"hello", NULL,
			"\t\r", "\"\\t\\r\"",
			"\\0", "\"\\\\0\"",
			"1\t\r\n\b", "\"1\\t\\r\\n\\b\"",
			"escaped zero \\0 here \t\r\n\b", "\"escaped zero \\\\0 here \\t\\r\\n\\b\"",
		};
		for (size_t i = 0; i < sizeof (cpons) / sizeof(char*); i+=2) {
			const char *cpon = cpons[i];
			INIT_OUT_BUFF();
			ccpon_pack_string_terminated(&out_ctx, cpon);
			*out_ctx.current = 0;
			test_cpon2(out_ctx.start, cpons[i+1]);
		}
	}
}

void test_cpons()
{
	const char* cpons[] = {
		"null", NULL,
		//"@", "null",
		"0.", NULL,
		"0.0", NULL,
		"223.", NULL,
		"2.30", NULL,
		"12.3e-10", "123e-11",
		"-0.00012", "-12e-5",
		"-1234567890.", "-1234567890.",
		"\"abc\"", NULL,
		"x\"abcd\"", "b\"\\xab\\xcd\"",
		"b\"ab\\xcd\"", NULL,
		"[]", NULL,
		"[1]", NULL,
		"[1,]", "[1]",
		"[1,2,3]", NULL,
		"[[]]", NULL,
		"i{1:2}", NULL,
		"i{\n\t1: \"bar\",\n\t345u : \"foo\",\n}", "i{1:\"bar\",345u:\"foo\"}",
		"[1u,{\"a\":1},2.30]", NULL,
		"<>1", NULL,
		"<1:2>3", NULL,
		"[1,<7:8>9]", NULL,
		"<8:3u>i{2:[[\".broker\",<1:2>true]]}", NULL,
		"<1:2,\"foo\":\"bar\">i{1:<7:8>9}", NULL,
		"<1:2,\"foo\":<5:6>\"bar\">[1u,{\"a\":1},2.30]", NULL,
		"i{1:2 // comment to end of line\n}", "i{1:2}",
		"/*comment 1*/{ /*comment 2*/ \n"
		"\t\"foo\"/*comment \"3\"*/: \"bar\", //comment to end of line\n"
		"\t\"baz\" : 1,\n"
		"/*\n"
		"\tmultiline comment\n"
		"\t\"baz\" : 1,\n"
		"\t\"baz\" : 1, // single inside multi\n"
		"*/\n"
		"}", "{\"foo\":\"bar\",\"baz\":1}",
		"a[1,2,3]", "[1,2,3]", // unsupported array type
		"<1:2>[3,<4:5>6]", NULL,
		"<4:\"svete\">i{2:<4:\"svete\">[0,1]}", NULL,
		//"d\"\"", "d\"1970-01-01T00:00:00Z\"",
		"d\"2018-02-02T00:00:00Z\"", NULL,
		"d\"2017-05-03T11:30:00-0700\"", "d\"2017-05-03T11:30:00-07\"",
		"d\"2017-05-03T11:30:12.345+01\"", NULL,
	};
	size_t cpons_cnt = sizeof (cpons) / sizeof (char*);
	for(size_t i = 0; i < cpons_cnt; i += 2) {
		test_cpon(cpons[i], cpons[i+1]);
	}
}

int main(int argc, const char * argv[])
{
	for (int i = 0; i < argc; ++i) {
		if(!strcmp(argv[i], "-v")) {
			o_silent = false;
		}
	}

	for (int i = CP_Null; i <= CP_CString; ++i) {
		printf("%d %x ", i, i);
		binary_dump((const char*)&i, 1);
		printf(" %s\n", cchainpack_packing_schema_name(i));
	}
	for (int i = CP_FALSE; i <= CP_TERM; ++i) {
		printf("%d %x ", i, i);
		binary_dump((const char*)&i, 1);
		printf(" %s\n", cchainpack_packing_schema_name(i));
	}

	printf("\nC Cpon test started.\n");

	test_vals();

	test_pack_int(1, "1");
	test_pack_int(-1234567890l, "-1234567890");

	test_pack_uint(1, "1u");
	test_pack_uint(1234567890l, "1234567890u");

	test_pack_double(0., "0.");
	test_pack_double(0.1, "0.1");
	test_pack_double(1, "1.");
	test_pack_double(1.2, "1.2");
	test_pack_double(1e-1, "0.1");
	test_pack_double(1.23e-1, "0.123");
	test_pack_double(1.23e-2, "1.23e-2");
	test_pack_double(1.23e3, "1230.");
	test_pack_double(1.23e6, "1230000.");
	test_pack_double(1.23e7, "1.23e7");
	test_pack_double(1e8, "1e8");
	test_pack_double(-1e8, "-1e8");
	test_pack_double(-123456789e-8, "-1.234567");
	test_pack_double(-123456789e-9, "-0.123456");
	test_pack_double(-123456789e-10, "-1.234567e-2");
	test_pack_double(123456789., "1.234567e8");
	test_pack_double(123456789e1, "1.234567e9");
	test_pack_double(123456789e2, "1.234567e10");

	test_unpack_number("1", CCPCP_ITEM_INT, 1);
	test_unpack_number("123u", CCPCP_ITEM_UINT, 123);
	test_unpack_number("+123", CCPCP_ITEM_INT, 123);
	test_unpack_number("-1", CCPCP_ITEM_INT, -1);
	test_unpack_number("0x1", CCPCP_ITEM_INT, 1);
	test_unpack_number("-0x1", CCPCP_ITEM_INT, -1);
	test_unpack_number("0xaB", CCPCP_ITEM_INT, 10*16+11);
	test_unpack_number("0aB", CCPCP_ITEM_INT, 0);
	test_unpack_number("00xaB", CCPCP_ITEM_INT, 0);
	test_unpack_number("1xaB", CCPCP_ITEM_INT, 1);
	test_unpack_number("1u", CCPCP_ITEM_UINT, 1);
	test_unpack_number("0x1a2b3c4d", CCPCP_ITEM_INT, 439041101);
	test_unpack_number("0.1", CCPCP_ITEM_DECIMAL, 0.1);
	test_unpack_number("0.", CCPCP_ITEM_DECIMAL, 0);
	test_unpack_number("1.", CCPCP_ITEM_DECIMAL, 1);
	test_unpack_number("0xa1.", CCPCP_ITEM_DECIMAL, 10*16+1);
	test_unpack_number("1e2", CCPCP_ITEM_DECIMAL, 100);
	test_unpack_number("1.e2", CCPCP_ITEM_DECIMAL, 100);
	test_unpack_number("1.23", CCPCP_ITEM_DECIMAL, 1.23);
	test_unpack_number("1.23e4", CCPCP_ITEM_DECIMAL, 1.23e4);
	test_unpack_number("-21.23e-4", CCPCP_ITEM_DECIMAL, -21.23e-4);
	test_unpack_number("-0.567e-3", CCPCP_ITEM_DECIMAL, -0.567e-3);
	test_unpack_number("1.23n", CCPCP_ITEM_DECIMAL, 1.23);

	test_unpack_datetime("d\"2018-02-02T0:00:00.001\"", 1, 0);
	test_unpack_datetime("d\"1970-01-01 00:00:00-01\"", 0, -60);
	test_unpack_datetime("d\"2018-02-02 01:00:00.001+01\"", 1, 60);
	test_unpack_datetime("d\"2018-12-02 0:00:00\"", 0, 0);
	test_unpack_datetime("d\"2041-03-04 0:00:00-1015\"", 0, -(10*60+15));
	test_unpack_datetime("d\"2041-03-04 0:00:00.123-1015\"", 123, -(10*60+15));
	test_unpack_datetime("d\"1970-01-01 0:00:00\"", 0, 0);
	test_unpack_datetime("d\"2017-05-03 5:52:03\"", 0, 0);
	test_unpack_datetime("d\"2017-05-03T15:52:03.923Z\"", 923, 0);
	test_unpack_datetime("d\"2017-05-03T15:52:03.000-0130\"", 0, -(1*60+30));
	test_unpack_datetime("d\"2017-05-03T15:52:03.923+00\"", 923, 0);

	if(datetime_str_to_msec_utc("d\"2017-05-03T18:30:00Z\"") != datetime_str_to_msec_utc("d\"2017-05-03T22:30:00+04\"")) {
		printf("FAIL! UTC offsets shall be the same\n");
		assert(false);
	}
	if(datetime_str_to_msec_utc("d\"2017-05-03T11:30:00-0700\"") != datetime_str_to_msec_utc("d\"2017-05-03T15:00:00-0330\"")) {
		printf("FAIL! UTC offsets shall be the same\n");
		assert(false);
	}

	test_cpons();

	test_pack_decimal(0, 0, "0.");
	test_pack_decimal(83, 1, "830.");
	test_pack_decimal(83, 0, "83.");
	test_pack_decimal(83, -1, "8.3");
	test_pack_decimal(83, -2, "0.83");
	test_pack_decimal(83, -3, "0.083");

	printf("\nPASSED\n");

}
