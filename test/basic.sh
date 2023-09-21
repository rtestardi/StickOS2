#! /bin/sh
# this test exercises platform similarities

case `uname 2>/dev/null` in
  CYGWIN*)
    build="windows"
    ;;
  Windows*)
    build="windows"
    ;;
  Linux)
    build="linux"
    ;;
esac

if [ X${1:-} = X-r ]; then
  BASIC="../stickos/PIC32/basic.exe"
else
  BASIC="../stickos/PIC32/basic.exe"
fi

echo first we test parsing and running expressions
"$BASIC" -q <<EOF
print 1
print 1+1
print 1+2*3
print 1*2+3
print (1+2)*3
print 1+(2*3)
print 1+(2*(3+4))
print (1+2)*(3+4)
print (1+2)*(3*(4+5))
print 1+1+1
print 1+(1+1)
print 16/4/2
print 16/(4/2)
EOF

echo then we unparse them
"$BASIC" -q <<EOF
10 print 1
20 print 1+1
30 print 1+2*3
40 print 1*2+3
50 print (1+2)*3
60 print 1+(2*3)
70 print 1+(2*(3+4))
80 print (1+2)*(3+4)
90 print (1+2)*(3*(4+5))
100 print 1+1+1
110 print 1+(1+1)
120 print 16/4/2
130 print 16/(4/2)
list
run
new
EOF

echo then we test format specifiers
"$BASIC" -q <<EOF
print hex 20, 30, "dec", dec 20, 30
print dec 0x100, 0x200, "hex", hex 0x100, 0x200
print "hex", hex 20, 30, "dec", dec 20, 30
print "dec", dec 0x100, 0x200, "hex", hex 0x100, 0x200
print "default", 0x200
print "default", 30
EOF

echo then we unparse them
"$BASIC" -q <<EOF
10 print hex 20, 30, "dec", dec 20, 30
20 print dec 0x100, 0x200, "hex", hex 0x100, 0x200
30 print "hex", hex 20, 30, "dec", dec 20, 30
40 print "dec", dec 0x100, 0x200, "hex", hex 0x100, 0x200
50 print "default", 0x200
60 print "default", 30
list
run
EOF

echo then we test operators
"$BASIC" -q <<EOF
print 1+2, 3+2
print 1-2, 2-1
print 2*3, 3*3
print 7/2, 7/3
print 7%2, 8%2
print 1|2, 2|6
print 1&2, 2&6
print 1^2, 2^6
print 1||0, 1||1, 0||0
print 1&&0, 1&&1, 0&&0
print 1^^0, 1^^1, 0^^0
print 1<<2, 8>>2
print 1<2, 2<1, 1<1
print 1>2, 2>1, 1>1
print 1<=2, 2<=1, 1<=1
print 1>=2, 2>=1, 1>=1
print 1==2, 2==1, 1==1
print 1!=2, 2!=1, 1!=1
EOF

echo then we unparse them
"$BASIC" -q <<EOF
1 print 1+2, 3+2
2 print 1-2, 2-1
3 print 2*3, 3*3
4 print 7/2, 7/3
5 print 7%2, 8%2
6 print 1|2, 2|6
7 print 1&2, 2&6
8 print 1^2, 2^6
9 print 1||0, 1||1, 0||0
10 print 1&&0, 1&&1, 0&&0
11 print 1^^0, 1^^1, 0^^0
12 print 1<<2, 8>>2
13 print 1<2, 2<1, 1<1
14 print 1>2, 2>1, 1>1
15 print 1<=2, 2<=1, 1<=1
16 print 1>=2, 2>=1, 1>=1
17 print 1==2, 2==1, 1==1
18 print 1!=2, 2!=1, 1!=1
list
run
new
EOF

echo then test unary operators
"$BASIC" -q <<EOF
print 1, !1, ~1
print 0, !0, ~0
print !1+3, ~1+3
print !0+3, ~0+3
print +2, -2
print 3*+2, 3*-2
print +2*3, -2*3
print !!4, !!0, !!(2+2), !!(2-2)
EOF

echo then we unparse them
"$BASIC" -q <<EOF
1print 1, !1, ~1
2print 0, !0, ~0
3print !1+3, ~1+3
4print !0+3, ~0+3
5print +2, -2
6print 3*+2, 3*-2
7print +2*3, -2*3
8print !!4, !!0, !!(2+2), !!(2-2)
list
run
new
EOF

echo then we test line input
"$BASIC" -q <<EOF
10 rem line 10
list
save
20 rem line 20
list
20 
list
30 rem line 30
list
15 rem line 15
list
save
20
list
10 rem new line 10
list
save
list
print "test undo"
10
list
undo
list
new
list
EOF

echo then we test some math
"$BASIC" -q <<EOF
10 dim a
20 dim r
list
25 let a=5
30 print a, r
run
50 print 3+5*6
60 print (3+5)*6
70 print 3+(5*6) ,2, 1
80 print ((3+5*6))
100 dim b, c,d ,e
110 let q = 4+7*(1+1)
120 let qq = 44+77*(11+11)
130 print q,qq
list
run
1 dim q, qq
list
run
EOF

echo test multiple assignments
"$BASIC" -q <<EOF
10 dim a,b , c
20 let a=5,b=5+5 ,   c=a+b
30 print a,b ,  c
list
run
EOF

