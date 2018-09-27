#ifndef __MY_CURL_H_
#define __MY_CURL_H_


#define false 0
#define true  1


#include "curl/curl.h"  
#include "curl/easy.h"

typedef size_t(*write_data_cb_t)(void *ptr, size_t size, size_t nmemb, void *stream);


size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
CURLcode curl_get_req(char *url, write_data_cb_t write_data_func);
CURLcode curl_post_req(const char *url, const char *postParams, char *response);



#endif