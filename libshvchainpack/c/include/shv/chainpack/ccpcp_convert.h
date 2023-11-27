#ifndef C_CPCP_CONVERT_H
#define C_CPCP_CONVERT_H

#include "ccpcp.h"

#ifdef __cplusplus
extern "C" {
#endif

void ccpcp_convert(ccpcp_unpack_context* in_ctx, ccpcp_pack_format in_format, ccpcp_pack_context* out_ctx, ccpcp_pack_format out_format);

#ifdef __cplusplus
}
#endif

#endif
