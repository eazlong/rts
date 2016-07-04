#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

#include <string>

namespace server
{
	class rtmp_data_processor
	{
	public:
		virtual int initialize() = 0;
		virtual void cleanup() = 0;
		virtual int process_audio( const char* buf, int size ) = 0;
		virtual int process_video( const char* buf, int size ) = 0;
		virtual int get_audio_status( std::string& file ) = 0;
		virtual int get_video_status() const = 0;
	};


	class rtmp_connection
	{
	public:
		enum
		{
			SUCCESS,
			FAILED,
			CONTINUE
		};
	public:
		rtmp_connection( rtmp_data_processor* process, int log_level );
		virtual ~rtmp_connection();

		static int send_connect_result( RTMP *r, double txn );
		static int send_result_number( RTMP *r, double txn, double ID );
		
		int handshake( int sockfd );
		int prepare();
		int process();
		void cleanup();

		rtmp_data_processor* get_data_processor() const;
		std::string get_id() const;
		long get_last_package_num() const;
		long get_cur_package_num() const;
	protected:
		int server_invoke( RTMPPacket* packet, int offset);

	private:
		RTMP *m_rtmp;        /* our session with the real client */
		RTMPPacket m_packet;
		std::string m_cert;
		std::string m_key;
		std::string m_id;
		int m_streamID;
		void* m_ssl_context;
		rtmp_data_processor* m_processor;
		long m_cur_package;
		long m_last_package;
	};
}