echo test some variable types
"$BASIC" -q <<EOF
1dim a as byte
2dim b as short
3dim c 
4let c=-1
5let b=c
6let a=b
7print hex a,b,c
8let c=0x10011
9let b=0x10022
10let a=0x10033
11print hex a,b,c
list
run
EOF

echo test re-dim errors
"$BASIC" -q <<EOF
1dim a as byte
2dim a as byte
list
run
new
1dim a as short
2dim a as short
list
run
new
1dim a
2dim a
list
run
EOF

echo test some assertions
"$BASIC" -q <<EOF
10 assert 1
20 assert 0
30 assert 3%2==1
40 dim a
50 let a=3%2==1
60 assert a
70 assert 3%2==0
90 let a=3%2==0
100 assert a
run
cont
cont
cont
cont
EOF

echo test some more statements
"$BASIC" -q <<EOF
130 if x+((y*2)) then
140 elseif 1 then
150 else
160 endif
170 while 1 do
180 endwhile
190 gosub other
200 return
list
EOF

echo then test delete
"$BASIC" -q <<EOF
10 dim a
20 dim r
25 let a=5
30 print a, r
50 print 3+5*6
60 print (3+5)*6
70 print 3+(5*6) ,2, 1
80 print ((3+5*6))
90 dim a
100 dim b, c,d ,e
110 let q = 4+7*(1+1)
120 let qq = 44+77*(11+11)
130 if x+((y*2)) then
140 elseif 1 then
150 else
160 endif
170 while 1 do
180 endwhile
190 gosub other
200 return
delete 120
list
delete 130-160
list
delete 100-
list
delete -30
list
delete
list
EOF

echo profile tests
"$BASIC" -q <<EOF
demo
95 break
save
profile
run
profile
9999 rem
save
profile
run
profile
EOF

echo larger tests
"$BASIC" -q <<EOF
new
1 print 1,2+2
2 dim a,x
3let a=5
4print a
5 let x=15
6 print a+x
7 rem dim a
8 rem dim a
list
run
new
10 dim a
20 let a=0
30 while a < 10 do
40 print a, a/2*2, a%2
50 if a%2 then
60 print 1, "odd", 2, "odd"
70 endif
80 let a=a+1
90 endwhile
91 gosub outer
92 gosub outer
100 end
110 print "not reached"
190 sub outer
200 gosub inner
210 return
220 endsub
290 sub inner
300 print "here"
310 return
320 endsub
400 on timer 1 do gosub timer
410 end
490 sub timer
500 print "timer"
510 return
520 endsub
list
run
trace
trace on
trace
run
trace off
step
step on
step
run
cont

cont
step off
print "outer"
list outer
print "inner"
list inner
print "delete inner"
delete inner
list
print "delete outer"
delete outer
list
list neither
delete neither
EOF

echo test some ifs and whiles
"$BASIC" -q <<EOF
10 if 0 then
20 print 0
30 if 1 then
40 print 2
50 else
60 print 3
70 endif
80 elseif 1 then
90 print 1
100 if 1 then
110 print 11
120 else
130 print 4
140 endif
150 else
160 print 0
170 if 1 then
180 print 5
190 else
200 print 6
210 endif
220 endif
list
run
list 100
list -100
list 100-
list 90-110
list -
EOF

echo test do/until
"$BASIC" -q <<EOF
10 dim a
20 do
30 print a
40 let a=a+1
50 until a==10
60 for a = 1 to 10
70 next
80 print a
list
run
run
20
list
20 do
50
list
EOF

echo test help
"$BASIC" -q <<EOF
help
help about
help commands
help modes
help statements
help blocks
help devices
help expressions
help variables
help pins
help zigflea
EOF

