#include "tcp_server.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

namespace server
{
	tcp_server::tcp_server( const std::string& address, const unsigned short port, bool mult_thread )
		:m_address( address ), m_port( port ), m_max_fd(0), m_need_lock( mult_thread )
	{
		if ( m_need_lock )
		{
			pthread_mutex_init( &m_lock, 0 );
		}
	}

	tcp_server::~tcp_server()
	{
		if ( m_need_lock )
		{
			pthread_mutex_destroy( &m_lock );
		}
	}

	int tcp_server::start()
	{
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket == -1)
		{
			//RTMP_Log(RTMP_LOGERROR, "%s, couldn't create socket", __FUNCTION__);
			return FAILED;
		}

		int tmp = 1;
		setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR,(char *) &tmp, sizeof(tmp) );
		setsockopt( m_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&tmp, sizeof(tmp)); 

		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(m_address.c_str());//htonl(INADDR_ANY);
		addr.sin_port = htons(m_port);

		if ( bind(m_socket, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1)
		{
			//RTMP_Log(RTMP_LOGERROR, "%s, TCP bind failed for port number: %d", __FUNCTION__, m_port);
			return FAILED;
		}

		if ( listen(m_socket, 10) == -1 )
		{
			//RTMP_Log(RTMP_LOGERROR, "%s, listen failed", __FUNCTION__);
			close(m_socket);
			return FAILED;
		}

		return SUCCESS;
	}

	int tcp_server::noblock()
	{
		int flag = fcntl( m_socket, F_GETFL, 0);
        flag |= O_NONBLOCK;
        if (fcntl( m_socket, F_SETFL, flag) < 0) 
        {      /* fgets no-block now */
            return FAILED;
        }
        return SUCCESS;
	}

	void tcp_server::stop()
	{
		close( m_socket );
	}


	int tcp_server::accept()
	{
		struct sockaddr_in addr;
	    socklen_t addrlen = sizeof(struct sockaddr_in);
	    int sockfd = ::accept( m_socket, (struct sockaddr *) &addr, &addrlen);

	    if (sockfd > 0)
	    {
	#ifdef linux
	       struct sockaddr_in dest;
	       char destch[16];
	       socklen_t destlen = sizeof(struct sockaddr_in);
	       getsockopt(sockfd, SOL_IP, SO_ORIGINAL_DST, &dest, &destlen);
	       strcpy(destch, inet_ntoa(dest.sin_addr));
	       printf( "%s: accepted connection from %s to %s\n", __FUNCTION__, inet_ntoa(addr.sin_addr), destch);
	#else
	       //RTMP_Log(RTMP_LOGDEBUG, "%s: accepted connection from %s\n", __FUNCTION__, inet_ntoa(addr.sin_addr));
	#endif
	      /* Create a new thread and transfer the control to that */
	       //RTMP_Log(RTMP_LOGDEBUG, "%s: processed request\n", __FUNCTION__);
	    }

	    return sockfd;
	}


	int tcp_server::wait_for_data_ready( std::list<int>& ready_fd_list, int timeout )  // client connection socket
	{
		// timeout for http requests
		struct timeval tv;
		memset(&tv, 0, sizeof(struct timeval));
		tv.tv_sec = timeout/1000/1000;
		tv.tv_usec = timeout%(1000*1000);

		fd_set fds;
		FD_ZERO( &fds );
		lock();
		for( std::set<int>::iterator it=m_client_fds.begin(); it!=m_client_fds.end(); it++ )
		{
			FD_SET((*it), &fds);
		}
		unlock();

		if ( select( m_max_fd+1, &fds, NULL, NULL, &tv) <= 0 )
		{
			return FAILED;
		}

		lock();
		for( std::set<int>::iterator it=m_client_fds.begin(); it!=m_client_fds.end(); it++ )
		{
			if ( FD_ISSET((*it), &fds) )
			{
				ready_fd_list.push_back(*it);
			}
		}
		unlock();

		return SUCCESS;
	}

	void tcp_server::add_client_fd( int sockfd )
	{
		printf( "add fd %d to list\n", sockfd );
		if ( m_max_fd < sockfd )
		{
			m_max_fd = sockfd;
		}
		lock();
		m_client_fds.insert( sockfd );
		unlock();
	}

	int tcp_server::read( int client_fd, char* buf, unsigned int size, int flags )
	{
		ssize_t length = ::recv( client_fd, buf, size, flags );
		return length;
	}

	int tcp_server::write( int client_fd, const char* buf, unsigned int size, int flags )
	{
		ssize_t length = ::send( client_fd, buf, size, flags );
		return length;
	}


	int tcp_server::get_event_type( int fd )
	{
		char buf[1];
		int r = read( fd, buf, 1, MSG_PEEK );
		printf("****************************************get_event_type：%d\n", r);
		if ( r < 0 )
		{
			return ERROR;
		}
		else if (r == 0 )
		{
			return CLOSE;
		}
		return DATA;
	}

	void tcp_server::close( int fd )
	{
		printf( "close fd %d\n", fd );
		::close( fd );
	}

	void tcp_server::remove_client_fd( int fd )
	{
		lock();
		m_client_fds.erase( fd );
		unlock();
	}

	void tcp_server::lock()
	{
		if ( m_need_lock )
		{
			pthread_mutex_lock(&m_lock);
		}
	}

	void tcp_server::unlock()
	{
		if ( m_need_lock )
		{
			pthread_mutex_unlock(&m_lock);
		}
	}
}