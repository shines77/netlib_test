@echo "start test server ..."
asio_echo_serv.exe --host=127.0.0.1 --port=8090 --mode=http --test=qps --packet-size=512 --thread-num=0
pause
