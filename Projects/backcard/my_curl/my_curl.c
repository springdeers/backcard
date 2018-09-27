#include "my_curl.h"

#pragma comment(lib, "libcurl.lib")  

FILE *fp;  //定义FILE类型指针
//这个函数是为了符合CURLOPT_WRITEFUNCTION而构造的
//完成数据保存功能
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	//int written = fwrite(ptr, size, nmemb, (FILE *)fp);
	printf("recv data from remote site:\n %s.\n", (char *)ptr);
	//return written;
	return nmemb;
}


// reply of the requery  
size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream)
{
	//cout << "----->reply" << endl;
	//string *str = (string*)stream;
	//cout << *str << endl;
	//(*str).append((char*)ptr, size*nmemb);
	//return size * nmemb;
	printf("recv from remote site stream:\n%s\n", (char*)ptr);
	printf("recv from remote site ptr   :\n%s\n",(char*)ptr);
}

// http GET  
CURLcode curl_get_req(char *url, write_data_cb_t write_data_cb)
{
	// init curl  
	CURL *curl = curl_easy_init();
	// res code  
	CURLcode res;
	if (curl)
	{
		// set params  
		curl_easy_setopt(curl, CURLOPT_URL, url); // url  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false  
		curl_easy_setopt(curl, CURLOPT_VERBOSE, false);
		//curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_cb);
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		//curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3); // set transport and time out time  
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
		// start req  
		res = curl_easy_perform(curl);
	}
	// release curl  
	curl_easy_cleanup(curl);
	return res;
}

// http POST  
CURLcode curl_post_req(const char *url, const char *postParams, char *response)
{
	// init curl  
	CURL *curl = curl_easy_init();
	// res code  
	CURLcode res;
	if (curl)
	{
		// set params  
		curl_easy_setopt(curl, CURLOPT_POST, 1); // post req  
		curl_easy_setopt(curl, CURLOPT_URL, url); // url  
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParams); // params  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false  
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
		// start req  
		res = curl_easy_perform(curl);
	}
	// release curl  
	curl_easy_cleanup(curl);
	return res;
}
