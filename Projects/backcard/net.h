#ifndef _NET_H__
#define _NET_H__
#include "client.h"

#ifdef __cplusplus 
extern "C"{
#endif

	void   check_inactive_client(int sec);
	void   remove_zombie_from_lives();
	void   clear_zombies();
	void   clear_deads_from_lives();
	void   check_dead_from_lives(sess_t sessdead);
	BOOL   check_zombie(sess_t sess, time_t now);
	void   pingsession(int waits);
	void   save_deadsession(int waits);
	int    send2namend(char* name, char* buffer, int bufferlen);

#ifdef __cplusplus 
}
#endif

#endif