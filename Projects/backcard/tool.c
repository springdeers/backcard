#include "tool.h"
#include <assert.h>

mio_fd_t mio_udp_client(int maxbufferszie)
{
	mio_fd_t rtn = NULL;
#ifdef _WIN32
	rtn = (mio_fd_t)calloc(1,sizeof(struct mio_fd_st));
	rtn->fd = socket(AF_INET, SOCK_DGRAM, 0);
	j_inet_setsock_bufsize(rtn->fd , maxbufferszie);
#endif
	return rtn;

}

void     mio_udp_close(mio_fd_t fd)
{
	if(fd && fd->fd)
		closesocket(fd->fd);
}

io_throughout_check_t io_throughout_check_new(int timeouts)
{
	io_throughout_check_t p ;

	while((p = malloc(sizeof(io_throughout_check_st))) == NULL) Sleep(1);

	p->count = p->memcnt  = 0;
	p->tick  = p->memtick = 0;
	p->timeouts = timeouts;
	p->throughout = 0;

	return p;
}

BOOL io_throughout_check(io_throughout_check_t checker,int count,int printit)
{
	BOOL rslt = FALSE;

	if(checker->memtick == 0 && checker->tick==0)
		checker->memtick = checker->tick = time(NULL);

	checker->count += count;
	checker->tick = time(NULL);

	if(checker->tick - checker->memtick > checker->timeouts)
	{	
		checker->throughout = (checker->count - checker->memcnt)/(checker->tick - checker->memtick);
		checker->memtick = checker->tick  = time(NULL);
		checker->memcnt  = checker->count = 0;
		rslt = TRUE;
	}

	if(rslt && printit)		
		printf("throughoutput: %d bytes/s \n",checker->throughout);

	return rslt;
}

int  io_throughout_check_rslt(io_throughout_check_t checker)
{
	return checker->throughout;
}

void  io_throughout_check_free(io_throughout_check_t checker)
{
	if(checker) free(checker);

}

ratecheck_t ratecheck_new(int total,int timeoutms)
{
	ratecheck_t p;

	while((p = malloc(sizeof(ratecheck_st))) == NULL) Sleep(1);

	p->total = total;
	p->waitmsecs = timeoutms;
	p->count = 0;
	p->last = 0;
	return p;
}

BOOL ratecheck_add(ratecheck_t check,int count)
{
	if(check == NULL) return FALSE;

	check->count += count;
	
	if(check->count >= check->total) return TRUE;

	if( GetTickCount()-check->last >= check->waitmsecs ) return TRUE;

	return FALSE;
}

int  ratecheck_left(ratecheck_t check)
{
	if(check == NULL) return 0;

	return check->total - check->count;
}

void ratecheck_reset(ratecheck_t check)
{
	if(check == NULL) return ;

	check->count = 0;

	check->last = GetTickCount();
}

int  ratecheck_count(ratecheck_t ratechecker)
{
	return ratechecker->count;
}

int s_atoi(const char* p){
	if (p == NULL)
		return 0;
	else
		return atoi(p);
}

char* s_strdup(const char* src){
	if (src != NULL){
		int strl = strlen(src);
		char* p = (char*)malloc(strl + 1);
		strcpy_s(p, strl + 1, src);
		return p;
	}

	return NULL;
}

double s_atof(const char* str){
	if (str == NULL) return 0;
	return atof(str);
}

void weektimesection_2str(weektimesection_t t, char* str)
{
	if (t == NULL || str == NULL) return;
	sprintf(str, "%d %02d:%02d:%02d~%02d:%02d:%02d", t->weekmask, t->hourStart, t->minuteStart, t->secondStart, t->hourEnd, t->minuteEnd, t->secondEnd);
}

unsigned int sow_weektime(weektime_t wt){

	int i = 0;
	int mask = wt->weekmask;

	while ((mask & 0x01) == 0 && i<7){
		i++;
		mask = wt->weekmask >> i;
	}

	return i * 86400 + wt->hour * 3600 + wt->minute * 60 + wt->second;
}

#define _SOW(day,hour,minute,second) (day*86400+hour*3600 + minute * 60 + second)


int weektime_in(weektime_t t, weektimesection_t s)
{
	if (t->weekmask & s->weekmask){
		if (_DAYSECONDS(t->hour, t->minute, t->second) > _DAYSECONDS(s->hourStart, s->minuteStart, s->secondStart)
			&& _DAYSECONDS(t->hour, t->minute, t->second) < _DAYSECONDS(s->hourEnd, s->minuteEnd, s->secondEnd))
			return 1;
		else
			return 0;
	}

	return 0;
}

int mytime_cmp_sub_nodate(mytime_t l, mytime_t r)
{
	if (l == NULL || r == NULL) return FALSE;

	if (l->hour < r->hour)
		return -1;
	else if (l->hour > r->hour)
		return 1;

	if (l->minute < r->minute)
		return -1;
	else if (l->minute > r->minute)
		return 1;

	if (l->second < r->second)
		return -1;
	else if (l->second > r->second)
		return 1;

	return 0;

}

int mytime_cmp_sub_datetime(mytime_t l, mytime_t r)
{
	if (l == NULL || r == NULL) return FALSE;

	if (l->year < r->year)
		return -1;
	else if (l->year > r->year)
		return 1;

	if (l->month < r->month)
		return -1;
	else if (l->month > r->month)
		return 1;

	if (l->day < r->day)
		return -1;
	else if (l->day > r->day)
		return 1;

	if (l->hour < r->hour)
		return -1;
	else if (l->hour > r->hour)
		return 1;

	if (l->minute < r->minute)
		return -1;
	else if (l->minute > r->minute)
		return 1;

	if (l->second < r->second)
		return -1;
	else if (l->second > r->second)
		return 1;

	return 0;

}

