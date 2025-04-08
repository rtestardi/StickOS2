#! /bin/sh
# this test exercises new v1.80 features

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

echo input parse
"$BASIC" -q <<'EOF'
10 dim a, b$[20]
20 input a, b$
30 print a*2, b$
list
run
12 hello world!
EOF

echo input arrays
"$BASIC" -q <<'EOF'
10 dim a, b[3], c[2]
20 input a, b, c
30 print a, b, c
list
run
12 34 56 78 90
EOF

echo input strings
"$BASIC" -q <<'EOF'
10 dim a$[10]
20 input a$
30 print a$
40 print "end"
list
run
hello world!
cont
run
bye
EOF

echo input overflows, garbage
"$BASIC" -q <<'EOF'
10 dim a, b[2]
20 input a, b
30 print a, b
list
run
1 2 3 4
cont
run
a b
cont
EOF

echo getchar
"$BASIC" -q <<'EOF'
10 print getchar
list
run
EOF

echo print raw
"$BASIC" -q <<'EOF'
10 dim a$[15]
20 let a$="hello world!"
30 print raw a
35 print hex a
36 print dec a
40 print a$, a#
list
run
50 dim b[15] as byte
60 let b$="world hello!"
70 print raw b
75 print hex b
76 print dec b
80 print b$, b#
list 50-
cont
EOF

echo more print raw
"$BASIC" -q <<'EOF'
   10 dim an[10] as byte, j
   20 for j = 0 to 9
   30   let an[j] = j+48
   40 next
   45 configure timer 0 for 1 s
   46 configure timer 1 for 3500ms
   50 on timer 0 do print raw an
   51 on timer 1 do stop
   60 halt
   list
   trace on
   run
EOF

echo print raw multibyte
"$BASIC" -q <<'EOF'
10 dim a[3] as short, b[3], c[3] as byte
20 input a, b, c
30 print raw a, b, c
40 print hex a, b, c
list
trace on
run
0x4142 0x4344 0x4546 0x30313233 0x34353637 0x38393a3b 0x61 0x62 0x63
EOF

echo print semicolon
"$BASIC" -q <<'EOF'
1 dim a$
2 let a$=" "
10 print 1,2,3;
20 print "4"+a$+"5"+a$+"6"
30 print ";";
40 print ";"
50 print 2 ; 3
list
run
EOF


echo strings
"$BASIC" -q <<'EOF'
dim a$[80]
10 dim a$[79]
20 let a$="hello"+" "+"world!"
30 let a$=a$[0+0:5]+a$+a$[6:3+3]
40 print a$, a$[9:3]
list
run
EOF

