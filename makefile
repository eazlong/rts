OBJS = realtime_translate_system.o thread.o audio_processor.o speex_audio_processor.o ogg_encode.o asr_client.o asr_client_baidu.o asr_client_nuance.o asr_client_keda.o \
	translate_client.o translate_client_google.o audio_data_processor.o \
	rtmp_connection.o tcp_server.o thread_pool.o log.o config.o config_content.o tinyxml2.o control_data_processor.o db.o asr_client_manager.o http_client.o room.o room_manager.o
CC = g++
INC = -I./audio -I./http -I../rtmpdump -I./server -I./thread -I./log -I./conf -I./xml -I./db -I./room -I./iflytek/include
LIB = -lspeex -lspeexdsp -lpthread -lcurl -lrtmp -lmysqlclient -logg -L./iflytek/libs/x86 -lmsc
#SLIB = /home/xialang/code/rtmp/rtmpdump/librtmp/librtmp.a
CFLAGS = -Wall -O0 -g -D_DEBUG
TAGET = realtime_translate_system

realtime_translate_system:$(OBJS)
	$(CC) $(OBJS) $(INC) $(LIB) -o $(TAGET) 
realtime_translate_system.o:rtranslate_server.c 
	$(CC) $(CFLAGS) $(INC) -c $< -o $@
thread.o:thread/thread.c
	$(CC) $(CFLAGS) -c $< -o $@
audio_processor.o:audio/audio_processor.cpp
	$(CC) $(CFLAGS) -c $< -o $@
ogg_encode.o:audio/ogg_encode.cpp
	$(CC) $(CFLAGS) -c $< -o $@
speex_audio_processor.o:audio/speex_audio_processor.cpp
	$(CC) $(CFLAGS) -c $< -o $@
http_client.o:http/http_client.cpp
	$(CC) $(CFLAGS) -c $< -o $@
asr_client.o:http/asr_client.cpp
	$(CC) $(CFLAGS) -c $< -o $@
asr_client_baidu.o:http/asr_client_baidu.cpp
	$(CC) $(CFLAGS) -c $< -o $@
asr_client_keda.o:http/asr_client_keda.cpp
	$(CC) $(CFLAGS) -c $< -o $@
asr_client_nuance.o:http/asr_client_nuance.cpp
	$(CC) $(CFLAGS) -c $< -o $@
asr_client_manager.o:http/asr_client_manager.cpp
	$(CC) $(CFLAGS) -c $< -o $@
translate_client.o:http/translate_client.cpp
	$(CC) $(CFLAGS) -c $< -o $@
translate_client_google.o:http/translate_client_google.cpp
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
db.o:db/db.cpp
	$(CC) $(CFLAGS) -c $< -o $@
room.o:room/room.cpp
	$(CC) $(CFLAGS) -c $< -o $@
room_manager.o:room/room_manager.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o $(TAGET)
