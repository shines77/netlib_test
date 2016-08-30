@echo "start test clients ..."
for /l %%i in (1,1,10) do start /min "asio_echo_client" asio_echo_client.exe --host=192.168.88.102 --port=8090 --mode=echo --test=qps --packet-size=64 --thread-num=1
