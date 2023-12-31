// GENERATE_HELP_BEGIN

#if ! SODEBUG || STICK_GUEST
const char * const help_general =
"fo\262mor\245information:\n"
"\300hel\260about\n"
"\300hel\260commands\n"
"\300hel\260modes\n"
"\300hel\260statements\n"
"\300hel\260blocks\n"
"\300hel\260devices\n"
"\300hel\260expressions\n"
"\300hel\260strings\n"
"\300hel\260variables\n"
"\300hel\260pins\n"
"\300hel\260zigflea\n"
"\n"
"se\245also:\n"
"\300https://rtestardi.github.io/StickOS/\n"
;

static const char * const help_commands =
"<Ctrl-C>\324-\015sto\260program\n"
"aut\257<line>\321-\015automaticall\271numbe\262progra\255lines\n"
"clea\262[flash]\317-\015clea\262ra\255[an\244flash\235variables\n"
"cls\331-\015clea\262termina\254screen\n"
"con\264[<line>]\317-\015continu\245progra\255fro\255stop\n"
"delet\245([<line>][-][<line>]|<subname>\011-\015delet\245progra\255lines\n"
"dir\331-\015lis\264save\244programs\n"
"edi\264<line>\321-\015edi\264progra\255line\n"
"hel\260[<topic>]\316-\015onlin\245help\n"
"lis\264([<line>][-][<line>]|<subname>\011-\015lis\264progra\255lines\n"
"loa\244<name>\321-\015loa\244save\244program\n"
"memory\326-\015prin\264memor\271usage\n"
"new\331-\015eras\245cod\245ra\255an\244flas\250memories\n"
"profil\245([<line>][-][<line>]|<subname>\011-\015displa\271profil\245info\n"
"purg\245<name>\320-\015purg\245save\244program\n"
"renumbe\262[<line>]\313-\015renumbe\262progra\255line\263(an\244save)\n"
"reset\327-\015rese\264th\245MCU!\n"
"ru\256[<line>]\320-\015ru\256program\n"
"sav\245[<name>|library]\307-\015sav\245cod\245ra\255t\257flas\250memory\n"
"subs\330-\015lis\264su\242names\n"
"undo\330-\015und\257cod\245change\263sinc\245las\264save\n"
#if UPGRADE
"upgrade\325-\015upgrad\245StickO\223firmware!\n"
#endif
"uptime\326-\015prin\264tim\245sinc\245las\264reset\n"
"\n"
"fo\262mor\245information:\n"
"\300hel\260modes\n"
;

static const char * const help_modes =
"analo\247[<millivolts>]\313-\015set/displa\271analo\247voltag\245scale\n"
"bau\244[<rate>]\323-\015set/displa\271uar\264consol\245bau\244rate\n"
"autoru\256[on|off]\320-\015autoru\256mod\245(o\256reset)\n"
"ech\257[on|off]\323-\015termina\254ech\257mode\n"
"inden\264[on|off]\321-\015listin\247inden\264mode\n"
"nodei\244[<nodeid>|none]\312-\015set/displa\271zigfle\241nodeid\n"
"number\263[on|off]\320-\015listin\247lin\245number\263mode\n"
"pin\263[<assign\036[<pinname>|none]]\300-\015set/displa\271StickO\223pi\256assignments\n"
"promp\264[on|off]\321-\015termina\254promp\264mode\n"
"serv\257[<Hz>]\324-\015set/displa\271serv\257H\272(o\256reset)\n"
"ste\260[on|off]\323-\015debugge\262single-ste\260mode\n"
"trac\245[on|off]\322-\015debugge\262trac\245mode\n"
"watchsmar\264[on|off]\315-\015low-overhea\244watchpoin\264mode\n"
"\n"
"pi\256assignments:\n"
"\300heartbeat\300safemode*\n"
"\300qspi_cs*\300zigflea_rst*\300zigflea_attn*\300zigflea_rxtxen\n"
"\n"
"fo\262mor\245information:\n"
"\300hel\260pins\n"
;

