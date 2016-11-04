# start me from within an app directory, e.g. nginx-0.8.54/

set -e

if [ $# -lt 1 ]
then	echo "Usage: $0 <executable> [args]"
	exit 1
fi

exe=$1

PA_ROOT=/home/typearmor/patharmor

DI=$PA_ROOT/Dyninst/
DRLIB=$PA_ROOT/DynamoRIO/lib64/release

DI_OPT=$PA_ROOT/bin/di-opt
PADYN_DI=$PA_ROOT/bin/padyn.di

if [ ! -x $DI_OPT ]
then	echo "$DI_OPT not found. Please build and install it first."
	echo "And invoke this script from an app directory."
	exit 1
fi

shift

set -x


DYNINST_DEBUG_SPRINGBOARD=0 DYNINST_DEBUG_RELOC=0 DYNINST_DEBUG_RELOCATION=0 LD_BIND_NOW=y DYNINSTAPI_RT_LIB=$DI/dyninstAPI_RT/libdyninstAPI_RT.so LD_LIBRARY_PATH=$DRLIB:$DI/dynC_API/:$DI/common/:$DI/parseAPI/:$DI/patchAPI/:$DI/symtabAPI:$DI/instructionAPI/:$DI/dyninstAPI/:$LD_LIBRARY_PATH $DI_OPT -load=$PADYN_DI -padyn -o $exe.patched $exe

mv $exe $exe.backup
mv $exe.patched $exe
#chown typearmor:typearmor $exe
