#include "expat/expat.h"
#ifndef __PRINTSVR_EXPAT__H__
#define __PRINTSVR_EXPAT__H__
#ifdef __cplusplus
extern "C"
{
#endif
	typedef struct _printsvr_exp_st{
		struct _exp_st _base;
		int   indlen;
		uchar checksum;
	}printsvr_exp_st, *printsvr_exp_t;

#define _BASE(p) p->_base

	printsvr_exp_t printsvr_exp_new(void* param);
	void          printsvr_exp_free(printsvr_exp_t);

#ifdef __cplusplus
}
#endif
#endif

