#ifndef __CARD_READERS__H__
#define __CARD_READERS__H__
#include "serialcom.h"
#include "commdef.h"
#include "util/util.h"
#define _max_com_num (64)

#ifdef __cplusplus
extern "C"{
#endif

	typedef struct _coms_st{
		com_t comarr[_max_com_num];
		int   comno[_max_com_num];
		int   baud[_max_com_num];
		int   num;
	}coms_st, *coms_t;

	coms_t coms_get();

	int    coms_openinit(coms_t coms, config_t cfg);

	void   coms_close(coms_t coms);

	void   do_coms_job(coms_t coms, int now);

#ifdef __cplusplus
}
#endif


#endif