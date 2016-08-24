# start me from within an app directory, e.g. nginx-0.8.54/

set -e

if [ $# -lt 1 ]
then	echo "Usage: $0 <executable> [args]"
	exit 1
fi

exe=$1

DI=`pwd`/../Dyninst/
DRLIB=`pwd`/../DynamoRIO/lib64/release

DI_OPT=../bin/di-opt
if [ ! -x $DI_OPT ]
then	echo "$DI_OPT not found. Please build and install it first."
	echo "And invoke this script from an app directory."
	exit 1
fi

shift

set -x
sudo LD_BIND_NOW=y DYNINSTAPI_RT_LIB=$DI/dyninstAPI_RT/libdyninstAPI_RT.so LD_LIBRARY_PATH=$DRLIB:$DI/dyninstAPI/:$LD_LIBRARY_PATH $DI_OPT -load=`pwd`/../bin/padyn.di -padyn -args `pwd`/$exe $*
