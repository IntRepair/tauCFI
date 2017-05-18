# start me from within an app directory, e.g. nginx-0.8.54/

set -e

if [ $# -lt 1 ]
then	echo "Usage: $0 <executable> [args]"
	exit 1
fi

exe=$1

PROJECT_ROOT=/home/matthias/typearmor
DYNAMORIO_LIBS=$PROJECT_ROOT/DynamoRIO/lib64/release
BINARY_DIR=$PROJECT_ROOT/bin

shift

set -x
sudo DYNINSTAPI_RT_LIB=/usr/local/lib/libdyninstAPI_RT.so LD_LIBRARY_PATH=$BINARY_DIR:$DYNAMORIO_LIBS:$LD_LIBRARY_PATH $BINARY_DIR/analyzer $exe --calltargets --callsites $*

