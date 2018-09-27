#ifndef _FORWARD_H_
#define _FORWARD_H_

#include "p2p.h"
#include "json/cJSON.h"
#include "client.h"
#include "p2pclient.h"
#include "tool.h"
#include "userstatus.h"



void forward_to_moniclient(char *head, char* jstr);
void forward_to_printsvr(char *head, char* jstr);

#endif