#include "../../libshvchainpack/c/ccpon.h"
#include "../../libshvchainpack/c/ccpcp.h"
#include "../../libshvchainpack/c/cchainpack.h"

#include <node_api.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

typedef struct js_env
{
	napi_env env;
	bool meta_just_closed;
} js_env;

static void js_env_init(js_env *self, napi_env env)
{
	self->env = env;
	self->meta_just_closed = false;
}

bool debug = false;
static void logs(js_env *jsenv, const char * format, ...)
{
	if(!debug)
	  return;
	
	char buffer[1024];

	va_list args;
	va_start (args, format);
	vsnprintf (buffer, sizeof(buffer), format, args);
	//perror (buffer);
	va_end (args);
	napi_value s1;
	napi_create_string_utf8(jsenv->env, buffer, strlen(buffer), &s1);
	napi_value global, console_obj, log;
	napi_get_global(jsenv->env, &global);
	napi_get_named_property(jsenv->env, global, "console", &console_obj);
	napi_get_named_property(jsenv->env, console_obj, "log", &log);
	napi_call_function(jsenv->env, global, log, 1, &s1, NULL);
}

static void log_var(js_env *jsenv, const char * format, napi_value val)
{
	if(!debug)
	  return;
	
	napi_value s1;
	napi_create_string_utf8(jsenv->env, format, strlen(format), &s1);
	napi_value global, console_obj, log;
	napi_get_global(jsenv->env, &global);
	napi_get_named_property(jsenv->env, global, "console", &console_obj);
	// napi_get_named_property(jsenv->env, global, "Array", &array_obj);
	napi_get_named_property(jsenv->env, console_obj, "log", &log);
	// napi_get_named_property(jsenv->env, array_obj, "push", &push);
	// napi_create_array(jsenv->env, &array);
	// napi_call_function(jsenv->env, args, push, 1, &s1, NULL);
	// napi_call_function(jsenv->env, args, push, 1, &val, NULL);
	napi_value args[] = {s1, val};
	napi_call_function(jsenv->env, global, log, 2, args, NULL);
}