echo substrings
"$BASIC" -q <<'EOF'
dim a$[10]
let a$="0123456789"
print a$[-1:3], a$[8:3]
print a$[-1:12]
print "not", a$[1:-1]
print a#
10 print a$[0:a#]
list
cont
EOF

echo vprint
"$BASIC" -q <<'EOF'
10 dim zz$[30]
20 dim aa
30 vprint zz$ = "hello", 2*2, "there!"
40 print zz$
50 vprint zz = 10*10
60 print zz
70 print raw zz
80 vprint aa = zz$[6:3]
list
run
80 vprint aa = zz$[6:2]
90 print aa
list 80-
cont 80
EOF

echo character constants
"$BASIC" -q <<'EOF'
10 dim a[5] as byte
20 dim b$[5]
30 let a[0] = 'h', b[0] = 'b'
40 let a[1] = 'e', b[1] = 'y'
50 let a[2] = 'l', b[2] = 'e'
60 let a[3] = 'l', b[3] = 0
70 let a[4] = 'o', b[4] = 0
80 print a#, a$, a, a$[0:1]
90 print b#, b$, b, b$[0:1]
list
run
EOF

echo long strings
"$BASIC" -q <<'EOF'
10 dim a$[79]
20 dim b$[10]
30 let b$="0123456789"
40 let a$=b$+b$
50 let a$=a$+a$
60 let a$=a$+a$
70 let a$=a$+a$
80 print a#, a$
90 print a$
100 print b#, b$
110 print b$
list
run
EOF

echo numeric extraction
"$BASIC" -q <<'EOF'
10 dim a, b$[10]
20 let a=4788
30 vprint b$=a
40 vprint a=b$[1:2]
50 print a*2
list
run
EOF

echo configure timer expressions
"$BASIC" -q <<'EOF'
1 dim m, o
2 let o=msecs
10 configure timer 1 for 2+2 s
20 on timer 1 do gosub done
30 sleep 3+3s
40 end
50 sub done
60 sleep 500ms
70 print (msecs-o)/1000
80 endsub
list
run
60 let m = msecs
61 while msecs <m+500 do
62 endwhile
list
run
EOF

echo i2c transfers
"$BASIC" -q <<'EOF'
10 dim a as byte, b as short, c[3], d$[4], e[4] as byte
20 let a=1,b=0x0203,c[0]=0x11223344,c[1]=0x55667788
25 let c[2]=0x99aabbcc
30 let d$="hi", e[0] = 's', e[1]='t', e[2]='o', e[3]='p'
40 i2c start 2+2
50 i2c write a,b
60 i2c write c,d
70 i2c write e,c[0]
80 i2c read a,b
90 i2c read c,d
100 i2c read e,c[0]
110 i2c stop
list
run
EOF

echo qspi transfers
"$BASIC" -q <<'EOF'
10 dim a as byte, b as short, c[3], d$[4], e[4] as byte
20 let a=1,b=0x0203,c[0]=0x11223344,c[1]=0x55667788
25 let c[2]=0x99aabbcc
30 let d$="hi", e[0] = 's', e[1]='t', e[2]='o', e[3]='p'
50 qspi a,b
60 qspi c,d
70 qspi e,c[0]
list
run
EOF

echo uart transfers
"$BASIC" -q <<'EOF'
10 dim a as byte, b as short, c[3], d$[4], e[4] as byte
20 let a=1,b=0x0203,c[0]=0x11223344,c[1]=0x55667788
25 let c[2]=0x99aabbcc
30 let d$="hi", e[0] = 's', e[1]='t', e[2]='o', e[3]='p'
50 uart 1 write a,b
60 uart 2 write c,d
70 uart 1 write e,c[0]
80 uart 2 read a,b
90 uart 1 read c,d
100 uart 2 read e,c[0]
list
run
EOF

echo parse errors strings
"$BASIC" -q <<'EOF'
10 dim a$[1:2]
11 dim a$[1]+
12 dim a$[1],
20 let a$[1:2] = "hello"
30 print a$[1]
40 let a$[1] = bye
50 let a$ = bye;
60 print a$[1];
70 if a$[1] = a$ then
80 vprint a$[1:2] = 0
90 vprint a$[1] = 0
91 vprint a$[1] = 0+
92 vprint a$[1] = 0,
93 vprint a$[1]] = 0
94 vprint a$[1]+ = 0
95 vprint a$[1], = 0
96 vprint a$[[1] = 0
100 input a$[1]
110 input a$[1:2]
120 input a$;
130 input a$+
140 input a$,
list
EOF

echo parse errors i2c, qspi, uart
"$BASIC" -q <<'EOF'
1 i2c
2 i2c xxx
10 i2c start 3+
11 i2c start 3,
12 i2c start
20 i2c read
21 i2c read a,
22 i2c read a+
30 i2c write
31 i2c write a,
32 i2c write a+
40 i2c stop 1
50 qspi
51 qspi a,
52 qspi a+
60 uart 2 read
61 uart 1 read a,
62 uart 2 read a+
70 uart 1 write
71 uart 2 write a,
72 uart 1 write a+
80 uart 9 read a
81 uart 2+1 read a
82 uart 2, read a
83 uart read a
list
EOF

echo runtime errors immediate
"$BASIC" -q <<'EOF'
dim a$[10]
dim b
let a$="12b"
vprint b=a$
print b
let a$="12345678901234567890"
print a$
vprint b=1,1
EOF

echo runtime errors program
"$BASIC" -q <<'EOF'
10 dim a$[10]
20 dim b
30 input a$
40 vprint b=a$
50 print b
60 input a$
70 print a$
80 vprint b=1,1
list
run
12b
cont
12345678901234567890
cont
EOF

echo test isr sleeps
"$BASIC" -q <<'EOF'
  10 dim i, s, start, tocks
  15 let s=1
  20 configure timer 1 for 3 s
  30 on timer 1 do gosub tock
  40 for i = 1 to 20
  50   let start = msecs
  60   sleep 1*s s
  70   assert msecs>start+800&&msecs<start+1700
  80 next
  90 assert tocks>=5&&tocks<=8
 100 end
 110 sub tock
 120   dim start
 130   let start = msecs
 140   sleep 500*s ms
 150   assert msecs>start+300&&msecs<start+700
 160   let tocks = tocks+1
 170 endsub
 list
 run
 15
 run
EOF

echo test basic arrays
"$BASIC" -q <<'EOF'
10 dim a[10]
20 dim i, j
30 for i = 0 to 9
40 for j = i to 9
50 let a[j] = a[j]+i
60 next
70 next
80 for i = 0 to 9
90 print i, a[i]
100 next
list
run
EOF

echo test recursion
"$BASIC" -q <<'EOF'
  10 dim t
  20 gosub sumit 7, t
  30 print t
  40 end
  50 sub sumit x, y
  60   dim z
  70   if x==1 then
  80     let y = 1
  90     return
 100   endif
 110   gosub sumit x-1, z
 120   let y = z+x
 130 endsub
 list
 run
EOF

echo test too many scopes/gosubs
"$BASIC" -q <<'EOF'
10 gosub sub
20 sub sub
29 print "sub"
30 gosub sub
40 endsub
list
run
21 if 1 then
22 if 1 then
31 endif
32 endif
list
run
EOF

echo test analog
"$BASIC" -q <<'EOF'
print analog-1
analog 3400
analog
print analog+1
EOF

echo test comments
"$BASIC" -q <<'EOF'
print 1 // nocomment
1 //nocomment
10 print 1 //comment
20 print "//nocomment"
30 print "" //comment
save
list
run
EOF

echo test noprint and nolet
"$BASIC" -q <<'EOF'
?1+2
10 ? 2+3
20 print 3+4
30 dim a,b,c
40 a=5
50 b=6,c=7
60 print a,b,c
list
run
EOF

echo test random
"$BASIC" -q <<'EOF'
10 dim a
20 for a = 1 to 4
30 print random // random
40 next
list
run
EOF

echo test library
"$BASIC" -q <<'EOF'
10 sub sub1
20 print "hello"
25 gosub again
30 endsub
31 sub again
32 print "again"
33 endsub
40 sub sub2 a,b
50 a=b*2
51 gosub sub3 0
60 endsub
subs
save library
subs
?"new"
new
subs
200 sub sub3 a
220 print "bye", a
230 endsub
40 dim i
50 gosub sub1
60 gosub sub2 i,2
70 print i
80 gosub sub3 1
100 end
list
run
90 gosub sub4
list
run
cont
subs
?"sub1"
list sub1
?"sub2"
list sub2
?"sub3"
list sub3
?"sub4"
list sub4
90
run
?"purge"
purge library
subs
run
EOF

echo test library recursion
"$BASIC" -q <<'EOF'
  50 sub sumit x, y
  60   dim z
  70   if x==1 then
  80     let y = 1
  90     return
 100   endif
 110   gosub sumit x-1, z
 120   let y = z+x
 130 endsub
save library
new
  10 dim t
  20 gosub sumit 7, t
  30 print t
  40 end
list sumit
?"main:"
list
run
EOF

echo test library read/data
"$BASIC" -q <<'EOF'
10 sub doit1
15 dim a
20 restore liblabel
30 read a
40 print "doit1", a
50 endsub
51 sub doit2
52 read a
53 print "doit2", a
59 endsub
60 label liblabel // space
70 data 123, 789
save library
new
1 gosub doit1
15 dim a
20 restore mainlabel
30 read a
40 print "main", a
41 gosub doit2
50 end
60 label mainlabel 
70 data 456
list doit1
list doit2
?"main:"
list
run
EOF

echo test library profile
"$BASIC" -q <<'EOF'
10 sub other
20 sleep 1 s
30 endsub
save library
new
subs
10 gosub other
save
run
profile
run
profile
EOF

echo test norem
"$BASIC" -q <<'EOF'
10 rem // hello
20 // bye
list
run
EOF

echo stop/continue in library
"$BASIC" -q <<'EOF'
10 sub doit
20 stop
25 print "library running"
30 endsub
save library
new
10 gosub doit
1 print "running"
list doit
?"main:"
list
run
cont
run
cont 30
run
purge library
list doit
?"main:"
list
run
EOF

echo _ in variable names
"$BASIC" -q <<'EOF'
10 dim _abc
20 _abc=2
30 let _abc=_abc+1
40 print _abc
50 restore xxx
60 label yyy
list
run
cont
EOF

echo wave statements
"$BASIC" -q <<'EOF'
10 dim pot
20 pot=3000
30 wave square pot*2
40 wave sine pot
50 wave triangle pot*3
60 wave ekg 0
70 wave xxx 0
80 wave square
list
run
EOF


exit 0
# XXX -- move this to basic2, along with:

15 dim a1 as pin rd9 for digital output
25 dim a2 as pin rd11 for digital output
35 dim a3 as pin rd15 for digital output
list
pins heartbeat an10
pins
