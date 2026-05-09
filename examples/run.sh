trap 'rm main' INT

if [ "$(uname -s)" = "Linux" ]; then
    modprobe tls && gcc main.c -o main -O3 -lssl -lcrypto -luring
else
    gcc main.c -o main -O3 -lssl -lcrypto
fi

./main && rm main