#include <shv/chainpack/ccpon.h>
#include <shv/chainpack/cchainpack.h>
#include <shv/chainpack/ccpcp_convert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* const cp2cp_help =
"\n"
"ChainPack to Cpon converter\n"
"\n"
"USAGE:\n"
"-i \"indent_string\"\n"
"	indent Cpon (default is no-indent \"\")\n"
"-t\n"
"	human readable metatypes in Cpon output\n"
"--ip\n"
"	input stream is Cpon (ChainPack otherwise)\n"
"--oc\n"
"	write output in ChainPack (Cpon otherwise)\n"
;
/*
int replace_str(std::string& str, const std::string& from, const std::string& to)
{
	int i = 0;
	for (i = 0; ; ++i) {
		size_t start_pos = str.find(from);
		if(start_pos == std::string::npos)
			break;
		str.replace(start_pos, from.length(), to);
	}
	return i;
}
*/
void help(const char *app_name)
{
	printf("%s%s", app_name, cp2cp_help);
	exit(0);
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static FILE *in_file = NULL;
static char in_buff[1024];
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

size_t unpack_underflow_handler(struct ccpcp_unpack_context *ctx)
{
	size_t n = fread(in_buff, 1, sizeof (in_buff), in_file);
	ctx->start = ctx->current = in_buff;
	ctx->end = ctx->start + n;
	return n;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static FILE *out_file = NULL;
static char out_buff[1024];
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void pack_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint)
{
	(void)size_hint;
	fwrite(out_buff, 1, (size_t)(ctx->current - ctx->start), out_file);
	ctx->start = ctx->current = out_buff;
	ctx->end = ctx->start + sizeof (out_buff);
}

int main(int argc, char *argv[])
{
	const char *o_indent = NULL;
	//bool o_translate_meta_ids = false;
	bool o_cpon_input = false;
	bool o_chainpack_output = false;
	const char *file_name = NULL;

	for(int i=1; i<argc; i++) {
		const char *arg = argv[i];
		if(strcmp(arg, "-h") == 0)
			help(argv[0]);
		if(strcmp(arg, "--help") == 0)
			help(argv[0]);

		if(!strcmp(arg, "-i")) {
			if(i < argc - 1) {
				o_indent = argv[++i];
				if(!strcmp(o_indent, "\\t"))
					o_indent = "\t";
				//fprintf(stderr, "indent: >>>%s<<<\n", o_indent);
			}
		}
		//else if(!strcmp(arg, "-t"))
		//	o_translate_meta_ids = true;
		else if(!strcmp(arg, "--ip"))
			o_cpon_input = true;
		else if(!strcmp(arg, "--oc"))
			o_chainpack_output = true;
		else if(!strcmp(arg, "-h") || !strcmp(arg, "--help"))
			help(argv[0]);
		else
			file_name = arg;
	}

	if(!file_name) {
		in_file = stdin;
	}
	else {
		in_file = fopen(file_name, "rb");
		if(!in_file) {
			fprintf(stderr, "Cannot open '%s' for reading\n", file_name);
			exit(-1);
		}
	}

	out_file = stdout;

	#define STATE_CNT 100
	ccpcp_container_state states[STATE_CNT];
	ccpcp_container_stack stack;
	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context in_ctx;
	ccpcp_unpack_context_init(&in_ctx, in_buff, 0, unpack_underflow_handler, &stack);

	ccpcp_pack_context out_ctx;
	ccpcp_pack_context_init(&out_ctx, out_buff, sizeof (out_buff), pack_overflow_handler);
	out_ctx.cpon_options.indent = o_indent;

	ccpcp_convert(&in_ctx, o_cpon_input? CCPCP_Cpon: CCPCP_ChainPack, &out_ctx, o_chainpack_output? CCPCP_ChainPack: CCPCP_Cpon);

	if(in_ctx.err_no != CCPCP_RC_OK && in_ctx.err_no != CCPCP_RC_BUFFER_UNDERFLOW)
		fprintf(stderr, "Parse error: %d - %s\n", in_ctx.err_no, in_ctx.err_msg);

	return 0;
}
