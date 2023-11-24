#include "cchainpack.h"
#include "ccpon.h"

#include <assert.h>

void ccpcp_convert(ccpcp_unpack_context* in_ctx, ccpcp_pack_format in_format, ccpcp_pack_context* out_ctx, ccpcp_pack_format out_format)
{
	if(!in_ctx->container_stack) {
		// ccpcp_convert() cannot work without input context container state set
		in_ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
		return;
	}
	bool o_cpon_input = (in_format == CCPCP_Cpon);
	bool o_chainpack_output = (out_format == CCPCP_ChainPack);
	bool meta_just_closed = false;
	do {
		ccpcp_container_state *current_state = ccpcp_unpack_context_top_container_state(in_ctx);
		if(o_cpon_input)
			ccpon_unpack_next(in_ctx);
		else
			cchainpack_unpack_next(in_ctx);
		if(in_ctx->err_no != CCPCP_RC_OK)
			break;

		if(o_chainpack_output) {
		}
		else {
			if(current_state != NULL) {
				bool is_string_concat = 0;
				if(in_ctx->item.type == CCPCP_ITEM_STRING) {
					ccpcp_string *it = &(in_ctx->item.as.String);
					if(it->chunk_cnt > 1) {
						// multichunk string
						// this can happen, when parsed string is greater than unpack_context buffer
						// or escape sequence is encountered
						// concatenate it with previous chunk
						is_string_concat = 1;
					}
				}
				if(!is_string_concat && in_ctx->item.type != CCPCP_ITEM_CONTAINER_END) {
					bool is_first_item = current_state->item_count == 1;
					static const bool IS_ONE_LINER = true;
					switch(current_state->container_type) {
					case CCPCP_ITEM_LIST:
						if(!meta_just_closed)
							ccpon_pack_field_delim(out_ctx, is_first_item, !IS_ONE_LINER);
						break;
					case CCPCP_ITEM_MAP:
					case CCPCP_ITEM_IMAP:
					case CCPCP_ITEM_META: {
						//bool is_key = (current_state->item_count % 2) == 1;
						bool is_val = (current_state->item_count % 2) == 0;
						if(is_val) {
							if(!meta_just_closed)
								ccpon_pack_key_val_delim(out_ctx);
						}
						else {
							// delimite value
							if(!meta_just_closed)
								ccpon_pack_field_delim(out_ctx, is_first_item, !IS_ONE_LINER);
						}
						break;
					}
					default:
						break;
					}
				}
			}
		}
		meta_just_closed = false;
		switch(in_ctx->item.type) {
		case CCPCP_ITEM_INVALID: {
			// end of input
			break;
		}
		case CCPCP_ITEM_NULL: {
			if(o_chainpack_output)
				cchainpack_pack_null(out_ctx);
			else
				ccpon_pack_null(out_ctx);
			break;
		}
		case CCPCP_ITEM_LIST: {
			if(o_chainpack_output)
				cchainpack_pack_list_begin(out_ctx);
			else
				ccpon_pack_list_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_MAP: {
			if(o_chainpack_output)
				cchainpack_pack_map_begin(out_ctx);
			else
				ccpon_pack_map_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_IMAP: {
			if(o_chainpack_output)
				cchainpack_pack_imap_begin(out_ctx);
			else
				ccpon_pack_imap_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_META: {
			if(o_chainpack_output)
				cchainpack_pack_meta_begin(out_ctx);
			else
				ccpon_pack_meta_begin(out_ctx);
			break;
		}
		case CCPCP_ITEM_CONTAINER_END: {
			//ccpcp_container_state *st = ccpcp_unpack_context_closed_container_state(in_ctx);
			if(!current_state) {
				in_ctx->err_no = CCPCP_RC_CONTAINER_STACK_UNDERFLOW;
				return;
			}
			meta_just_closed = (current_state->container_type == CCPCP_ITEM_META);

			if(o_chainpack_output) {
				cchainpack_pack_container_end(out_ctx);
			}
			else {
				switch(current_state->container_type) {
				case CCPCP_ITEM_LIST:
					ccpon_pack_list_end(out_ctx, false);
					break;
				case CCPCP_ITEM_MAP:
					ccpon_pack_map_end(out_ctx, false);
					break;
				case CCPCP_ITEM_IMAP:
					ccpon_pack_imap_end(out_ctx, false);
					break;
				case CCPCP_ITEM_META:
					ccpon_pack_meta_end(out_ctx, false);
					break;
				default:
					// cannot finish Cpon container without container type info
					in_ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
					return;
				}
			}
			break;
		}
		case CCPCP_ITEM_BLOB: {
			ccpcp_string *it = &in_ctx->item.as.String;
			if(o_chainpack_output) {
				if(it->chunk_cnt == 1 && it->last_chunk) {
					// one chunk string with known length is always packed as RAW
					cchainpack_pack_blob(out_ctx, (uint8_t*)it->chunk_start, it->chunk_size);
				}
				else if(it->string_size >= 0) {
					if(it->chunk_cnt == 1)
						cchainpack_pack_blob_start(out_ctx, (size_t)(it->string_size), (uint8_t*)it->chunk_start, (size_t)(it->string_size));
					else
						cchainpack_pack_blob_cont(out_ctx, (uint8_t*)it->chunk_start, it->chunk_size);
				}
				else {
					// cstring
					// not supported, there is nothing like CBlob
				}
			}
			else {
				// Cpon
				if(it->chunk_cnt == 1)
					ccpon_pack_blob_start(out_ctx, (uint8_t*)it->chunk_start, it->chunk_size);
				else
					ccpon_pack_blob_cont(out_ctx, (uint8_t*)it->chunk_start, (unsigned)(it->chunk_size));
				if(it->last_chunk)
					ccpon_pack_blob_finish(out_ctx);
			}
			break;
		}
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &in_ctx->item.as.String;
			if(o_chainpack_output) {
				if(it->chunk_cnt == 1 && it->last_chunk) {
					// one chunk string with known length is always packed as RAW
					cchainpack_pack_string(out_ctx, it->chunk_start, it->chunk_size);
				}
				else if(it->string_size >= 0) {
					if(it->chunk_cnt == 1)
						cchainpack_pack_string_start(out_ctx, (size_t)(it->string_size), it->chunk_start, (size_t)(it->string_size));
					else
						cchainpack_pack_string_cont(out_ctx, it->chunk_start, it->chunk_size);
				}
				else {
					// cstring
					if(it->chunk_cnt == 1)
						cchainpack_pack_cstring_start(out_ctx, it->chunk_start, it->chunk_size);
					else
						cchainpack_pack_cstring_cont(out_ctx, it->chunk_start, it->chunk_size);
					if(it->last_chunk)
						cchainpack_pack_cstring_finish(out_ctx);
				}
			}
			else {
				// Cpon
				if(it->chunk_cnt == 1)
					ccpon_pack_string_start(out_ctx, it->chunk_start, it->chunk_size);
				else
					ccpon_pack_string_cont(out_ctx, it->chunk_start, (unsigned)(it->chunk_size));
				if(it->last_chunk)
					ccpon_pack_string_finish(out_ctx);
			}
			break;
		}
		case CCPCP_ITEM_BOOLEAN: {
			if(o_chainpack_output)
				cchainpack_pack_boolean(out_ctx, in_ctx->item.as.Bool);
			else
				ccpon_pack_boolean(out_ctx, in_ctx->item.as.Bool);
			break;
		}
		case CCPCP_ITEM_INT: {
			if(o_chainpack_output)
				cchainpack_pack_int(out_ctx, in_ctx->item.as.Int);
			else
				ccpon_pack_int(out_ctx, in_ctx->item.as.Int);
			break;
		}
		case CCPCP_ITEM_UINT: {
			if(o_chainpack_output)
				cchainpack_pack_uint(out_ctx, in_ctx->item.as.UInt);
			else
				ccpon_pack_uint(out_ctx, in_ctx->item.as.UInt);
			break;
		}
		case CCPCP_ITEM_DECIMAL: {
			if(o_chainpack_output)
				cchainpack_pack_decimal(out_ctx, in_ctx->item.as.Decimal.mantisa, in_ctx->item.as.Decimal.exponent);
			else
				ccpon_pack_decimal(out_ctx, in_ctx->item.as.Decimal.mantisa, in_ctx->item.as.Decimal.exponent);
			break;
		}
		case CCPCP_ITEM_DOUBLE: {
			if(o_chainpack_output)
				cchainpack_pack_double(out_ctx, in_ctx->item.as.Double);
			else
				ccpon_pack_double(out_ctx, in_ctx->item.as.Double);
			break;
		}
		case CCPCP_ITEM_DATE_TIME: {
			ccpcp_date_time *it = &in_ctx->item.as.DateTime;
			if(o_chainpack_output)
				cchainpack_pack_date_time(out_ctx, it->msecs_since_epoch, it->minutes_from_utc);
			else
				ccpon_pack_date_time(out_ctx, it->msecs_since_epoch, it->minutes_from_utc);
			break;
		}
		}
		{
			ccpcp_container_state *top_state = ccpcp_unpack_context_top_container_state(in_ctx);
			// take just one object from stream
			if(!top_state) {
				if(((in_ctx->item.type == CCPCP_ITEM_STRING || in_ctx->item.type == CCPCP_ITEM_BLOB)  && !in_ctx->item.as.String.last_chunk)
						|| meta_just_closed) {
					// do not stop parsing in the middle of the string
					// or after meta
				}
				else {
					break;
				}
			}
		}
	} while(in_ctx->err_no == CCPCP_RC_OK && out_ctx->err_no == CCPCP_RC_OK);

	if(out_ctx->handle_pack_overflow)
		out_ctx->handle_pack_overflow(out_ctx, 0);
}