static const char * const help_statements =
"<line>\337-\015delet\245progra\255lin\245fro\255cod\245ram\n"
"<line\036<statement>\300/\017comment\307-\015ente\262progra\255lin\245int\257cod\245ram\n"
"\n"
"<variable>[$\235\035<expression\036[\014...]\301-\015assig\256variable\n"
"\037[dec|hex|raw\235<expression\036[\014...\235[;\235-\015prin\264results\n"
"asser\264<expression>\322-\015brea\253i\246expressio\256i\263false\n"
"dat\241<n\036[\014...]\325-\015read-onl\271data\n"
"di\255<variable>[$][[n]\235[a\263...\235[\014...\235-\015dimensio\256variables\n"
"end\342-\015en\244program\n"
"halt\341-\015loo\260forever\n"
"inpu\264[dec|hex|raw\235<variable>[$\235[\014...\235-\015inpu\264data\n"
"labe\254<label>\330-\015read/dat\241label\n"
"le\264<variable>[$\235\035<expression\036[\014...\235-\015assig\256variable\n"
"prin\264[dec|hex|raw\235<expression\036[\014...\235[;\235-\015prin\264results\n"
"rea\244<variable\036[\014...]\316-\015rea\244read-onl\271dat\241int\257variables\n"
"re\255<remark>\331-\015remark\n"
"restor\245[<label>]\324-\015restor\245read-onl\271dat\241pointer\n"
"slee\260<expression\036(s|ms|us)\311-\015dela\271progra\255execution\n"
"stop\341-\015inser\264breakpoin\264i\256code\n"
"vprin\264<variable>[$\235\035[dec|hex|raw\235<expression\036[\014...\235-\015prin\264t\257variable\n"
"\n"
"fo\262mor\245information:\n"
"\300hel\260blocks\n"
"\300hel\260devices\n"
"\300hel\260expressions\n"
"\300hel\260strings\n"
"\300hel\260variables\n"
;

static const char * const help_blocks =
"i\246<expression\036then\n"
"[elsei\246<expression\036then]\n"
"[else]\n"
"endif\n"
"\n"
"fo\262<variable\036\035<expression\036t\257<expression\036[ste\260<expression>]\n"
"\300[(break|continue\011[n]]\n"
"next\n"
"\n"
"whil\245<expression\036do\n"
"\300[(break|continue\011[n]]\n"
"endwhile\n"
"\n"
"do\n"
"\300[(break|continue\011[n]]\n"
"unti\254<expression>\n"
"\n"
"gosu\242<subname\036[<expression>\014...]\n"
"\n"
"su\242<subname\036[<param>\014...]\n"
"\300[return]\n"
"endsub\n"
;

static const char * const help_devices =
"timers:\n"
"\300configur\245time\262<n\036fo\262<n\036(s|ms|us)\n"
"\300o\256time\262<n\036d\257<statement>\316-\015o\256time\262execut\245statement\n"
"\300of\246time\262<n>\334-\015disabl\245time\262interrupt\n"
"\300mas\253time\262<n>\333-\015mask/hol\244time\262interrupt\n"
"\300unmas\253time\262<n>\331-\015unmas\253time\262interrupt\n"
"\n"
"uarts:\n"
"\300configur\245uar\264<n\036fo\262<n\036bau\244<n\036dat\241(even|odd|no\011parit\271[loopback]\n"
"\300o\256uar\264<n\036(input|output\011d\257<statement>\300-\015o\256uar\264execut\245statement\n"
"\300of\246uar\264<n\036(input|output)\316-\015disabl\245uar\264interrupt\n"
"\300mas\253uar\264<n\036(input|output)\315-\015mask/hol\244uar\264interrupt\n"
"\300unmas\253uar\264<n\036(input|output)\313-\015unmas\253uar\264interrupt\n"
"\300uar\264<n\036(read|write\011<variable\036[\014...]\301-\015perfor\255uar\264I/O\n"
"\n"
"i2c:\n"
"\300i2\243(star\264<addr>|(read|write\011<variable\036[\014...]|stop\011-\015maste\262i2\243I/O\n"
"\n"
"qspi:\n"
"\300qsp\251<variable\036[\014...]\322-\015maste\262qsp\251I/O\n"
"\n"
"watchpoints:\n"
"\300o\256<expression\036d\257<statement>\313-\015o\256exp\262execut\245statement\n"
"\300of\246<expression>\331-\015disabl\245exp\262watchpoint\n"
"\300mas\253<expression>\330-\015mask/hol\244exp\262watchpoint\n"
"\300unmas\253<expression>\326-\015unmas\253exp\262watchpoint\n"
;

static const char * const help_expressions =
"th\245followin\247operator\263ar\245supporte\244a\263i\256C,\n"
"i\256orde\262o\246decreasin\247precedence:\n"
"\300<n>\325-\015decima\254constant\n"
"\3000x<n>\323-\015hexadecima\254constant\n"
"\300'c'\325-\015characte\262constant\n"
"\300<variable>\316-\015simpl\245variable\n"
"\300<variable>[<expression>]\300-\015arra\271variabl\245element\n"
"\300<variable>#\315-\015lengt\250o\246arra\271o\262string\n"
"\300(\301)\323-\015grouping\n"
"\300!\301~\323-\015logica\254not\014bitwis\245not\n"
"\300*\301/\301%\317-\015times\014divide\014mod\n"
"\300+\301-\323-\015plus\014minus\n"
"\300>>\300<<\322-\015shif\264right\014left\n"
"\300<=\300<\300>=\300>\314-\015inequalities\n"
"\300==\300!=\322-\015equal\014no\264equal\n"
"\300|\301^\301&\317-\015bitwis\245or\014xor\014and\n"
"\300||\300^^\300&&\316-\015logica\254or\014xor\014and\n"
"fo\262mor\245information:\n"
"\300hel\260variables\n"
;

