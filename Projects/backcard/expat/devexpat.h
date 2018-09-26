#include "expat.h"
#ifndef __DEVEXPAT__H__
#define __DEVEXPAT__H__

#ifdef __cplusplus
extern "C"{
#endif

	typedef struct _dev_exp_st{
		struct _exp_st _base;
		int   indlen;
		uchar checksum;
	}dev_exp_st, *dev_exp_t;

#define _BASE(p) p->_base

	dev_exp_t dev_exp_new(void* param);
	void      dev_exp_free(dev_exp_t);


#ifdef __cplusplus
}
#endif

#endif

