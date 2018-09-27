#ifndef __TOOL__H__
#define __TOOL__H__
#include "mio/mio.h"

#define _arraysize(_arr__) (sizeof(_arr__)/sizeof(_arr__[0]))

#define NULL     0
#define HEADLEN  12
#define FALSE    0
#define TRUE     1

enum {
	PACK_UNSEARCHED = 0,
	PACK_SEARCHED,
	PACK_STILLDATA,
	PACK_UNFINISHED,
	PACK_FINISHED,
};

enum {
	SEARCHMACHINE_NULL = 0,
	SEARCH_GETHEAD,
	SEARCH_GETDATA,
	SEARCH_GETCHECK,
	SEARCHMACHINE_BEGININD,
	SEARCHMACHINE_IDENTIFIER,
	SEARCHMACHINE_LENGTH,
	SEARCHMACHINE_BINARYCHECK,
};

#define ISLCHAR(w)     (w>='a'&&w<='z')
#define ISHCHAR(w)     (w>='A'&&w<='Z')
#define ISDIGIT(w)     (w>='0'&&w<='9')
#define ISCHAR(w)       (ISDIGIT(w)|| ISLCHAR(w)||ISHCHAR(w))

#define NULLReturn(condition , rtnval)      {if((condition) == NULL)  return rtnval;}
#define FALSEReturn(condition, rtnval)      {if((condition) == FALSE) return rtnval;}
#define TRUEReturn(condition , rtnval)      {if((condition) == TRUE)  return rtnval;}

typedef struct _SMsgTitle_st{
	int id;
	char title[32];
}SMsgTitle_st, *SMsgTitle_t;


mio_fd_t mio_udp_client(int maxbufferszie);
void     mio_udp_close(mio_fd_t fd);

typedef struct _io_throughout_check_st{
	int count;
	int memcnt;
	time_t tick;
	int64_t memtick;
	int timeouts;
	int throughout;
}io_throughout_check_st,*io_throughout_check_t;

io_throughout_check_t io_throughout_check_new(int timeouts);
int                   io_throughout_check_rslt(io_throughout_check_t checker);
BOOL                  io_throughout_check(io_throughout_check_t checker,int count,int printit);
void                  io_throughout_check_free(io_throughout_check_t checker);

typedef struct _ratecheck_st{
	int total;
	int waitmsecs;
	int count;
	long long last;
}ratecheck_st,*ratecheck_t;

ratecheck_t ratecheck_new(int total,int timeouts);
BOOL	ratecheck_add(ratecheck_t,int count);
int		ratecheck_left(ratecheck_t);
void	ratecheck_reset(ratecheck_t);
int		ratecheck_count(ratecheck_t);

int     s_atoi(const char* p);
char*   s_strdup(const char* src);
double  s_atof(const char* str);
void    print_hex(unsigned char buffer[], int len);

typedef struct _mytime_st
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}mytime_st, *mytime_t;

typedef struct _weektime_st{
	unsigned char weekmask;
	int hour;
	int minute;
	int second;
}weektime_st, *weektime_t;

typedef struct _weektimesection_st
{
	unsigned char weekmask;

	int hourStart;
	int minuteStart;
	int secondStart;

	int hourEnd;
	int minuteEnd;
	int secondEnd;
}weektimesection_st, *weektimesection_t;

void	   weektimesection_2str(weektimesection_t t, char* str);
int		   weektime_in(weektime_t t, weektimesection_t s);
int		   mytime_cmp_sub(mytime_t l, mytime_t r);
void       mytime2str(mytime_t t, char* str);
void       str2mytime(char* str, mytime_t t);
weektime_t myweektime(weektime_t wt);
mytime_t   mylocaltime(mytime_t t);
mytime_t   localtime2mytime(struct tm* src, mytime_t dest);
int        str2hms(char* str, int* hour, int* minute, int* second);

#define _DAYSECONDS(hour,minute,second) (hour*3600+minute*60+second)
#define _SOW(day,hour,minute,second) (day*86400+hour*3600 + minute * 60 + second)

typedef struct _vec_st{
	int    cnt;
	int    cap;
	int    size;
	char*  data;
}*vec_t, vec_st;

vec_t   vec_alloc(vec_t, int cnt, int size);
int     vec_add(vec_t, int id, int x, int y, int z);
void    vec_clear(vec_t);
void    vec_free(vec_t);
void*   vec_data(vec_t, int);


#endif