#pragma once
#include <string>

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
private:
	static config_content* m_instance;
};