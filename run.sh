export LD_LIBRARY_PATH=.
./deceptus

if [[ -f "00000.bmp" ]]; then
    echo "creating video"
    convert -delay 1.5 -loop 0 *.bmp session.gif
    gifsicle -i session.gif -O3 --resize 480x270 -o session_compressed.gif
    rm *.bmp
    echo "done :)"
fi

