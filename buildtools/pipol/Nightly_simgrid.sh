#!/bin/bash

#PRE-PIPOL /home/mescal/navarro/pre-simgrid.sh

#PIPOL esn i386-linux-ubuntu-intrepid.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-intrepid.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-jaunty.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-fedora-core11.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core11.dd.gz none 02:00 --user --silent

#PIPOL esn i386_kvm-linux-debian-lenny none 02:00 --user --silent
#PIPOL esn i386_kvm-linux-debian-testing none 02:00 --user --silent

#PIPOL esn amd64_kvm-linux-debian-lenny none 02:00 --user --silent
#PIPOL esn amd64_kvm-linux-debian-testing none 02:00 --user --silent

SYSTEM=`uname`

if [ -e ./pipol ] ; then
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
else
	mkdir ./pipol
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
fi
cd ./pipol/$PIPOL_HOST

svn checkout svn://scm.gforge.inria.fr/svn/simgrid/simgrid/trunk simgrid-trunk --quiet

sh ./simgrid-trunk/buildtools/pipol/liste_install.sh
perl ./simgrid-trunk/buildtools/pipol/cmake.pl

cd simgrid-trunk

rm CMakeCache.txt

#ucontext
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_smpi=on .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
make clean

#pthread
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_java=on \
-Dwith_context=pthread \
-Denable_smpi=on .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
make clean

#gtnets
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Denable_java=on \
-Dwith_context=auto \
-Denable_smpi=on .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
make clean

#full_flags
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=on \
-Denable_compile_warnings=on \
-Denable_smpi=on .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
make clean

#supernovae
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=on \
-Denable_smpi=on .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
make clean

#model checking
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_graphviz=on \
-Denable_model-checking=on \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Denable_java=on \
-Dwith_context=auto \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_smpi=on .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

if [ $SYSTEM = Linux ] ; then
	cd ..
	home_dir=`pwd`

	svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/GTNetS/ --quiet
	cd GTNetS
	unzip gtnets-current.zip > /dev/null
	tar zxvf gtnets-current-patch.tgz  > /dev/null
	cd gtnets-current
	cat ../00*.patch | patch -p1 > /dev/null

	ARCH_32=`uname -m | cut -d'_' -f2`

	if [ x$ARCH_32 = x64 ] ; then #only if 64 bit processor family
	cat ../AMD64-FATAL-Removed-DUL_SIZE_DIFF-Added-fPIC-compillin.patch | patch -p1 > /dev/null
	fi

	ln -sf Makefile.linux Makefile
	make -j 3 depend > /dev/null
	make -j 3 debug > /dev/null 2>&1
	make -j 3 opt > /dev/null 2>&1
	wait
	cd $home_dir
	absolute_path=`pwd`
	userhome=$absolute_path

	if [ -e $userhome/usr/lib ] ; then
		echo ""
	else
		mkdir $userhome/usr	
		mkdir $userhome/usr/lib
	fi

	if [ -e $userhome/usr/include ] ; then
		echo ""
	else	
		mkdir $userhome/usr/include 
	fi

	cp -fr $absolute_path/GTNetS/gtnets-current/*.so $userhome/usr/lib/
	ln -sf $userhome/usr/lib/libgtsim-opt.so $userhome/usr/lib/libgtnets.so

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$userhome/usr/lib/
	mkdir $userhome/usr/include/gtnets
	cp -fr $absolute_path/GTNetS/gtnets-current/SRC/*.h $userhome/usr/include/gtnets
	wait
	cd $home_dir
	rm -rf $absolute_path/GTNetS
	cd simgrid-trunk
	
	if [ -e $userhome/usr/lib/libgtsim-opt.so ] ; then
		#gtnets
		cmake -Denable_lua=on \
		-Denable_ruby=on \
		-Denable_lib_static=on \
		-Denable_graphviz=on \
		-Denable_model-checking=off \
		-Denable_tracing=on \
		-Denable_latency_bound_tracking=on \
		-Denable_gtnets=on \
		-Denable_java=on \
		-Dwith_context=auto \
		-Denable_smpi=on .
		ctest -D NightlyStart
		ctest -D NightlyConfigure
		ctest -D NightlyBuild
		ctest -D NightlyTest
		ctest -D NightlySubmit
		make clean
	fi
fi

if( $PIPOL_IMAGE = i386-linux-ubuntu-karmic.dd.gz )
	#Make the memcheck mode
	cmake -Denable_gtnets=off \
	-Denable_memcheck=on ./
	ctest -D NightlyStart
	ctest -D NightlyConfigure
	ctest -D NightlyBuild
	ctest -D NightlyMemCheck
	ctest -D NightlySubmit
	make clean
fi