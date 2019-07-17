export LD_LIBRARY_PATH=.
./deceptus

if [[ -f "00000.bmp" ]]; then
    echo "creating video"
    convert -delay 1.5 -loop 0 *.bmp session.gif
    rm *.bmp
    echo "done :)"
fi

