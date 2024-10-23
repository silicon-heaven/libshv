#pragma once
#ifdef __has_include
	#if __has_include(<unistd.h>)
		#include <unistd.h>
	#endif
#endif

#ifndef __ssize_t_defined
#include <stdint.h>
typedef intmax_t ssize_t;
#endif
