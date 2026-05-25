trap 'rm main' INT

printf 'Select Example: \n1. Simple Routing\n2. File Server\n3. WebSocket\n4. File I/O\n'

stty -echo
read -n 1 name
stty echo

case $name in
    49) rg="Pressed 1"  ;; # works?
    50) rg="Pressed 2" ;;
    51) rg="Pressed 3" ;;
    52) rg="Pressed 4" ;;
    *) exit 0 ;;
esac

echo $rg

if [ "$(uname -s)" = "Linux" ]; then
    modprobe tls && gcc main.c -o main -O3 -lssl -lcrypto -luring
else
    gcc main.c -o main -O3 -lssl -lcrypto
    # eval "$arg -lm -lavformat -lavcodec -lavutil -framework AudioToolbox"
fi

./main && rm main