static const char * const help_strings =
"v\004i\263\241nul-terminate\244vie\267int\257\241byt\245arra\271v[]\n"
"\n"
"strin\247statements:\n"
"\300dim\014input\014let\014print\014vprint\n"
"\300i\246<expression\036<relation\036<expression\036then\n"
"\300whil\245<expression\036<relation\036<expression\036do\n"
"\300unti\254<expression\036<relation\036<expression\036do\n"
"\n"
"strin\247expressions:\n"
"\300\"literal\"\324-\015litera\254string\n"
"\300<variable>$\322-\015variabl\245string\n"
"\300<variable>$[<start>:<length>]\300-\015variabl\245substring\n"
"\300+\334-\015concatenate\263strings\n"
"\n"
"strin\247relations:\n"
"\300<=\300<\300>=\300>\321-\015inequalities\n"
"\300==\300!=\327-\015equal\014no\264equal\n"
"\300~\300!~\330-\015contains\014doe\263no\264contain\n"
"fo\262mor\245information:\n"
"\300hel\260variables\n"
;

static const char * const help_variables =
"al\254variable\263mus\264b\245dimensioned!\n"
"variable\263dimensione\244i\256\241su\242ar\245loca\254t\257tha\264sub\n"
"simpl\245variable\263ar\245passe\244t\257su\242param\263b\271reference\033otherwise\014b\271value\n"
"arra\271variabl\245indice\263star\264a\2640\n"
"\266i\263th\245sam\245a\263v[0]\014excep\264fo\262input/print/i2c/qspi/uar\264statements\n"
"\n"
"ra\255variables:\n"
"\300di\255<var>[$][[n]]\n"
"\300di\255<var>[[n]\235a\263(byte|short)\n"
"\n"
"absolut\245variables:\n"
"\300di\255<var>[[n]\235[a\263(byte|short)\235a\264addres\263<addr>\n"
"\n"
"flas\250paramete\262variables:\n"
"\300di\255<varflash>[[n]\235a\263flash\n"
"\n"
"pi\256alia\263variables:\n"
"\300di\255<varpin\036a\263pi\256<pinname\036fo\262(digital|analog|servo|frequency|uart\011\\\n"
"\344(input|output\011\\\n"
"\344[debounced\235[inverted\235[open_drain]\n"
"\n"
"syste\255variable\263(read-only):\n"
"\300analog\300getchar"
"\300msecs\300nodeid\n"
"\300random\300seconds\300ticks\300ticks_per_msec\n"
"\n"
"fo\262mor\245information:\n"
"\300hel\260pins\n"
;

