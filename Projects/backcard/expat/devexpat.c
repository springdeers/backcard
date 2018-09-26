#include <stdlib.h>
#include <string.h>

#include "devexpat.h"
#include "client.h"
#include "p2p.h"
#include "protobuffer.h"

#define NULL     0
#define HEADLEN  2
#define _COUNT   1
#define FALSE    0
#define TRUE     1

static unsigned char g_title[2] ={0x00,0xFF};

#define _nfc_code     0x02   //NFC��ǩ��Ϣ
#define _heart_code   0xFF  //����
#define _score_code   0xFE //�ɼ�����
#define _tfi_code     0x04   //TFI[1]=02��ʾNFC�����ǩ��Ϣ��TFI[1]=��ʾNFC�������

static int search_frame(dev_exp_t explain,uchar*p,int len,int* searchpos);
static int explain(dev_exp_t exp,uchar*frame,int len);

static int istitle(uchar* str)
{
	int rtn = 0;
	int i   = 0 ;

	if(str == 0) return 0;

	for(; i < _COUNT ; i++)
	{
		if(memcmp(g_title,str,2) == 0)		
			break;		
	}

	if(i < _COUNT)
		return 1;
	else
		return 0;
}

dev_exp_t dev_exp_new(void* param)
{
	dev_exp_t rtn = NULL;

	while((rtn = (dev_exp_t)calloc(1,sizeof(dev_exp_st))) == NULL);

	_BASE(rtn).framelen = 0;
	_BASE(rtn).m_cb = NULL;
	_BASE(rtn).searchstatus = SEARCHMACHINE_NULL;
	_BASE(rtn).param = param;
	_BASE(rtn).search= search_frame;
	_BASE(rtn).explain = explain;

	return rtn;
}

void    dev_exp_free(dev_exp_t exp)
{
	if(exp)
		free(exp);
}

//////////////////////////////////////////////////////////////////////////
static int search_frame(dev_exp_t explain,uchar*p,int len,int* searchpos)
{
	int dwCurrPt = 0 ;
	uchar ch;
	_exp_t exp = &_BASE(explain);
	
	for(; dwCurrPt<len; dwCurrPt++)
	{
		ch = p[dwCurrPt];

		switch(exp->searchstatus)
		{
		case SEARCHMACHINE_NULL:	//  0: $
			{
				exp->Head[exp->HeadPtr++]     = ch;
				if(exp->HeadPtr == HEADLEN)
				{//����ͷ��
					exp->Head[HEADLEN] = '\0';
					if(istitle(exp->Head))
					{
						int c = 0;
						exp->searchstatus = SEARCHMACHINE_IDENTIFIER;
						memcpy(exp->frame,exp->Head,HEADLEN);						

						exp->framelen = HEADLEN;
						exp->HeadPtr = 0;
						explain->checksum = 0;
						/*for(; c < HEADLEN ; c++)
							explain->checksum ^= exp->Head[c];*/
						continue;
					}else
					{
						int i = 1;
						for(; i<HEADLEN;i++)
						{
							if(exp->Head[i] == 0x00)
								break;
						}

						if(i<HEADLEN)
						{
							int j = 0 ;
							for(; j < HEADLEN-i ; j++)
								exp->Head[j] = exp->Head[j+i];

							exp->HeadPtr -= i;
						}else
							exp->HeadPtr  = 0;

						exp->searchstatus = SEARCHMACHINE_NULL;
					}
				}
			}
			break;		

		case SEARCHMACHINE_IDENTIFIER:
			{
				exp->frame[exp->framelen] = ch;
				exp->framelen ++;
				if (exp->framelen >=4)
					explain->checksum ^= ch;
				if(exp->framelen == 3){
					if ((unsigned int)exp->frame[2] == 0xFF)
					{
						explain->indlen = (unsigned int)(p[5]) * 16*16 + (unsigned int)(p[6]);
						explain->indlen += 6;
					}
					else
					{
						explain->indlen = (unsigned int)(exp->frame[2]);
						explain->indlen += 4;
					}
					
					if((explain->indlen) >= (UNPACKBUFFSIZE-1)){
						exp->searchstatus = SEARCHMACHINE_NULL;

						continue;
					}
				}

				if(exp->framelen >= UNPACKBUFFSIZE-1)
				{
					exp->searchstatus = SEARCHMACHINE_NULL;
					continue;
				}

				//end part..
				if(exp->framelen == explain->indlen)
				{
					if(explain->checksum != 0){
						exp->searchstatus = SEARCHMACHINE_NULL;
						continue;
					}

					if(dwCurrPt==(len-1))
					{//�ҵ�������һ֡����,�һ�������ֻ��һ֡����
						*searchpos = dwCurrPt;
						dwCurrPt=0;
						exp->searchstatus=SEARCHMACHINE_NULL;//added 2002-09-04 23:06						
						return PACK_FINISHED;
					}else{//����һ֡����ȡ���,��������֡������
						*searchpos = dwCurrPt;
						exp->searchstatus=SEARCHMACHINE_NULL;			
						return PACK_STILLDATA;
					}
				}
			}
			break;
		default://Ĭ��״̬
			exp->searchstatus=SEARCHMACHINE_NULL;
		}
	}
	dwCurrPt=0;
	//һ֡����δ������(SearchMachine=SEARCHMACHINE_LENGTH,�´ν���ʱ����)
	if(exp->searchstatus == SEARCHMACHINE_NULL)
		return PACK_UNSEARCHED;
	else
		return PACK_UNFINISHED;
}

static int   explain(dev_exp_t exp,uchar*frame,int len)
{
	int  msgtype;
	NULLReturn(frame,0);
	NULLReturn(exp,0);

	msgtype= frame[_tfi_code];
	if (msgtype == _nfc_code)
	{
		
	}
	else if (msgtype == _heart_code)
	{
		
	}
	else if (msgtype == _score_code)
	{

	}
	if (_BASE(exp).m_cb) _BASE(exp).m_cb(msgtype, frame, len, exp);

	return 0;
}
