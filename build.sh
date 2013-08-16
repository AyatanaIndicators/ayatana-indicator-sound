#! /bin/bash
set -e

ECLIPSE=false
CLEAN=false
SOURCEDIR="$PWD"
BRANCHNAME=`basename $SOURCEDIR`
BUILDDIR="$SOURCEDIR/../$BRANCHNAME-build"

usage() {
    echo "usage: $0 [OPTIONS] [BUILD_TYPE]\n" >&2
    echo "Script to build indicator-sound. If BUILD_TYPE is not specified, it defaults to \"Debug\".\n" >&2
    echo "OPTIONS:" >&2
    echo " -e, --eclipse Generate Eclipse projects" >&2
    echo " -c, --clean Clean the build tree before building" >&2
    echo >&2
    exit 1
}

ARGS=`getopt -n$0 -u -a --longoptions="eclipse,clean,help" -o "sch" -- "$@"`
[ $? -ne 0 ] && usage
eval set -- "$ARGS"

while [ $# -gt 0 ]
do
    case "$1" in
       -e|--eclipse) ECLIPSE=true;;
       -c|--clean)   CLEAN=true;;
       -h|--help)    usage;;
       --)           shift;break;;
    esac
    shift
done

[ $# -gt 1 ] && usage

BUILD_TYPE="Debug"
[ $# -eq 1 ] && BUILD_TYPE="$1"

if [ -f "/usr/bin/ninja" ] ; then
  if $ECLIPSE; then
    GENERATOR="Eclipse CDT4 - Ninja"
  else
    GENERATOR="Ninja"
  fi
  BUILD_COMMAND="ninja"
else
  if $ECLIPSE; then
    GENERATOR="Eclipse CDT4 - Unix Makefiles"
  else
    GENERATOR="Unix Makefiles"
  fi
  BUILD_COMMAND="make"
fi

echo "Using $BUILD_COMMAND to build"

if $CLEAN; then
  rm -rf $BUILDDIR
fi

mkdir -p $BUILDDIR
(
  cd $BUILDDIR
  cmake "$SOURCEDIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX="$SOURCEDIR/../$BRANCHNAME-install"
  $BUILD_COMMAND
)
