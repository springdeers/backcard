#pragma  once
#include "expat/expat.h"

struct OVERLAPPED;

#ifdef __cplusplus
extern "C"{
#endif
	enum{ eCode_ok,eCode_err };
	enum { eState_new, eState_open, eState_init }; //for comsess_st.state

	typedef int(*onreadcb_t)(unsigned char* rbuffer, int rlen, int code,void* param);
	typedef onreadcb_t onwritecb_t;

	typedef struct _comsess_st{
		OVERLAPPED osreader;
		unsigned char rbuffer[1024];
		DWORD  readlen;
		BOOL   ifreadwating;

		OVERLAPPED oswrite;
		unsigned char wbuffer[1024];
		DWORD  writelen;

		int state;
		int type;
	}comsess_st, *comsess_t;

	typedef struct _com_st{
		comsess_st sess;
		void*      hcom;

		int    comno;
		int    baudrate;
		int    ifparity;
		int    paritype;
		int    databits;
		int    stopbits;
		_exp_t expat;
		onreadcb_t _cb;
	}com_st, *com_t;

	com_t com_new();

	int   com_open(com_t com, int comno, int baud, int ifparity, int paritype, int databits, int stopbits, onreadcb_t rcb);

	void  com_close(com_t com);

	void  com_free(com_t com);

	void  com_app(com_t com, onreadcb_t rcb, onwritecb_t wcb);

	int   writecom(com_t com, char wbuffer[], int wlen);//blocking write..

	int   com_run(com_t com[], int comnum, unsigned int waitimeout_ms);

	int   com_sess_init(comsess_t com);

#ifdef __cplusplus
}
#endif