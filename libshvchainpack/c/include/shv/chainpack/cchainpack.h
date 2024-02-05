#pragma once

#include "ccpcp.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CP_INVALID = -1,

	CP_Null = 128,
	CP_UInt,
	CP_Int,
	CP_Double,
	CP_Bool,
	CP_Blob,
	CP_String, // UTF8 encoded string
	CP_DateTimeEpoch_depr, // deprecated
	CP_List,
	CP_Map,
	CP_IMap,
	CP_MetaMap,
	CP_Decimal,
	CP_DateTime,
	CP_CString,

	CP_FALSE = 253,
	CP_TRUE = 254,
	CP_TERM = 255,
} cchainpack_pack_packing_schema;

const char* cchainpack_packing_schema_name(int sch);

void cchainpack_pack_uint_data(ccpcp_pack_context* pack_context, uint64_t num);

void cchainpack_pack_null (ccpcp_pack_context* pack_context);
void cchainpack_pack_boolean (ccpcp_pack_context* pack_context, bool b);
void cchainpack_pack_int (ccpcp_pack_context* pack_context, int64_t i);
void cchainpack_pack_uint (ccpcp_pack_context* pack_context, uint64_t i);
void cchainpack_pack_double (ccpcp_pack_context* pack_context, double d);
void cchainpack_pack_decimal (ccpcp_pack_context* pack_context, int64_t i, int exponent);
void cchainpack_pack_date_time (ccpcp_pack_context* pack_context, int64_t epoch_msecs, int min_from_utc);

void cchainpack_pack_blob (ccpcp_pack_context* pack_context, const uint8_t *buff, size_t buff_len);
void cchainpack_pack_blob_start (ccpcp_pack_context* pack_context, size_t string_len, const uint8_t *buff, size_t buff_len);
void cchainpack_pack_blob_cont (ccpcp_pack_context* pack_context, const uint8_t *buff, size_t buff_len);

void cchainpack_pack_string (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_string_start (ccpcp_pack_context* pack_context, size_t string_len, const char* buff, size_t buff_len);
void cchainpack_pack_string_cont (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);

void cchainpack_pack_cstring (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_cstring_terminated (ccpcp_pack_context* pack_context, const char* str);
void cchainpack_pack_cstring_start (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_cstring_cont (ccpcp_pack_context* pack_context, const char* buff, size_t buff_len);
void cchainpack_pack_cstring_finish (ccpcp_pack_context* pack_context);

void cchainpack_pack_list_begin (ccpcp_pack_context* pack_context);
void cchainpack_pack_map_begin (ccpcp_pack_context* pack_context);
void cchainpack_pack_imap_begin (ccpcp_pack_context* pack_context);
void cchainpack_pack_meta_begin (ccpcp_pack_context* pack_context);

void cchainpack_pack_container_end(ccpcp_pack_context* pack_context);

uint64_t cchainpack_unpack_uint_data(ccpcp_unpack_context *unpack_context, bool *ok);
uint64_t cchainpack_unpack_uint_data2(ccpcp_unpack_context *unpack_context, int *err_code);
void cchainpack_unpack_next (ccpcp_unpack_context* unpack_context);

#ifdef __cplusplus
}
#endif
