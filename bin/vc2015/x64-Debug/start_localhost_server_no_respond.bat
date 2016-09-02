@echo "start test server ..."
asio_echo_serv.exe --host=0.0.0.0 --port=8090 --mode=echo --test=qps --packet-size=64 --thread-num=1 --echo=0
pause
