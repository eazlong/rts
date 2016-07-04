#include "asr_client.h"
#include <curl/easy.h>
#include <string.h>
#include <unistd.h>

namespace http
{
	curl_init asr_client::m_init;

	asr_client::asr_client()
	{
	}

	asr_client::~asr_client()
	{
		
	}


	int asr_client::start()
	{
		m_curl = curl_easy_init();

		const char* host = "POST /NMDPAsrCmdServlet/dictation?appId=NMDPTRIAL_yangchuang_xrrjkj_cn20160523051623&appKey=edf3cc49b90816be770f6855efe189eef24594f8dfc4a8d07771f7106a1c9f570f6ca0c874b686ebc5ef40b5d0caa3c20f37baeefb62be72d3b5823dd12194e0&id=fc2jvf7p HTTP/1.1\r\n"
					"Host:dictation.nuancemobility.net\r\n"
					"Content-Type:audio/x-wav;codec=pcm;bit=16;rate=16000\r\n"
					"Accept:text/plain\r\n"
					"Accept-Language:ENUS\r\n"
					"Accept-Topic:Dictation\r\n"
					"Expect: 100-continue\r\n"
					"Transfer-Encoding:chunked\r\n\r\n";
		const char* url = "https://dictation.nuancemobility.net";//appId=NMDPTRIAL_yangchuang_xrrjkj_cn20160523051623&appKey=edf3cc49b90816be770f6855efe189eef24594f8dfc4a8d07771f7106a1c9f570f6ca0c874b686ebc5ef40b5d0caa3c20f37baeefb62be72d3b5823dd12194e0&id=fc2jvf7p";
		
   		curl_easy_setopt( m_curl, CURLOPT_URL, url);   
   		//curl_easy_setopt( m_curl, CURLOPT_POSTFIELDS, url );
		//curl_easy_setopt( m_curl, CURLOPT_POST, 1L );
		curl_easy_setopt( m_curl, CURLOPT_VERBOSE, 1L ); 
		//curl_easy_setopt( m_curl, CURLOPT_FOLLOWLOCATION, 1);

		// struct curl_slist *headers = NULL;
		// headers = curl_slist_append( headers, "Content-Type:audio/x-speex;rate=8000" );
		// headers = curl_slist_append( headers, "Accept:text/plain" );
		// headers = curl_slist_append( headers, "Accept-Language:cn_MA" );
		// headers = curl_slist_append( headers, "Accept-Topic:dictation" );
		// headers = curl_slist_append( headers, "Transfer-Encoding:chunked" );
		//curl_easy_setopt( m_curl, CURLOPT_HTTPHEADER, headers );
		//curl_easy_setopt( m_curl, CURLOPT_HEADER, 1L );
		//curl_easy_setopt( m_curl, CURLOPT_COOKIEFILE, "/Users/zhu/CProjects/curlposttest.cookie");

		curl_easy_setopt( m_curl, CURLOPT_CONNECT_ONLY, 1L );

  		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYPEER, 0L );
  		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYHOST, 0L );

		CURLcode res = curl_easy_perform( m_curl );
		if ( res != CURLE_OK )
		{
			printf("****ERROR****:create curl error! %d\n", res);
		}
		//curl_slist_free_all( headers );

		long sockextr;
    	res = curl_easy_getinfo( m_curl, CURLINFO_LASTSOCKET, &sockextr);
	    if(CURLE_OK != res) {
	      printf("Error: %s\n", curl_easy_strerror(res));
	      return 1;
	    }
	    curl_socket_t sockfd = (curl_socket_t)sockextr;
	    wait_on_socket(sockfd, 0, 60000L);
	    size_t n;
	    curl_easy_send( m_curl, host, strlen(host), &n );

	    printf( "Send data:%s\n", host);
	  	
	  	curl_easy_cleanup( m_curl );

		return 0;
	}																																																																																																																																																																																																															

	std::string asr_client::make_url( const std::string& host, const std::map<std::string,std::string>& params)
	{
		std::string url;
		url += host;
		std::map<std::string,std::string>::const_iterator it = params.begin();
		for( ; it != params.end(); it ++ )
		{
			url += (it==params.begin()?"?":"&") + it->first + "=" + it->second;
		}
		//url += " HTTP/1.1";
		return url;
	}

	int asr_client::post( const char* buffer, size_t buflen )
	{
		long sockextr;
    	CURLcode res = curl_easy_getinfo( m_curl, CURLINFO_LASTSOCKET, &sockextr);
	    if(CURLE_OK != res) {
	      printf("Error: %s\n", curl_easy_strerror(res));
	      return 1;
	    }
	    curl_socket_t sockfd = (curl_socket_t)sockextr;
	    wait_on_socket(sockfd, 0, 60000L);
		size_t n;
	    curl_easy_send( m_curl, buffer, buflen, &n );
		return 0;
	}

	 			
    int asr_client::process_result()
    {
    	long sockextr;
    	CURLcode res = curl_easy_getinfo( m_curl, CURLINFO_LASTSOCKET, &sockextr);
	    if(CURLE_OK != res) {
	      printf("Error: %s\n", curl_easy_strerror(res));
	      return 1;
	    }

	    printf("get result from socket!\n");
	    curl_socket_t sockfd = (curl_socket_t)sockextr;
	    size_t iolen;
		for(;;) 
		{
			char buf[1024] = {0};

			wait_on_socket(sockfd, 1, 60000L);
			res = curl_easy_recv(m_curl, buf, 1024, &iolen);

			if(CURLE_OK != res)
				break;

			curl_off_t nread = (curl_off_t)iolen;

			printf("Received %" CURL_FORMAT_CURL_OFF_T " bytes. \n %s \n", nread, buf );
		}
		return 0;
    }

    /* Auxiliary function that waits on the socket. */
	int asr_client::wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
	{
	  struct timeval tv;
	  fd_set infd, outfd, errfd;
	  int res;

	  tv.tv_sec = timeout_ms / 1000;
	  tv.tv_usec= (timeout_ms % 1000) * 1000;

	  FD_ZERO(&infd);
	  FD_ZERO(&outfd);
	  FD_ZERO(&errfd);

	  FD_SET(sockfd, &errfd); /* always check for error */

	  if(for_recv) {
	    FD_SET(sockfd, &infd);
	  }
	  else {
	    FD_SET(sockfd, &outfd);
	  }

	  /* select() returns the number of signalled sockets or -1 */
	  res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
	  return res;
	}																
}