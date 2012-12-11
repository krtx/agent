#!/bin/sh

vector=$1
border=$2

/usr/bin/gnuplot <<EOF

set view map

set contour
set cntrparam linear
set cntrparam levels discrete 0
unset surface
set table 'contour'
splot "${border}" with lines linetype 2 lc rgb "black"
unset table
unset contour

set terminal png
plot "${vector}" u 1:2:(\$3 < 0 ? 0 : 1/0) w p pt 7 lc rgb "blue", "${vector}" u 1:2:(\$3 > 0 ? 0 : 1/0) w p pt 7 lc rgb "red", 'contour' w l

EOF

