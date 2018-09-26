#ifndef _FORWARD_H_
#define _FORWARD_H_

#include "p2p.h"
#include "json/cJSON.h"
#include "client.h"
#include "p2pclient.h"
#include "tool.h"
#include "userstatus.h"

// printsvr_callback
void forward_log_dyncode_to_monitor(char *jstr);
//void forward_dyncode_to_app(char * jstr);
//void forward_warn_to_app(char *jstr);
//
//// moniclient_callback
//void forward_reqcode_to_printsvr(char* jstr);
//void forward_log_reqcode_to_monitor(char *jstr);
//void forward_taskpau_to_monitor(char *jstr);
//void forward_log_taskpau_to_monitor(char *jstr);
//void forward_taskcon_to_monitor(char *jstr);
//void forward_log_taskcon_to_monitor(char *jstr);
//void forward_taskdone_to_monitor(char *jstr);
//void forward_log_taskdone_to_monitor(char *jstr);
//
//
//void forward_status_to_monitor(char* jstr);
//void forward_log_to_monitor(char *jstr);
//void forward_pos_to_printsvr(char *jstr);
//void forward_pos_to_monitor(char *jstr);
//
//// moniclient and printsvr callback
//void forward_warn_to_monitor(char * jstr);
//
//// monitor callback
//void forward_taskpau_ack_to_printsvr(char *jstr);
//void forward_taskpau_ack_to_app(char *jstr);
//void forward_taskcon_ack_to_printsvr(char *jstr);
//void forward_taskcon_ack_to_app(char *jstr);
//void forward_taskdone_ack_to_printsvr(char *jstr);
//void forward_taskdone_ack_to_app(char *jstr);

void forward_to_app(char *head, char* jstr);
void forward_to_printsvr(char *head, char* jstr);
void forward_to_monitor(char *head, char *jstr);	// ������ֱ��͸����monitor
void forward_log_x_to_monitor(char *head, char* jstr);// ������Ӧ����־��ת����monitor�����Ҵ��

#endif