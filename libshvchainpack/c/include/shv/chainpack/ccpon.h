#ifndef C_CPON_H
#define C_CPON_H

#include "ccpcp.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t ccpon_timegm(struct tm *tm);
void ccpon_gmtime(int64_t epoch_sec, struct tm *tm);

/******************************* P A C K **********************************/

void ccpon_pack_null (ccpcp_pack_context* pack_context);
void ccpon_pack_boolean (ccpcp_pack_context* pack_context, bool b);
void ccpon_pack_int (ccpcp_pack_context* pack_context, int64_t i);
void ccpon_pack_uint (ccpcp_pack_context* pack_context, uint64_t i);
void ccpon_pack_double (ccpcp_pack_context* pack_context, double d);
void ccpon_pack_decimal (ccpcp_pack_context* pack_context, int64_t mantisa, int exponent);
void ccpon_pack_date_time (ccpcp_pack_context* pack_context, int64_t epoch_msecs, int min_from_utc);

typedef enum {CCPON_Auto = 0, CCPON_Always, CCPON_Never} ccpon_msec_policy;
void ccpon_pack_date_time_str (ccpcp_pack_context* pack_context, int64_t epoch_msecs, int min_from_utc, ccpon_msec_policy msec_policy, bool with_tz);

void ccpon_pack_blob (ccpcp_pack_context* pack_context, const uint8_t *s, size_t buff_len);
void ccpon_pack_blob_start (ccpcp_pack_context* pack_context, const uint8_t *buff, size_t buff_len);
void ccpon_pack_blob_cont (ccpcp_pack_context* pack_context, const uint8_t *buff, unsigned buff_len);
void ccpon_pack_blob_finish (ccpcp_pack_context* pack_context);

void ccpon_pack_string (ccpcp_pack_context* pack_context, const char* s, size_t l);
void ccpon_pack_string_terminated (ccpcp_pack_context* pack_context, const char* s);
void ccpon_pack_string_start (ccpcp_pack_context* pack_context, const char*buff, size_t buff_len);
void ccpon_pack_string_cont (ccpcp_pack_context* pack_context, const char*buff, unsigned buff_len);
void ccpon_pack_string_finish (ccpcp_pack_context* pack_context);

void ccpon_pack_list_begin (ccpcp_pack_context* pack_context);
void ccpon_pack_list_end (ccpcp_pack_context* pack_context, bool is_oneliner);

void ccpon_pack_map_begin (ccpcp_pack_context* pack_context);
void ccpon_pack_map_end (ccpcp_pack_context* pack_context, bool is_oneliner);

void ccpon_pack_imap_begin (ccpcp_pack_context* pack_context);
void ccpon_pack_imap_end (ccpcp_pack_context* pack_context, bool is_oneliner);

void ccpon_pack_meta_begin (ccpcp_pack_context* pack_context);
void ccpon_pack_meta_end (ccpcp_pack_context* pack_context, bool is_oneliner);

void ccpon_pack_copy_str (ccpcp_pack_context* pack_context, const char *str);

void ccpon_pack_field_delim (ccpcp_pack_context* pack_context, bool is_first_field, bool is_oneliner);
void ccpon_pack_key_val_delim (ccpcp_pack_context* pack_context);

/***************************** U N P A C K ********************************/
const char* ccpon_unpack_skip_insignificant(ccpcp_unpack_context* unpack_context);
void ccpon_unpack_next (ccpcp_unpack_context* unpack_context);
void ccpon_unpack_date_time(ccpcp_unpack_context *unpack_context, struct tm *tm, int *msec, int *utc_offset);


#ifdef __cplusplus
}
#endif

#endif /* C_CPON_H */