echo test dims
"$BASIC" -q <<EOF
10 dim a as pin a3 for digital output
20 dim b as pin a4 for digital input
30 dim c as pin a1 for analog input
31 dim x as pin a5 for digital output inverted
32 dim y as pin a6 for digital input inverted
33 dim z as pin a2 for analog input inverted
34 dim h as pin b0 for digital output open_drain
35 dim d1 as pin b1 for digital output inverted open_drain
36 dim d2 as pin b2 for digital input debounced inverted
37 dim d3 as pin b3 for digital input debounced 
38 dim d4 as pin a7 for analog input debounced
40 dim d5 as pin a8 for analog input debounced inverted
41 dim d6 as pin b4 for digital output inverted open_drain
42 dim d as flash
43 dim m as remote on nodeid 7
44 dim n[4] as remote on nodeid 8
50 dim e
55 let e=5
60 dim f as flash
70 dim g
80 print a, b, c, d, e, f, g, m, n[3], d1, d2, d3, d4, d5, d6
90 dim abs1 at address 0
91 dim abs2 as byte at address 16
92 dim abs3 as short at address 0x123
93 dim abs4[4] as short at address 0x1048
94 let abs1=1, abs2=2, abs3=3, abs4=4, abs4[1]=5
95 print abs1, hex abs2, abs3, abs4, abs4[1]
list
run
cont
new
10 dim y[0] as pin a0 for digital input
11 dim y[1] as pin a0 for digital input
12 dim y[2] as pin a0 for digital input
13 dim x[10] as pin a0 for digital input
14 dim y[2] as pin a0 for digital input
list
run
new
10 dim x
20 dim leds[1] as pin a3 for digital output
30 dim leds[2] as pin a4 for digital output
40 dim leds[3] as pin a5 for digital output
50 for x = 0 to 8
60   let leds[1] = x&1
70   let leds[2] = x>>1&1
80   let leds[3] = x>>2&1
90 next
list
run
new
10 print "programmatic shift"
20 dim x[1] as pin a3 for digital output
21 dim x[2] as pin a4 for digital output
22 dim x[3] as pin a5 for digital output
23 dim x[4] as pin a6 for digital output
24 dim y
30 for y = 1 to 3
40   if y==4 then
50     let x[y] = 0
60   else
70     let x[y] = x[y+1]
80   endif
90 next
list
run
new
10 print "programmatic shift with index error"
20 dim x[0] as pin a3 for digital output
21 dim x[1] as pin a4 for digital output
22 dim x[2] as pin a5 for digital output
23 dim x[3] as pin a6 for digital output
24 dim y
30 for y = 1 to 4
40   let x[y-1] = x[y]
50 next
list
run
new
  5 print "gosub params"
  10 dim x
  20 dim arr[3]
  30 dim leds[0] as pin a3 for digital output
  40 dim leds[1] as pin a4 for digital output
  50 dim leds[3] as pin a5 for digital output
  60 let x = 2, leds[0] = 1, leds[1] = 1, arr[0] = 1, arr[1] = 2
  70 gosub dump
  80 gosub p x
  90 gosub dump
 100 gosub p leds
 110 gosub dump
 120 gosub p leds[1]
 130 gosub dump
 140 print "setting"
 150 gosub set x
 160 gosub dump
 170 gosub set leds
 180 gosub dump
 190 gosub set leds[1]
 200 gosub dump
 210 gosub set_1 arr
 220 gosub dump
 230 gosub set_1 leds
 240 end 
 250 sub p a
 260   print a
 270 endsub 
 280 sub set a
 290   print a
 300   let a = 0
 310 endsub 
 320 sub dump
 330   print x, leds[0], leds[1], arr[0], arr[1]
 340 endsub 
 350 sub set_1 a
 360   let a[1] = 0
 370 endsub 
list
run
EOF

echo test flash memory
"$BASIC" -q <<EOF
trace on
autorun off
10 dim a as flash
20 dim b[4] as flash
25 print a
30 for a = 0 to 3
40 let b[a] = b[a]+a*a
50 next
60 for a = 0 to 3
70 print a,b[a]
80 next
list
indent
indent on
indent off
list
run
run
autorun
clear flash
run
autorun
prompt
prompt off
prompt on
echo
echo on
echo off
EOF

echo test array parsing
"$BASIC" -q <<EOF
10 dim a[3], b
20 dim b, a[3+4]
30 dim a[3+(4*2)]
40 dim a[(3+4)*2]
50 dim a[(3+4)*2]
60 dim a[(3+4)*2] as byte
70 dim a[(3+4)*2] as flash
80 dim a[(3+4)-3]
110 let a[3] = b[3]
120 let a[3+4] = b[3+4]
130 let a[3+(4*2)] = a[3+(4*2)]
140 let a[(3+4)*2] = a[(3+4)*2]
150 let a[(3+4)*2] = a[(3+4)*2]+2
160 let a[(3+4)*2] = 2*a[(3+4)*2]+2
170 let a[(3+4)*2] = (2*a[(3+4)*2])+2
180 let a[(3+4)*2] = 2*(a[(3+4)*2])+2
190 let a[(3+4)*2] = 2*(a[(3+4)*2]+2)
list
new
10 dim a[4]
20 dim i
30 while i<4 do
40 let a[i] =i*2
50 let i=i+1
60 endwhile
70 let i=0
80 while i<4 do
90 print a[i]
100 let i=i+1
110 endwhile
run
EOF

echo test long variable names
"$BASIC" -q <<EOF
10 dim long
20 dim evenlonger
30 dim muchmuchmuchmuchlonger
40 print long, evenlonger, muchmuchmuchmuchlonger
50 let long=1
60 let evenlonger=2
70 let muchmuchmuchmuchlonger=3
80 print long, evenlonger, muchmuchmuchmuchlonger
90 dim long2 as byte
100 dim evenlonger2 as byte
110 dim muchmuchmuchmuchlonger2 as byte
120 print long2, evenlonger2, muchmuchmuchmuchlonger2
130 print long, evenlonger, muchmuchmuchmuchlonger
list
run
cont
EOF

echo test read/data
"$BASIC" -q <<EOF
   1 dim a, b
  10 data 1, 0x2, 3
  20 data 0x4
  30 data 5, 0x6
  35 data -7
  40 while 1 do
  50   read a
  60   print a
  70 endwhile
  75 data 0x10
  list
numbers off
list
numbers on
run
   1 dim a, b
  10 data 1, 0x2, 3
  20 data 0x4
  30 data 5, 0x6
  35 data -7
  40 while 1 do
  50   read a, b
  60   print a, b
  70 endwhile
  75 data 0x10
  80 restore
  90   read a, b
  100   print a, b
  list
run
cont 80
   1 dim a, b, c
  10 data 1, 0x2, 3
  20 data 0x4
  29 label middle
  30 data 5, 0x6
  35 data 7
  40 while 1 do
  50   read a, b, c
  60   print a, b, c
  70 endwhile
  75 data 0x10
  80 restore middle
  90   read a, b, c
  100   print a, b, c
  list
run
cont 80
EOF

echo test autorun
"$BASIC" -q <<EOF
autorun
autorun on
autorun
autorun on
autorun
autorun off
autorun
EOF

