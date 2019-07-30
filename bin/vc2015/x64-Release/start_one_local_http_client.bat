@echo "start test clients ..."
asio_echo_client.exe --host=127.0.0.1 --port=8090 --mode=http --test=qps --packet-size=512 --thread-num=1
pause
