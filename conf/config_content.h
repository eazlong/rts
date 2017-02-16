#pragma once
#include <string>
#include <list>

class config_content
{
private:
	config_content();
	virtual ~config_content();
public:
	static config_content* get_instance();

public:
	typedef struct thread_
	{
		short audio_threads;
		short asr_threads;
		short translate_threads;
	}thread;
	thread thds;

	typedef struct audio_input_
	{
		std::string format;
		int samplerate;
	}audio_input;
	audio_input audio_info;

	typedef struct server_
	{
		std::string device;
		unsigned short port;
	}server;
	server audio_svr;
	server control_svr;

	typedef struct log_
	{
		int log_level;
		int rtmp_log_level;
		int stdout;
		std::string file;
	}log;
	log l;

	typedef struct translate_account_
	{
		std::string type;
		std::string client_id;
		std::string client_secret;
	}translate_account;
	std::list<translate_account> taccounts;

	typedef struct  asr_account_
	{
		std::string type;
		std::string id;
		std::string appid;
		std::string secret_key;
		std::string accept_format;
		long auth_interval;
	}asr_account;
	std::list<asr_account> aaccounts;

private:
	static config_content* m_instance;
};