echo test pins
"$BASIC" -q <<EOF
pins
pins aaa
pins heartbeat
pins heartbeat aaa
pins heartbeat a5
pins safemode* none
pins heartbeat a4 aaa
pins safemode* a3 aaa
pins heartbeat
pins safemode*
pins
EOF

echo test servo
"$BASIC" -q <<EOF
servo
servo 30
servo
servo 3000
servo 40 aaa
servo
10 dim x as pin a3 for servo output
list
run
EOF

echo test variable scopes
"$BASIC" -q <<EOF
10 dim a
20 let a=1
25 print a
30 gosub first
40 print a
50 gosub second
60 print a
70 end
90 sub first
100 let a=2
110 print "local a", a
120 return
130 endsub
190 sub second
200 dim a
210 let a=3
220 print "local a", a
230 return
240 endsub
list
renumber
list
renumber 100
list
run
EOF

echo test variable scope overflow
"$BASIC" -q <<EOF
10 dim a
15 dim b[500] as byte
20 gosub alloc
30 end
90 sub alloc
95 dim b[500] as byte
100 let a=a+1
110 if a < 20 then
120 gosub alloc
125 endif
130 endsub
list
92 dim g as flash
list 92
run
92 dim g as pin a3 for digital output
list 92
run
92
run
print a
15 for a = 1 to 10
25 next
delete 100-125
96 print b[499]
list
run
EOF

echo test while breaks
"$BASIC" -q <<EOF
10 dim a
20 while 1 do
30 if a==10 then
40 break
50 endif
60 let a=a+1
70 endwhile
80 print a
90 end
list
run
19 while 1 do
71 let a=a+1
72 endwhile
40 break 2
list
run
EOF

echo test while continues
"$BASIC" -q <<EOF
  10 dim i
  20 while i<15 do
  30   let i=i+1
  40   sleep 100 ms
  50   if i%5==0 then
  60     continue
  70   endif
  80   print i
  90 endwhile
list
run
11 dim j
45 for j = 1 to 3
46 print "   ",j
50 if j==2&&i%5==0 then
60 continue 2
62 print "not"
75 next
list
run
EOF

echo test for loops
"$BASIC" -q <<EOF
10 dim x,y
20 for y = 0 to 10 step 2
30 for x = 1 to 2
40 print y+x
50 next
60 next
list
run
41 if y==7 then
42 break
43 endif
list
run
41 if y==8 then
list
run
42 break 2
list
run
EOF

echo test arrays
"$BASIC" -q <<EOF
   1 dim i, j
   2 dim a[256]
   3 while 1 do
  20   let i = (i*13+7)%256
  21   let j=j+1
  22   if a[i] then
  23     stop
  24   endif
  26   let a[i] = 1
  30 endwhile
list
run
print j
2 dim a[256] as byte
list
run
print j
memory
clear
memory
clear flash
EOF

echo test large flash program
"$BASIC" -q <<EOF
demo 3 1000
demo 3 2000
demo 3 3000
memory
list
save
memory
list
demo 3 4000
demo 3 5000
demo 3 6000
memory
list
save
memory
list
demo 3 7000
demo 3 8000
demo 3 9000
memory
renumber
list
save
memory
renumber
list
save
new
memory
EOF

echo test configures
"$BASIC" -q <<EOF
10 configure uart 1 for 9600 baud 8 data even parity
20 configure uart 2 for 115200 baud 7 data no parity
30 configure uart 2 for 1200 baud 6 data odd parity loopback
40 configure timer 0 for 1000 s
50 configure timer 1 for 10 ms
55 configure timer 2 for 17 us
70 qspi a,b,c,d
list
new
demo2
list
EOF

echo test qspi error case
"$BASIC" -q <<EOF
  10 dim cmd1 as byte
  20 while 0>500 do
  30   qspi cmd1
  40 endwhile
  50 end
EOF

echo test uart
"$BASIC" -q <<EOF
demo1
list
run
EOF

echo return from sub scope
"$BASIC" -q <<EOF
  10 gosub output
  20 end
 140 sub output
 150   if 1!=8 then
 151     return
 152   endif
 210 endsub
 list
 run
EOF

echo if, elseif, else
"$BASIC" -q <<EOF
10 dim a
20 for a = -5 to 5
30   if !a then
40     print a, "is zero"
50   elseif a%2 then
60     print a, "is odd"
70   else
80     print a, "is even"
90   endif
100 next
list
run
EOF

echo filesystem
"$BASIC" -q <<EOF
10 rem this is a program
20 rem 1
save prog1
20 rem 2
save prog2
20 rem 3
save prog3
20 rem 4
save prog4
20 rem 22
save prog2
20 rem 44
save prog4
dir
list
load prog1
list
load prog2
list
load prog3
list
load prog4
list
purge prog0
dir
list
purge prog1
dir
list
purge prog2
dir
list
20 rem 444
save prog4
purge prog3
dir
list
purge prog5
dir
list
new
load prog4
list
20 rem long
save this is a very long program name
save this is a very longer program name
dir
load prog4
list
load this is a very long xxx
dir
list
purge this is a very long yyy
dir
list
save j1
save j2
save j3
save j4
save j5
save j6
save j7
save j9
save j10
save j11
save j12
EOF

### on/off/mask/unmask ###