static const char * const help_pins =
"pi\256names:\n"
#if defined(__32MX250F128B__)
"\3000/8\3031/9\3032/10\3023/11\3024/12\3025/13\3026/14\3027/15\n"
"\300------\015------\015------\015------\015------\015------\015------\015--------+\n"
"\300ra0\303ra1\323ra4\333\274POR\224A\n"
"\377 |\304A+8\n"
"\300rb0\303rb1\303rb2\303rb3\303rb4\303rb5\313rb7\303\274POR\224B\n"
"\300rb8\303rb9\333rb13\302rb14\302rb15\302|\304B+8\n"
"\n"
"al\254pin\263suppor\264genera\254purpos\245digita\254input/output\n"
"ra[0-1],rb[0-3,13-15\235\035potentia\254analo\247inpu\264pin\263(mV)\n"
"rb[2,5,7,9,13\235\035potentia\254analo\247outpu\264(PWM\011pin\263(mV)\n"
"rb[2,5,7,9,13\235\035potentia\254serv\257outpu\264(PWM\011pin\263(us)\n"
"rb[2,5,7,9,13\235\035potentia\254frequenc\271outpu\264pin\263(Hz)\n"
"rb\030(u2\011\035potentia\254uar\264inpu\264pin\263(receive\244byte)\n"
"rb1\024(u2\011\035potentia\254uar\264outpu\264pin\263(transmi\264byte)\n"
"\n"
"i2c: rb8=SCL1, rb9=SDA1\n"  // XXX -- why don't other MCUs show these?
"qspi: ra4=SDI2, rb13=SDO2, rb14=SS2, rb15=SCK2\n"  // XXX -- why don't other MCUs show these?
#elif defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
"\303ICSP\306GN\2043V\0235V\300a0\300a1\300a2\300a3\300a4\300a5\300a6\300a7\300a8\n"
"\300U\370e2\n"
"\300S\361GN\204SCOPE\n"
"\300B\374B\n"
"\377N\n"
"\377C\n"
"\364GND\300WAVE\n"
"\373e3\n"
"\303s1\300e1\304GN\2043V\0235V\300b0\300b1\300b2\300b3\300b4\300b5\300b6\300b7\300b8\n"
"\n"
"[ab][0-8],e[1-3],s\021suppor\264genera\254purpos\245digita\254input/output\n"
"(digita\254input\263b[0-8\235ar\2455\226tolerant)\n"
"a[1-8\235\035potentia\254analo\247inpu\264pin\263(mV)\n"
"a[3-8\235\035potentia\254analo\247outpu\264(PWM\011pin\263(mV)\n"
"a[3-8\235\035potentia\254serv\257outpu\264(PWM\011pin\263(us)\n"
"a[3-8\235\035potentia\254frequenc\271outpu\264pin\263(Hz)\n"
"b\026(u1)\014b\024(u2\011\035potentia\254uar\264inpu\264pin\263(receive\244byte)\n"
"b\030(u1)\014b\027(u2\011\035potentia\254uar\264outpu\264pin\263(transmi\264byte)\n"
"i2c: a6=SDA, a7=SCL\n"  // XXX -- why don't other MCUs show these?
"qspi: b0=SDO, b1=SDI, b2=SCK, b3=SS\n"  // XXX -- why don't other MCUs show these?
#else
"\3000/8\3031/9\3032/10\3023/11\3024/12\3025/13\3026/14\3027/15\n"
"\300------\015------\015------\015------\015------\015------\015------\015--------+\n"
"\300an0\303an1\303an2\303an3\303an4\303an5\303an6\303an7\303\274POR\224B\n"
"\300an8\303an9\303an10\302an11\302an12\302an13\302an14\302an15\302|\304B+8\n"
"\310rc1\303rc2\303rc3\303rc4\333\274POR\224C\n"
"\340rc12\302rc13\302rc14\302rc15\302|\304C+8\n"
"\300rd0\303rd1\303rd2\303rd3\303rd4\303rd5\303rd6\303rd7\303\274POR\224D\n"
"\300rd8\303rd9\303rd10\302rd11\302rd12\302rd13\302rd14\302rd15\302|\304D+8\n"
"\300re0\303re1\303re2\303re3\303re4\303re5\303re6\303re7\303\274POR\224E\n"
"\300re8\303re9\363|\304E+8\n"
"\300rf0\303rf1\303rf2\303rf3\303rf4\303rf5\323\274POR\224F\n"
"\300rf8\333rf12\302rf13\322|\304F+8\n"
"\300rg0\303rg1\303rg2\303rg3\323rg6\303rg7\303\274POR\224G\n"
"\300rg8\303rg9\323rg12\302rg13\302rg14\302rg15\302|\304G+8\n"
"\n"
"al\254pin\263suppor\264genera\254purpos\245digita\254input/output\n"
"an\037\035potentia\254analo\247inpu\264pin\263(mV)\n"
"rd[0-4\235\035potentia\254analo\247outpu\264(PWM\011pin\263(mV)\n"
"rd[0-4\235\035potentia\254serv\257outpu\264(PWM\011pin\263(us)\n"
"rd[0-4\235\035potentia\254frequenc\271outpu\264pin\263(Hz)\n"
"rf\024(u2\011\035potentia\254uar\264inpu\264pin\263(receive\244byte)\n"
"rf\025(u2\011\035potentia\254uar\264outpu\264pin\263(transmi\264byte)\n"
#endif
;

static const char * const help_zigflea =
"connec\264<nodeid>\314-\015connec\264t\257MC\225<nodeid\036vi\241zigflea\n"
"<Ctrl-D>\324-\015disconnec\264fro\255zigflea\n"
"\n"
"remot\245nod\245variables:\n"
"\300di\255<varremote>[[n]\235a\263remot\245o\256nodei\244<nodeid>\n"
"\n"
"zigfle\241cable:\n"
"\300MCU\320MC1320X\n"
"\300-------------\306-----------\n"
// REVISIT -- implement zigflea on MRF24J40
"\300sck1\317spiclk\n"
"\300sdi1\317miso\n"
"\300sdo1\317mosi\n"
"\300int1\317irq*\n"
"\300pin\263qspi_cs*\306ce*\n"
"\300pin\263zigflea_rst*\302rst*\n"
"\300pin\263zigflea_rxtxen\300rxtxen\n"
"\300vss\320vss\n"
"\300vdd\320vdd\n"
;
#endif

