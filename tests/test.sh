if [ "$(uname -s)" = "Linux" ]; then
    gcc test_server.c -o server -O3 -flto -lssl -lcrypto -luring
    gcc test_client.c -o test -O3 -flto -lssl -lcrypto -lkqueue

    sudo modprobe tls
else
    exit 1
fi

sudo ./server

rm -f test server