echo on/off/mask/unmask
"$BASIC" -q <<EOF
10 on xxx do gosub yyy
20 on zzz+aaa do stop
30 off xxx
40 off zzz+aaa
50 unmask xxx
60 unmask zzz+aaa
70 on timer 1 do gosub tick
80 on uart 1 input do gosub rx
90 off timer 1
100 off uart 1 input
list
EOF

### watchpoints ###

echo watchpoints
"$BASIC" -q <<EOF
watchsmart
watchsmart off
watchsmart
watchsmart on
watchsmart
10 dim x
20 on x do print 1
30 on x+0 do print 2
40 on x do print 3
list
run
new
10 dim x
20 on x+1 do print 1, x
30 on x+2 do print 2, x
40 on x+3 do print 3, x
50 on x+4 do print 4, x
60 on x+5 do print 5, x
list
run
new
watchsmart off
10 dim x
20 on x+1 do print 1, x
30 on x+2 do print 2, x
40 on x+3 do print 3, x
50 on x+4 do print 4, x
70 let x=1
80 print "80"
90 let x=-1
100 let x=2
110 print "110"
120 let x=-2
130 let x=1
140 print "140"
150 let x=-3
160 let x=1
170 print "170"
180 let x=-4
190 let x=1
200 print "200"
210 let x=-5
220 let x=1
list
run
new
watchsmart on
10 dim x
20 on x+1 do print 1, x
30 on x+2 do print 2, x
40 on x+3 do print 3, x
50 on x+4 do print 4, x
70 let x=1
80 print "80"
90 let x=-1
100 let x=2
110 print "110"
120 let x=-2
130 let x=1
140 print "140"
150 let x=-3
160 let x=1
170 print "170"
180 let x=-4
190 let x=1
200 print "200"
210 let x=-5
220 let x=1
list
run
new
10 dim i
20 on i%5 do stop
30 while 1 do
40 let i=i+1
50 let i=i+1
51 if i==100 then
52 break
53 endif
60 endwhile
list
run
print i
cont
print i
off i%5
cont
print i
new
10 dim x
20 on x==1 do print "ONE",x
21 on x%2 do print "odd",x
30 for x = 1 to 10
31 next
40 print "masking"
41 mask x==1
42 for x = 1 to 10
43 next
70 print "unmasking"  
71 unmask x==1
72 for x = 1 to 10
73 next
80 print "off-ing"  
81 off x==1
82 for x = 1 to 10
83 next
list
run
new
10 dim x as flash
20 on x!=0 do print 20
30 on x==0 do print 30
40 let x=1
50 print "done"
list
run
run
EOF

### long prints ###

echo long prints
"$BASIC" -q <<EOF
10 dim i, j
11 let i=1
12 let j=1
20 while (i < 1000000000) do
25 let i=i*10
26printj,"---------1---------2---------3---------4---------5--------",i
30 endwhile
list
run
12 let j=1000000000
list
run
12 let j=1
26printi,j,"---------1---------2---------3---------4---------5--------"
list
run
12 let j=1000000000
list
run
EOF

### parse errors ###

echo parse errors
"$BASIC" -q <<EOF
print 3**3
print *3
print 3*
print 3+
print 3!
print a[3!]
print a[3
print a[3]]
print a[b[3**3]]
print (3
print 3)
print +
print *
print !
EOF

### runtime errors ###

echo runtime errors
"$BASIC" -q <<EOF
print a[b[3]]
print 3/0
print 3%0
assert 1
assert 0
dim a
dim a as byte
print a
print b
print a[1]
let b=0
let a[1] = 0
EOF

### pin types ###

echo pin types
"$BASIC" -q <<EOF
10 dim audio as pin a3 for frequency output
20 dim voltage as pin a4 for analog output
30 dim audio2 as pin b0 for frequency output
40 dim voltage2 as pin b1 for analog output
50 dim xxx as pin a5 for frequency input
60 dim xxx as pin a6 for frequency inpuy
list
EOF

### statement errors ###