#define CALL_SAFE(jsenv, the_call) \
	if ((the_call) != napi_ok) { \
		logs(jsenv, "napi call error %s", #the_call); \
		napi_throw_error(jsenv->env, NULL, "napi call error: " #the_call); \
	}

#define DECLARE_NAPI_PROPERTY(name, func)                                \
  { (name), 0, (func), 0, 0, 0, napi_default, 0 }

#define NAPI_ASSERT_BASE(jsenv, assertion, message, ret_val)               \
  do {                                                                   \
    if (!(assertion)) {                                                  \
      napi_throw_error(                                                  \
          (jsenv->env),                                                         \
        NULL,                                                            \
          "assertion (" #assertion ") failed: " message);                \
      return ret_val;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL on failed assertion.
// This is meant to be used inside napi_callback methods.
#define NAPI_ASSERT(jsenv, assertion, message)                             \
  NAPI_ASSERT_BASE(jsenv, assertion, message, NULL)

// static char in_buff[1024];

// size_t unpack_underflow_handler(struct ccpcp_unpack_context *ctx)
// {
//	size_t n = fread(in_buff, 1, sizeof (in_buff), in_file);
//	ctx->start = ctx->current = in_buff;
//	ctx->end = ctx->start + n;
//	return n;
// }

// static char out_buff[1024];

// void pack_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint)
// {
//	(void)size_hint;
//	fwrite(out_buff, 1, ctx->current - ctx->start, out_file);
//	ctx->start = ctx->current = out_buff;
//	ctx->end = ctx->start + sizeof (out_buff);
//}



typedef struct js_var
{
	napi_value type_str;
	napi_value meta;
	napi_value val;
	napi_value map_key;
} js_var;

static void js_make_null(js_env *jsenv, js_var *self)
{
	napi_get_null(jsenv->env, &(self->val));
}



static void js_var_init(js_env *jsenv, js_var *self)
{
	napi_get_null(jsenv->env, &self->type_str);


	if (!jsenv->meta_just_closed) {
		napi_get_null(jsenv->env, &self->meta);
		logs(jsenv, "Nulling meta");
	} else {
		jsenv->meta_just_closed = false;
	}



	napi_get_null(jsenv->env, &self->val);
	napi_get_null(jsenv->env, &self->map_key);
}

static void js_make_bool(js_env *jsenv, js_var *self, bool b)
{
	js_var_init(jsenv, self);
	napi_get_boolean(jsenv->env, b, &self->val);
}

static void js_make_int(js_env *jsenv, js_var *self, int64_t i)
{
	//logs(jsenv, "%lld", i);
	js_var_init(jsenv, self);
	napi_create_int64(jsenv->env, i, &(self->val));
}

static void js_make_double(js_env *jsenv, js_var *self, double i)
{
	js_var_init(jsenv, self);
	napi_create_double(jsenv->env, i, &self->val);
}

static void js_make_decimal(js_env *jsenv, js_var *self, ccpcp_decimal *it)
{
	js_var_init(jsenv, self);

	double d = it->mantisa;
	d *= pow(10, -it->dec_places);
	napi_create_double(jsenv->env, d, &self->val);
}

static void js_make_datetime(js_env *jsenv, js_var *self, ccpcp_date_time *it)
{
	js_var_init(jsenv, self);
	const char type_cstr[] = "DateTime";
	CALL_SAFE(jsenv, napi_create_string_utf8(jsenv->env, type_cstr, sizeof(type_cstr) - 1, &self->type_str));

	napi_value dt_msec_since_epoch;
	napi_value dt_minutes_from_utc;
	CALL_SAFE(jsenv, napi_create_object(jsenv->env, &self->val));
	CALL_SAFE(jsenv, napi_create_int64(jsenv->env, it->msecs_since_epoch, &dt_msec_since_epoch));
	CALL_SAFE(jsenv, napi_create_int32(jsenv->env, it->minutes_from_utc, &dt_minutes_from_utc));
	CALL_SAFE(jsenv, napi_set_named_property(jsenv->env, self->val, "msec", dt_msec_since_epoch));
	CALL_SAFE(jsenv, napi_set_named_property(jsenv->env, self->val, "tz", dt_minutes_from_utc));
}

static void js_make_string_concat(js_env *jsenv, js_var *self, const char *str, size_t len)
{
	log_var(jsenv, "current result: ", self->val);

	napi_valuetype t;
	napi_typeof(jsenv->env, self->val, &t);
	if (t == napi_null) {
		log_var(jsenv, "string 2:", self->val);
		CALL_SAFE(jsenv, napi_create_string_utf8(jsenv->env, str, len, &self->val));
		log_var(jsenv, "string 3:", self->val);
	}
	else {
		log_var(jsenv, "string 4:", self->val);

		napi_value s1;
		napi_create_string_utf8(jsenv->env, str, len, &s1);
		napi_value global, string_obj, prototype, concat;
		napi_get_global(jsenv->env, &global);
		napi_get_named_property(jsenv->env, global, "String", &string_obj);
		CALL_SAFE(jsenv, napi_get_named_property(jsenv->env, string_obj, "prototype", &prototype));
		napi_get_named_property(jsenv->env, prototype, "concat", &concat);
		napi_call_function(jsenv->env, self->val, concat, 1, &s1, &self->val);
	}
}

static void js_make_list(js_env *jsenv, js_var *self)
{
	js_var_init(jsenv, self);
	napi_create_array(jsenv->env, &(self->val));
	log_var(jsenv, "4: ", self->val);
}

static void js_make_map(js_env *jsenv, js_var *self)
{
	js_var_init(jsenv, self);
	napi_create_object(jsenv->env, &self->val);
}

static void js_make_meta(js_env *jsenv, js_var *self)
{
	js_var_init(jsenv, self);
	napi_create_object(jsenv->env, &self->meta);
}

static void js_add_list_item(js_env *jsenv, js_var *self, js_var *it)
{
	napi_value global, array_obj, prototype, push;
	napi_get_global(jsenv->env, &global);
	CALL_SAFE(jsenv, napi_get_named_property(jsenv->env, global, "Array", &array_obj));
	CALL_SAFE(jsenv, napi_get_named_property(jsenv->env, array_obj, "prototype", &prototype));
	CALL_SAFE(jsenv, napi_get_named_property(jsenv->env, prototype, "push", &push));
	CALL_SAFE(jsenv, napi_call_function(jsenv->env, self->val, push, 1, &it->val, NULL));
}

static void js_make_map_key_concat(js_env *jsenv, js_var *self, const char *str, size_t len)
{

	log_var(jsenv, "string key 1: ", self->map_key);
	napi_valuetype t;
	napi_typeof(jsenv->env, self->map_key, &t);
	if (t == napi_null) {
		napi_create_string_utf8(jsenv->env, str, len, &self->map_key);
		log_var(jsenv, "string key 2: ", self->map_key);
	}
	else {
		napi_value s1;
		napi_create_string_utf8(jsenv->env, str, len, &s1);
		napi_value global, string_obj, prototype, concat;
		napi_get_global(jsenv->env, &global);
		napi_get_named_property(jsenv->env, global, "String", &string_obj);
		CALL_SAFE(jsenv, napi_get_named_property(jsenv->env, string_obj, "prototype", &prototype));
		napi_get_named_property(jsenv->env, prototype, "concat", &concat);
		napi_call_function(jsenv->env, self->map_key, concat, 1, &s1, &self->map_key);
	}
}

static void js_make_map_int_key(js_env *jsenv, js_var *self, int64_t key)
{
	logs(jsenv, "making key %lld ", key);
	napi_create_int64(jsenv->env, key, &(self->map_key));
	// napi_set_property(jsenv->env, &self->val, "val", NULL);
}

static void js_add_map_val(js_env *jsenv, js_var *self, js_var *val)
{
	log_var(jsenv, "add map val", val->val);
	napi_set_property(jsenv->env, self->val, self->map_key, val->val);
	napi_get_null(jsenv->env, &self->map_key);
	log_var(jsenv, "result", self->val);
}

static void js_add_meta_val(js_env *jsenv, js_var *self, js_var *val)
{
	log_var(jsenv, "add meta val", val->val);
	napi_set_property(jsenv->env, self->meta, self->map_key, val->val);
	napi_get_null(jsenv->env, &self->map_key);
	log_var(jsenv, "meta result", self->meta);
}

static void js_wrap_meta(js_env *jsenv, js_var *self)
{
	bool has_meta = false;
	bool has_type = false;
	{
		napi_valuetype t;
		CALL_SAFE(jsenv, napi_typeof(jsenv->env, self->type_str, &t));
		has_type = (t == napi_string);
	}
	{
		napi_valuetype t;
		log_var(jsenv, "is this meta", self->meta);
		CALL_SAFE(jsenv, napi_typeof(jsenv->env, self->meta, &t));
		has_meta = (t == napi_object);
	}
	if(has_type || has_meta) {
		napi_value wrapped_val;
		CALL_SAFE(jsenv, napi_create_object(jsenv->env, &wrapped_val));
		if(has_type)
			CALL_SAFE(jsenv, napi_set_named_property(jsenv->env, wrapped_val, "_type", self->type_str));
		if(has_meta)
			CALL_SAFE(jsenv, napi_set_named_property(jsenv->env, wrapped_val, "_meta", self->meta));
		CALL_SAFE(jsenv, napi_set_named_property(jsenv->env, wrapped_val, "_val", self->val));
		self->val = wrapped_val;
	}
}

js_var *top_js_var(ccpcp_unpack_context *self)
{
	js_var *js_vars = (js_var *)self->custom_context;
	if(self->container_stack && self->container_stack->length > 0) {
		ccpcp_container_state *top_st = self->container_stack->container_states + self->container_stack->length - 1;
		if(top_st && top_st->item_count == 0) {
			if(self->container_stack->length > 0)
				return js_vars + self->container_stack->length - 1;
			else
				return NULL;
		}
		return js_vars + self->container_stack->length;
	}
	return js_vars;
}

js_var *parent_js_var(ccpcp_unpack_context *self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		js_var *js_vars = (js_var *)self->custom_context;
		ccpcp_container_state *top_st = self->container_stack->container_states + self->container_stack->length - 1;
		if(top_st && top_st->item_count == 0) {
			if(self->container_stack->length > 1)
				return js_vars + self->container_stack->length - 2;
			else
				return NULL;
		}
		return js_vars + self->container_stack->length - 1;
	}
	return NULL;
}

napi_value parse_cpon_buffer(napi_env env, napi_callback_info info)
{

	js_env jsenv1;
	js_env *jsenv = &jsenv1;

	js_env_init(jsenv, env);

	size_t argc = 1;
	napi_value args[1];
	CALL_SAFE(jsenv, napi_get_cb_info(jsenv->env, info, &argc, args, NULL, NULL));

	NAPI_ASSERT(jsenv, argc >= 1, "Wrong number of arguments");

	log_var(jsenv, "Incoming value", args[0]);

	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	js_var js_vars[STATE_CNT];
	ccpcp_container_stack stack;
	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context in_ctx;

	char* buffer_data;
	size_t buffer_length;
	CALL_SAFE(jsenv, napi_get_buffer_info(jsenv->env, args[0], &buffer_data, &buffer_length));

	ccpcp_unpack_context_init(&in_ctx, buffer_data, buffer_length, NULL, &stack);
	in_ctx.custom_context = js_vars;


	bool o_cpon_input = true; // (in_format == CCPCP_Cpon);

	js_var_init(jsenv, &js_vars[0]);
//	bool meta_just_closed = false;
//	bool string_concat = false;

	do {

		if(o_cpon_input)
			ccpon_unpack_next(&in_ctx);
		else
			cchainpack_unpack_next(&in_ctx);
		if(in_ctx.err_no != CCPCP_RC_OK)
			break;

		js_var *var = top_js_var(&in_ctx);

		// if (string_concat) {}
		// else {
		// 	js_var_init(jsenv, var, !meta_just_closed);
		// }

//		meta_just_closed = false;

//		string_concat = false;
		switch(in_ctx.item.type) {
		case CCPCP_ITEM_INVALID: {
			// end of input
			break;
		}
		case CCPCP_ITEM_NULL: {
			js_make_null(jsenv, var);
			break;
		}
		case CCPCP_ITEM_LIST: {
			js_make_list(jsenv, var);
			break;
		}
		case CCPCP_ITEM_META:
		{
			js_make_meta(jsenv, var);
			break;
		}

		case CCPCP_ITEM_MAP:
		case CCPCP_ITEM_IMAP: {
			js_make_map(jsenv, var);
			break;
		}
		case CCPCP_ITEM_CONTAINER_END: {
			ccpcp_container_state *st = ccpcp_unpack_context_closed_container_state(&in_ctx);
			if(!st) {
				in_ctx.err_no = CCPCP_RC_CONTAINER_STACK_UNDERFLOW;
				goto error;
			}
			jsenv->meta_just_closed = (st->container_type == CCPCP_ITEM_META);
			if(jsenv->meta_just_closed) {
				logs(jsenv, "meta is closed");
			}
			break;
		}
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &in_ctx.item.as.String;
			if (it->chunk_cnt == 1) {
				js_var_init(jsenv, var);
			}
			js_make_string_concat(jsenv, var, it->chunk_start, it->chunk_size);
			break;
		}
		case CCPCP_ITEM_BOOLEAN: {
			js_make_bool(jsenv, var, in_ctx.item.as.Bool);
			break;
		}
		case CCPCP_ITEM_INT: {
			js_make_int(jsenv, var, in_ctx.item.as.Int);
			break;
		}
		case CCPCP_ITEM_UINT: {
			js_make_int(jsenv, var, (int64_t)in_ctx.item.as.UInt);
			break;
		}
		case CCPCP_ITEM_DECIMAL: {
			// TODO convert decimal to double
			ccpcp_decimal *it = &in_ctx.item.as.Decimal;

			js_make_decimal(jsenv, var, it);
			break;
		}
		case CCPCP_ITEM_DOUBLE: {
			js_make_double(jsenv, var, in_ctx.item.as.Double);
			break;
		}
		case CCPCP_ITEM_DATE_TIME: {
			// TODO convert datetime to string
			ccpcp_date_time *it = &in_ctx.item.as.DateTime;
			js_make_datetime(jsenv, var, it);
			break;
		}
		}

		log_var(jsenv, "1: ", js_vars[0].val);

		bool is_value_complete = false;
		switch(in_ctx.item.type) {
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &(in_ctx.item.as.String);
			is_value_complete = (it->last_chunk);
		}
		case CCPCP_ITEM_LIST:
			break;
		case CCPCP_ITEM_MAP:
		case CCPCP_ITEM_IMAP:
		case CCPCP_ITEM_META:
			break;
		default:
			is_value_complete = true;
			break;
		}

		if(is_value_complete && !jsenv->meta_just_closed)
		{
			logs(jsenv, "wrapping with meta data");
			js_wrap_meta(jsenv, var);
		}

		js_var *parent_var = parent_js_var(&in_ctx);
		ccpcp_container_state *parent_state = ccpcp_unpack_context_parent_container_state(&in_ctx);
		if(parent_state && parent_var) {
			if(is_value_complete) {
				switch(parent_state->container_type) {
				case CCPCP_ITEM_LIST:
					js_add_list_item(jsenv, parent_var, var);
					break;
				case CCPCP_ITEM_MAP:
				case CCPCP_ITEM_IMAP:
				case CCPCP_ITEM_META: {
					bool is_key = (parent_state->item_count % 2);
					if(is_key) {
						if(in_ctx.item.type == CCPCP_ITEM_STRING) {
							ccpcp_string *it = &(in_ctx.item.as.String);
							js_make_map_key_concat(jsenv, parent_var, it->chunk_start, it->chunk_size);
						}
						else if(in_ctx.item.type == CCPCP_ITEM_INT) {
							js_make_map_int_key(jsenv, parent_var, in_ctx.item.as.Int);
						}
						else if(in_ctx.item.type == CCPCP_ITEM_UINT) {
							js_make_map_int_key(jsenv, parent_var, in_ctx.item.as.UInt);
						}
						else {
							// invalid key type
							napi_throw_error(jsenv->env, NULL, "Invalid key type");
						}
					}
					else {
						if(parent_state->container_type == CCPCP_ITEM_META)
							js_add_meta_val(jsenv, parent_var, var);
						else
							js_add_map_val(jsenv, parent_var, var);
					}
					break;
				}
				default:
					break;
				}
			}
		}

		log_var(jsenv, "2: ", js_vars[0].val);

		{
			ccpcp_container_state *top_state = ccpcp_unpack_context_top_container_state(&in_ctx);
			// take just one object from stream
			if(!top_state) {
				if((in_ctx.item.type == CCPCP_ITEM_STRING && !in_ctx.item.as.String.last_chunk)
						|| jsenv->meta_just_closed) {
					// do not stop parsing in the middle of the string
					// or after meta
				}
				else {
					break;
				}
			}
		}
	} while(in_ctx.err_no == CCPCP_RC_OK);
	error:
	return js_vars[0].val;
}

static napi_value Init(napi_env env, napi_value exports) {
	napi_property_descriptor desc = DECLARE_NAPI_PROPERTY("parse_cpon_buffer", parse_cpon_buffer);
	napi_define_properties(env, exports, 1, &desc);
	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
