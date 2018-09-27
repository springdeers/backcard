#ifndef __MY_CURL_H_
#define __MY_CURL_H_


#define false 0
#define true  1


#include "curl/curl.h"  
#include "curl/easy.h"

CURLcode curl_get_req(char *url, char *response);
CURLcode curl_post_req(const char *url, const char *postParams, char *response);



#endif