echo statement errors
"$BASIC" -q <<EOF
print "on"
on xxx
on timer a
on timer do a
on timer 1
on timer 1 do
on timer 1 do xxx
on timer 1 do let xxx =
on timer 1 do let xxx = *
on timer 4 do let a=0
on timer 1 do let a=0
on uart a
on uart 1
on uart 1 xxx
on uart 1 input a
on uart 1 input do
on uart 1 input do xxx
on uart 1 input do let xxx =
on uart 1 input do let xxx = *
on uart 4 input do let a=0
on uart 1 input do let a=0
print "off"
off xxx
off timer
off timer 1 xxx
off timer 4
off timer 1
off uart
off uart 1 input xxx
off uart 4 input
off uart 1 input
print "mask"
mask xxx
mask timer
mask timer 1 xxx
mask timer 4
mask timer 1
mask uart
mask uart 1 input xxx
mask uart 4 input
mask uart 1 input
print "configure"
configure xxx
configure timer
configure timer 0
configure timer 0 for
configure timer 0 xxx
configure timer 0 for 12
configure timer 0 for 12 xxx
configure timer 5 for 12 ms
configure timer 1 for 12 ms
configure uart
configure uart 1
configure uart 1 for
configure uart 1 xxx
configure uart 1 for 12
configure uart 1 for 12 xxx
configure uart 1 for 12 baud
configure uart 1 for 12 baud 7
configure uart 1 for 12 baud 1 data
configure uart 1 for 12 baud 7 data
configure uart 1 for 12 baud 7 data xxx
configure uart 1 for 12 baud 7 data even
configure uart 1 for 12 baud 7 data even xxx
configure uart 1 for 12 baud 7 data even parity xxx
configure uart 1 for 12 baud 1 data even parity
configure uart 4 for 12 baud 7 data even parity
print "assert"
assert
assert3,
assert(3
print "read"
read
read aaa,
read (aaa)
read 3
print "data"
data
data q
data ,10
data 10,
data (10)
print "label"
label
print "restore"
restore xxx
print "dim"
dim
dim ,
dim 0
dim a,
dim a www
dim a byte
dim a as xx
dim a as byte a
dim 0 as byte
dim a[]
dim a[(0]
dim a as flash b
dim a as byte flash
dim a as byte
dim a as flash
print "pin 0"
dim b as pin www
dim b as pin a0
dim b as pin a0 xxx
dim b as pin a0 for
print "analog 1"
dim b as pin a1 for analog input xxx
dim b as pin a1 for analog yyy xxx
dim b as pin a0 for yyy input xxx
print "analog 2"
dim b as pin a1 for analog input
dim b as pin a1 for analog input xxx
dim b as pin a1 for analog input debounced xxx
print "frequency"
dim c as pin a3 for frequency input
dim d as
print "digital io"
dim d as pin b0 for digital input xxx
dim d as pin b0 for digital input debounced xxx
dim d as pin b0 for digital input inverted xxx
dim d as pin b0 for digital input debounced inverted xxx
dim d as pin b0 for digital output open_drain xxx
dim d as pin b0 for digital output debounced open_drain xxx
dim d as pin b0 for digital output inverted open_drain xxx
dim d as pin b0 for digital output debounced inverted open_drain xxx
print "remote"
dim ee as remote
dim eee as remote on
dim eeee as remote on x
dim e as remote on nodeid
dim f[4] as remote on nodeid
dim g as remote on nodeid 3+
print "absolute vars"
dim abs1 at
dim abs2 at xxx
dim abs3 at address xxx
dim abs4 at address 0x20 xxx
dim abs5 as
dim abs6 as xxx
dim abs7 as byte xxx
dim abs8 as byte at
dim abs9 as byte at xxx
dim abs10 as byte at address
dim abs11 as byte at address xxx
dim abs12 as byte at address 0x30 xxx
dim abs13 as short xxx
dim abs14 as short at
dim abs15 as short at xxx
dim abs16 as short at address
dim abs17 as short at address xxx
dim abs18 as short at address 0x40 xxx
dim abs19[4] at
dim abs20[4] at xxx
dim abs21[4] at address xxx
dim abs22[4] at address 0x50 xxx
dim abs23[4] as byte at
dim abs24[4] as byte at xxx
dim abs25[4] as byte at address xxx
dim abs26[4] as byte at address 0x60 xxx
dim abs27[4] as short at
dim abs28[4] as short at xxx
dim abs29[4] as short at address xxx
dim abs30[4] as short at address 0x70 xxx
print "let"
let
let 0=0
let 0
let a
let a=
let a=()
let a=*
lat
let a=0,
print "print"
print
print ()
print "a
print a"
print ",0
print ,"
print ,0"
print 0,0"
print 0,"0
print "a",
print 0,0 0
print ("
print "(
print "if"
if
if 0 do
if then
if () then
if 0 then a
if 0, then
print "while"
while
while 1 then
while do
while () do
while 0 do a
while 0, do
print "elseif"
elseif
elseif 0 do
elseif then
elseif () then
elseif 0 then a
elseif 0, then
print "solos"
elsea
endif0
endwhile+
print "break"
break
break a
break ,
break 100,
print "for"
for
for x
for x =
for x = 1
for x = 1 to
for x = 1 to 10
for x = to 10
for x = 1  10
for x = 1 to 
for x = 1, to 10
for x, = 1 to 10
for x = 1 to 10 step
for x = 1 to 10 step 1,
for x = 1 to 10 step 1a
print "sleep"
10 sleep xxx
11 sleep 17
13 sleep 18 xxx
14 sleep 19 ms xxx
15 sleep 20 us xxx
print "more"
nexti
next0
gosub
sub
return0
endsub+
sleep+
sleep0,
sleep,
stop1
end1
watchsmart xxx
watchsmart on xxx
watchsmart off xxx
watchsmart xxx on
watchsmart xxx off
EOF

### command errors ###

echo command errors
"$BASIC" -q <<EOF
print "autorun"
autorun aaa
autorun on aaa
print "clear"
clear aaa
clear flash aaa
print "clone"
clone aaa
clone run aaa
print "cont"
cont aaa
cont 100 aaa
print "delete"
delete aaa
delete aaa-100
delete 100-aaa
delete 100+110
delete 100-110 aaa
delete -100 aaa
delete 100- aaa
delete
print "dir"
dir a
print "help"
help aaa
help about aaa
print "list"
list aaa
list aaa-100
list 100-aaa
list 100+110
list 100-110 aaa
list -100 aaa
list 100- aaa
list
print "load"
load aaa
load aaa,aaa
print "memory"
memory aaa
print "new"
new aaa
print "purge"
purge aaa
purge aaa,aaa
print "renumber"
renumber aaa
renumber 10 aaa
print "reset"
reset aaa
print "run"
run 10 aaa
run aaa
run 10+
print "save"
save aaa,aaa
dir
print "step"
step aaa
step on aaa
print "trace"
trace aaa
 trace on aaa
print "undo"
undo aaa
print "uptime"
uptime +
EOF

### overflow tests ###

echo overflow tests
"$BASIC" -q <<EOF
print !!!!!!!!!!0
print !!!!!!!!!!1
print !!!!!!!!!!!0
print !!!!!!!!!!!!0
10 dim ticks
20 configure timer 0 for 200 ms
30 on timer 0 do gosub tick
40 gosub main
50 end
60 sub main
70   sleep 400 ms
75   print "going"
80   gosub main
90 endsub
100 sub tick
110   let ticks = ticks+1
120 endsub
run
new
10 dim ticks
20 configure timer 0 for 200 ms
30 on timer 0 do gosub tick
40 gosub main
50 end
60 sub main
65 if 1 then
66 if 1 then
70   sleep 400 ms
75   print "going"
80   gosub main
81 endif
82 endif
90 endsub
100 sub tick
110   let ticks = ticks+1
120 endsub
run
65 while 1 do
66 while 1 do
81 endwhile
82 endwhile
new
run
10 gosub main
20 end
30 sub main
40 print "main"
50 dim a,b,c,d,e,f,g,h,i,j,k,l,m,n
60 gosub main
70 endsub
run
new
10 dim b[200]
20 dim c[200]
30 dim d[200]
40 dim e[200]
50 dim f[200]
60 dim i[200]
70 dim j[200]
run
new
demo3
demo3 1000
demo3 2000
demo3 3000
save
demo3 4000
demo3 5000
demo3 6000
demo3 7000
demo3 8000
demo3 9000
memory
save
list
memory
demo3 10000
demo3 11000
demo3 12000
demo3 13000
demo3 14000
demo3 15000
demo3 16000
demo3 17000
demo3 18000
demo3 19000
demo3 20000
memory
demo3 21000
memory
demo3 22000
save
memory
demo3 23000
memory
demo3 24000
memory
demo3 25000
demo3 26000
demo3 27000
demo3 28000
save
memory
list
print "delete line 10"
10
memory
print "delete line 24000"
24000
memory
print "delete line 24000-"
delete 24000-
memory
print "delete line 22000-"
delete 22000-
memory
print "delete line -1000"
delete -1000
memory
print "save"
save
memory
list
EOF

### bad blocks ###

echo bad blocks
"$BASIC" -q <<EOF
10 while 1 do
run
10 endwhile
run
10 if 1 then
run
10 else
run
10 elseif 1 then
run
10 endif
run
1 dim i
10 for i = 1 to 10
run
10 next
run
10 if 1 then
20 elseif 1 then
run
30 else
run
10 sub aaa
run
10 endsub
run
10 return
run
10 break
run
10 while 1 do
20 break 2
30 endwhile
run
EOF

### negative run conditions

echo test negative run conditions
"$BASIC" -q <<EOF
10 if 0 then
15 let a[7] = b[7]
16 print c[7]
20 print 3/0
30 print "hello",1,"there"
40 configure timer 1 for 1ms
50 on timer 1 do gosub aaa
60 sleep 100000000 ms
70 endif
80 sleep 100 ms
90 configure timer 1 for 700ms
100 on timer 1 do print "tick"
110 if 0 then
120 unmask timer 1
130 mask timer 1
140 off timer 1
145 configure timer 1 for 10ms
150 endif
160 sleep 1000 ms
170 off timer 1
175 dim i
176 read i
180 if 0 then
190 assert 0
200 assert 3/0
210 read aaa
220 read bbb,ccc
230 restore
240 data 1,2
250 endif
260 read i
270 print i
280 if 0 then
290 dim a
300 dim a as byte
310 dim b[3000]
320 let q=1
330 print q
340 endif
350 while 1 do
360 if 0 then
365 end
366 stop
370 print "not"
380 break
390 else
400 print "yes"
410 break
415 endif
420 endwhile
run
EOF

### long line trimming ###

echo test long line trimming
"$BASIC" -q <<EOF
10 if 1 then
20 if 1 then
30 if 1 then
40 if 1 then
50 if 1 then
60 if 1 then
70rem01234567890123456789012345678901234567890123456789012345678901234567
1rem01234567890123456789012345678901234567890123456789012345678901234567
80 endif
90 endif
100 endif
110 endif
120 endif
130 endif
list
61 if 1 then
71 endif
list
edit 61
upgrade
EOF

### slow tests follow ###

echo test interrupt masking
"$BASIC" -q <<EOF
10 configure timer 0 for 500 ms
20 on timer 0 do print "tick"
30 sleep 750 ms
40 mask timer 0
50 sleep 2 s
60 unmask timer 0
70 sleep 250 ms
80 off timer 0
 100 configure uart 1 for 300 baud 8 data no parity loopback
 110 dim tx as pin b8 for uart output
 120 dim rx as pin b6 for uart input
 140 on uart 1 input do print "rx", rx
 145 on uart 1 output do print "txed"
 150 let tx = 3
 160 sleep 500 ms
 170 mask uart 1 input
 180 let tx = 4
 190 sleep 500 ms
 200 print "unmasking"
 210 unmask uart 1 input
 220 sleep 500 ms
 230 off uart 1 input
 240 let tx = 5
 250 sleep 500 ms
 260 print "poll", rx
list
run
EOF

echo test timers
"$BASIC" -q <<EOF
1 configure timer 0 for 3500 ms
2 on timer 0 do gosub seven
9 configure timer 1 for 1000 ms
10 on timer 1 do print 2
20 sleep 500 ms
29 configure timer 2 for 2000 ms
30 on timer 2 do print 4
31 sleep 1 ms  
32 configure timer 3 for 2000000 us
33 on timer 3 do print "t"
40 sleep 5000 ms
50 end
90 sub seven
100 print "seven"
110 return
120 endsub
list
run
EOF

echo test ticks/msec
"$BASIC" -q <<EOF
1 dim m, t
2 let m = msecs, t = ticks
10 print "ticks/msec:", ticks_per_msec
20 configure timer 0 for 3000 ms
30 on timer 0 do print "3000 ms timer at",msecs-m,"ms",ticks-t,"t" 
40 configure timer 1 for 1750000 us
50 on timer 1 do print "1750000 us timer at",msecs-m,"ms",ticks-t,"t" 
90 sleep 10000 ms
110 let ticks_per_msec=1
list
run
EOF

echo test time units
"$BASIC" -q <<EOF
10 sleep 1 s
20 sleep 2*2 ms
30 sleep 3000000 us
40 configure timer 1 for 1 s
50 configure timer 2 for 10 ms
60 configure timer 3 for 100 us
save
list
delete 40-
save
run
profile
EOF

echo test sub parameters
"$BASIC" -q <<EOF
 10 print "sub params1"
 20 dim i
 30 assert i==0
 40 gosub f i
 50 assert i==1
 60 print "passed"
 70 end 
 80 sub f x
 90   let x = x+1
100 endsub 
list
run
new
5 print "sub params2"
10 dim a
20 let a = 7
30 assert a==7 && ticks_per_msec==4
40 gosub f a, 3, ticks_per_msec+0
50 assert a==8 && ticks_per_msec==4
60 let a = 12
70 gosub f a+0, 3, 4
80 assert a==12 && ticks_per_msec==4
90 gosub f a, a, a
100 assert a==2
110 print "passed"
120 end
130 sub f x, y, z
140   print "f1:", x+y, z
150   let x = 8
160   let y = 9
170   let z = 2
180   print "f2:", x+y, z
190 endsub
list
run
new
5 print "sub params2.1"
10 dim a
20 gosub foo a, a
30 end
50 sub foo a, a
60 endsub
list
run
new
 5 print "sub params3"
 20 assert ticks_per_msec==4
 30 gosub f ticks_per_msec+0
 40 assert ticks_per_msec==4
 50 print "passed"
 60 end 
 90 sub f x
100   let x = x*2
110   print x, ticks_per_msec
120 endsub 
list
run
new
 10 print "sub params4"
 20 dim a[2]
 25 dim b[2]
 30 let a[0] = 5, a[1] = 6
 40 assert a[0]==5&&a[1]==6
 50 gosub f a[0]+0, a[1]
 60 assert a[0]==5&&a[1]==6
 70 gosub f2 a
 80 assert a[0]==6&&a[1]==7
 85 gosub foo a, b
 87 print a[0], b[1]
 88 assert a[0]==0 && b[1]==6
 90 print "passed"
 100 end 
 110 sub f x, y
 120   print "fa", x, y
 130   let x = x+1
 140   let y = y+1
 150   print "fb", x, y
 160 endsub 
 170 sub f2 z
 180   let z[0] = z[0]+1
 190   let z[1] = z[1]+1
 200 endsub
 300 sub foo i, j
 310   if i > 0 then
 320     let i = i - 1
 330     let j[1] = j[1] + 1
 340     gosub foo i, j
 350   endif
 360 endsub
list
run
new
10 print "sub params - call with too many params1"
20 gosub f
30 print "ok"
40 gosub f ticks_per_msec
50 end 
60 sub f
70 endsub 
list
run
new
10 print "sub params - call with too many params2"
20 dim a
30 gosub f ticks_per_msec+0
40 print "ok"
50 gosub f ticks_per_msec+0, a
60 stop 
70 sub f x
80 endsub 
list
run
new
 10 print "sub pin params"
 20 dim p as pin b0 for digital output
 30 dim i
 40 for i = 1 to 10
 50   gosub flip p, i%i
 60 next 
 70 print "passed"
 80 end 
 90 sub flip o, v
100   let o = v
110 endsub 
list
run
new
 10 print "sub param array by-ref"
 20 dim x[3], b[2]
 30 let x[0] = 1, x[1] = 2, x[2] = 3
 31 let b[0] = 11, b[1] = 1
 40 assert x[0]==1&&x[1]==2&&x[2]==3
 50 gosub f x
 60 assert x[0]==10&&x[1]==20&&x[2]==30
 70 print "pass1"
 75 gosub f b
 80 stop 
 90 sub f a
 95   print a[0]
100   let a[0] = 10
110   let a[1] = 20
120   let a[2] = 30
130 endsub 
list
run
new
  10 print "sub param nested type error"
  20 dim a[2]
  30 gosub f a
  40 gosub f a[1]
  50 end 
  60 sub f x
  70   print "f", x
  80   gosub y x
  90 endsub 
 100 sub y p
 110   print "y", p[1]
 120 endsub 
list
run
new
 10 print "sub param with same-name local"
 20 dim a
 30 gosub f a
 40 stop
 50 sub f x
 60   dim x[2]
 70 endsub
list
run
EOF

echo test demo
"$BASIC" -q <<EOF
demo 0
list
rem run
demo 1
list
run
demo 2
list
run
demo 3
120 sleep 500 ms
list
trace on
run
EOF

