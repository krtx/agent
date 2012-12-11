#!/bin/sh

vector=$1
border=$2

/usr/bin/gnuplot <<EOF

ismin(x) = (x<min)?min=x:0
ismax(x) = (x>max)?max=x:0
grabx(x)=(x<x0)?x0=x:(x>x1)?x1=x:0
graby(y)=(y<y0)?y0=y:(y>y1)?y1=y:0

x0 = 1e38
x1 = -1e38
y0 = 1e38
y1 = -1e38

set table "/dev/null"
plot "${vector}" u (grabx(\$1)):(graby(\$2))
unset table

set view map

set contour
set cntrparam linear
set cntrparam levels discrete 0
unset surface
set table 'contour'
splot "${border}" with lines linetype 2 lc rgb "black"
unset table
unset contour

set xrange[x0-5:x1+5]
set yrange[y0-5:y1+5]

set size 1, 1
set terminal png
plot 'contour' w l lw 1.5 lc rgb "black" notitle, "${vector}" u 1:2:(\$3 < 0 ? 0 : 1/0) w p pt 19 lc rgb "blue" notitle, "${vector}" u 1:2:(\$3 > 0 ? 0 : 1/0) w p pt 19 lc rgb "red" notitle

EOF

