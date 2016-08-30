@echo "start test server ..."
asio_echo_serv.exe --host=127.0.0.1 --port=8090 --mode=http --test=pingpong --packet-size=64 --thread-num=8
pause
