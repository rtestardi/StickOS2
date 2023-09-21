#! /bin/sh

# build script
# N.B we assume file names are not replicated throughout tree, except for build flavor overloading

set -a

ROOTDIR=`pwd -L`

case `uname 2>/dev/null` in
  Windows*)
    PATHSEP=";"
    ;;
  Linux|CYGWIN*)
    PATHSEP=":"
    ;;
esac

GOAL=

if [ X${1:-} = X-ac ]; then
  GOAL=allclean
  shift 1
fi

if [ X${1:-} = X-c ]; then
  GOAL=clean
  shift 1
fi

if [ X${1:-} = X-dc ]; then
  GOAL=depclean
  shift 1
fi

if [ X${1:-} = X-l ]; then
  GOAL=list
  shift 1
fi

unset TARGET TARGETTYPE ASMS LCFS SOURCES LIBS ALLCLEANS INCLUDES DEFINES

case `uname 2>/dev/null` in
  Windows*|CYGWIN*)
    build="windows"
    ;;
  Linux)
    build="linux"
    ;;
esac

# our build flavors are below
grep -v "^#" <<EOF | while read TARGET BUILD CPU
  stickos windows.STICK_GUEST 512HBL.PIC32.__32MX440F512H__.HIDBL
  stickos windows.STICK_GUEST 512H.PIC32.__32MX440F512H__
  stickos windows.STICK_GUEST 512LBL.PIC32.__32MX460F512L__.HIDBL
  stickos windows.STICK_GUEST 512L.PIC32.__32MX460F512L__
  stickos windows.STICK_GUEST 795HBL.PIC32.__32MX795F512H__.HIDBL
  stickos windows.STICK_GUEST 795H.PIC32.__32MX795F512H__
  stickos windows.STICK_GUEST 795LBL.PIC32.__32MX795F512L__.HIDBL
  stickos windows.STICK_GUEST 795L.PIC32.__32MX795F512L__
  stickos windows.STICK_GUEST UNO32.CHIPKIT.PIC32.__32MX320F128H__
  stickos windows.STICK_GUEST MAX32.CHIPKIT.PIC32.__32MX795F512L__
EOF
do
  for MODE in DEBUG RELEASE; do
    # exclude flavors not requested by the user
    for i in $*; do
      if echo "$TARGET $BUILD $CPU $MODE" | grep -i $i >/dev/null; then
        :
      else
        continue 2
      fi
    done
    
    # exclude impossible flavor combinations
    if [ ${BUILD%%.*} = linux -a $build = windows ]; then
      continue
    fi
    if [ ${BUILD%%.*} = windows -a $build = linux ]; then
      continue
    fi
    if [ ${BUILD%%.*} = cw6 -a $build = linux ]; then
      continue
    fi
    if [ ${BUILD%%.*} = cw7 -a $build = linux ]; then
      continue
    fi
    
    PATHS=`echo $TARGET $BUILD $CPU $MODE | sed 's![.]! !g'`
        
    echo "*** $PATHS ***"

    # compute all possible source/include directories
    DIRS=`find . -name \*.[Cchs] -o -name \*.lcf | sed 's!/[^/]*$!!' | sort -u | sed 's!$!/;!'`

    # select preferred sources/include directories
    SOURCEDIRS=`for i in $PATHS; do echo "$DIRS" | grep -i "/$i/"; done`

    # append remaining source/include directories
    PATHS=`echo $PATHS | sed 's! !/|/!g'`
    SOURCEDIRS=`echo "$SOURCEDIRS"; echo "$DIRS" | grep -viE "/$PATHS/"`
        
    # make source/include directories absolute
    SOURCEDIRS=`echo $SOURCEDIRS | sed "s![.]/!$ROOTDIR/!g"`

    # specify object directory name
    OBJDIR=obj.${BUILD%%.*}.${CPU%%.*}.${MODE%%.*}

    # finally, make the target flavor
    # N.B. we pass ROOTDIR, PATHSEP, TARGET, BUILD, CPU, MODE, SOURCEDIRS, OBJDIR thru environment
    if make --no-print-directory -C $TARGET -f Makefile.$TARGET $GOAL; then
      :
    else
      exit 1
    fi
    
    echo
  done
done

echo "build done!"