int mytime_cmp_sub(mytime_t l, mytime_t r)
{
	if (l == NULL || r == NULL) return FALSE;

	if ((l->year == 0 && l->month == 0 && l->day == 0) || (r->year == 0 && r->month == 0 && r->day == 0)){
		return mytime_cmp_sub_nodate(l, r);
	}
	else{
		return mytime_cmp_sub_datetime(l, r);
	}
}

weektime_t myweektime(weektime_t wt)
{
	struct tm t;
	if (wt == NULL) return NULL;

	_getsystime(&t);

	wt->hour = t.tm_hour;
	wt->minute = t.tm_min;
	wt->second = t.tm_sec;
	wt->weekmask = 0;
	wt->weekmask |= (1 << t.tm_wday);

	return wt;
}

mytime_t mylocaltime(mytime_t dest)
{
	struct tm t;
	if (dest == NULL) return NULL;

	_getsystime(&t);
	t.tm_year += 1900;
	t.tm_mon += 1;

	dest->year = t.tm_year;
	dest->month = t.tm_mon;
	dest->day = t.tm_mday;
	dest->hour = t.tm_hour;
	dest->minute = t.tm_min;
	dest->second = t.tm_sec;

	return dest;
}

mytime_t localtime2mytime(struct tm* src, mytime_t dest)
{
	if (src == NULL || dest == NULL) return NULL;
	dest->year = src->tm_year;
	dest->month = src->tm_mon;
	dest->day = src->tm_mday;
	dest->hour = src->tm_hour;
	dest->minute = src->tm_min;
	dest->second = src->tm_sec;

	return dest;
}

void mytime2str(mytime_t t, char* str)
{
	if (t == NULL || str == NULL) return;
	sprintf(str, "%d-%02d-%02d %02d:%02d:%02d", t->year, t->month, t->day, t->hour, t->minute, t->second);
}

void str2mytime(char* str, mytime_t t)
{
	char* p = str;

	if (t == NULL || str == NULL) return;
	//"Y-M-D h:m:s"

	while (*p != '\0')
	{
		while (*p == ' ')p++;
		t->year = atoi(p);

		while (*p != '-')p++;
		p++;
		while (*p == ' ')p++;
		t->month = atoi(p);

		while (*p != '-')p++;
		p++;
		while (*p == ' ')p++;
		t->day = atoi(p);

		while (*p != ' ')p++;
		while (*p == ' ')p++;
		t->hour = atoi(p);

		while (*p != ':')p++;
		p++;
		while (*p == ' ')p++;
		t->minute = atoi(p);

		while (*p != ':')p++;
		p++;
		while (*p == ' ')p++;
		t->second = atoi(p);
		break;
	}
}

int str2hms(char* str, int* hour, int* minute, int* second)
{
	char* psz = NULL;

	if (str == NULL || hour == NULL || minute == NULL || second == NULL) return 0;

	psz = str;
	while ((*psz == ' ' || *psz == '\t') && *psz != '\0') psz++;
	if (*psz == '\0') return 0;

	*hour = atoi(psz);

	psz = strchr(psz, ':');
	if (psz){
		psz++;
		*minute = atoi(psz);
	}
	else
		return 0;

	psz = strchr(psz, ':');
	if (psz){
		psz++;
		*second = atoi(psz);
	}
	else
		return 0;

	return 1;
}


void print_hex(unsigned char buffer[], int len)
{
	int i = 0;
	static char map[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for (; i < len; i++){
		printf("%c", map[(buffer[i] >> 4) & 0x0F]);
		printf("%c", map[(buffer[i]) & 0x0F]);
	}
}

vec_t vec_alloc(vec_t p, int caporcnt, int size)
{
	vec_t rtn = NULL;

	assert(p);
	assert(caporcnt > 0);
	if (p == NULL || caporcnt <= 0) return NULL;

	if (p == NULL)
	{
		while ((rtn = (vec_t)malloc(sizeof(vec_st))) == NULL) sleep(10);
		while (rtn->data = (char*)malloc(caporcnt * size) == NULL) sleep(10);
		rtn->cap = caporcnt;
		rtn->cnt = 0;
		rtn->size = size;
	}
	else{
		if (caporcnt > p->cap)
		{
			p->data = (char*)realloc(p->data, caporcnt * size);
			rtn = p;
		}
		assert(p->size == size);
	}

	return rtn;
}

int     vec_add(vec_t p, int id, int x, int y, int z)
{
	assert(p != NULL && p->cnt < p->cap && p->data != NULL);
	if (p == NULL || p->cnt >= p->cap || p->data == NULL) return 0;

	memcpy(&(p->data[p->cnt*p->size]), p, p->size);
	p->cnt++;

	return p->cnt;
}

void*    vec_data(vec_t p, int i)
{
	assert(p != NULL && i < p->cnt && p->data != NULL);
	if (p == NULL || i >= p->cnt || p->data == NULL) return 0;

	return &(p->data[i*p->size]);
}

void    vec_clear(vec_t p)
{
	assert(p != NULL);
	p->cnt = 0;
}

void    vec_free(vec_t p)
{
	if (p)
	{
		if (p->data)
			free(p->data);

		free(p);
	}
}
