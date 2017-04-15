ps -ef|grep realtime_translate_system|grep -v grep|awk '{print "kill -9 " $2}' |sh
export LD_LIBRARY_PATH=$(pwd)/iflytek/libs/x64
./realtime_translate_system
