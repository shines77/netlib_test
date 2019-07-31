@echo "start test clients ..."
for /l %%i in (1,1,10) do start /min "asio_echo_client" asio_echo_client.exe --host=127.0.0.1 --port=8090 --mode=echo --test=qps --packet-size=64 --thread-num=1
