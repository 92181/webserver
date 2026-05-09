gcc test_client.c -o test -O3 -lssl && gcc test_server.c -o server -O3 -lssl -lcrypto && ./server && rm test && rm server

# add if linux use -libkqueue else do nothing