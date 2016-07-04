OBJS = realtime_translate_system.o thread.o audio_processor.o speex_audio_processor.o asr_client.o translate_client.o audio_data_processor.o \
	rtmp_connection.o tcp_server.o thread_pool.o log.o config.o config_content.o tinyxml2.o control_data_processor.o
CC = g++
INC = -I./audio -I./http -I../rtmpdump -I./server -I./thread -I./log -I./conf -I./xml
LIB = -lspeex -lspeexdsp -lpthread -lcurl -lrtmp
#SLIB = /home/xialang/code/rtmp/rtmpdump/librtmp/librtmp.a
CFLAGS = -Wall -O -g -D_DEBUG
TAGET = realtime_translate_system

realtime_translate_system:$(OBJS)
	$(CC) $(OBJS) $(INC) $(LIB) -o $(TAGET) 
realtime_translate_system.o:rtranslate_server.c 
	$(CC) $(CFLAGS) $(INC) -c $< -o $@
thread.o:thread/thread.c
	$(CC) $(CFLAGS) -c $< -o $@
audio_processor.o:audio/audio_processor.cpp
	$(CC) $(CFLAGS) -c $< -o $@
speex_audio_processor.o:audio/speex_audio_processor.cpp
	$(CC) $(CFLAGS) -c $< -o $@
asr_client.o:http/asr_client.cpp
	$(CC) $(CFLAGS) -c $< -o $@
translate_client.o:http/translate_client.cpp
	$(CC) $(CFLAGS) -c $< -o $@
audio_data_processor.o:server/audio_data_processor.cpp
	$(CC) $(CFLAGS) -c $< -o $@
control_data_processor.o:server/control_data_processor.cpp
	$(CC) $(CFLAGS) -c $< -o $@
rtmp_connection.o:server/rtmp_connection.cpp
	$(CC) $(CFLAGS) -c $< -o $@
tcp_server.o:server/tcp_server.cpp
	$(CC) $(CFLAGS) -c $< -o $@
thread_pool.o:thread/thread_pool.cpp
	$(CC) $(CFLAGS) -c $< -o $@
log.o:log/log.cpp
	$(CC) $(CFLAGS) -c $< -o $@
config.o:conf/config.cpp
	$(CC) $(CFLAGS) -c $< -o $@
config_content.o:conf/config_content.cpp
	$(CC) $(CFLAGS) -c $< -o $@
tinyxml2.o:xml/tinyxml2.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o $(TAGET)
