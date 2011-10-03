#########################################
### Fill in the "make install" target ###
#########################################
	  
# doc
if(NOT EXISTS ${CMAKE_HOME_DIRECTORY}/doc/html/)
	file(MAKE_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/html/)
endif(NOT EXISTS ${CMAKE_HOME_DIRECTORY}/doc/html/)
install(DIRECTORY "${CMAKE_HOME_DIRECTORY}/doc/html/"
  DESTINATION "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/doc/simgrid/html/"
  PATTERN ".svn" EXCLUDE 
  PATTERN ".git" EXCLUDE 
  PATTERN "*.o" EXCLUDE
  PATTERN "*~" EXCLUDE
)

#### Generate the manpages
if( NOT MANPAGE_DIR )
	set( MANPAGE_DIR $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man/share/man/man1 )
endif( NOT MANPAGE_DIR)

add_custom_target(TARGET install
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MANPAGE_DIR}
	COMMAND pod2man tools/simgrid_update_xml.pl > ${MANPAGE_DIR}/simgrid_update_xml.1
	COMMENT "Generating manpages"
)

# binaries
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/smpicc
                 ${CMAKE_BINARY_DIR}/bin/smpif2c
                 ${CMAKE_BINARY_DIR}/bin/smpiff
                 ${CMAKE_BINARY_DIR}/bin/smpirun
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/)

install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/tesh
DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/)

install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/graphicator
DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/)
	
install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/tools/MSG_visualization/colorize.pl
        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/
		RENAME simgrid-colorizer)
		
add_custom_target(simgrid-colorizer ALL
COMMENT "Install ${CMAKE_BINARY_DIR}/bin/colorize"
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/tools/MSG_visualization/colorize.pl ${CMAKE_BINARY_DIR}/bin/colorize
)
				
install(PROGRAMS ${CMAKE_HOME_DIRECTORY}/tools/simgrid_update_xml.pl
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/
		RENAME simgrid_update_xml)
		
add_custom_target(simgrid_update_xml ALL
COMMENT "Install ${CMAKE_BINARY_DIR}/bin/simgrid_update_xml"
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/tools/simgrid_update_xml.pl ${CMAKE_BINARY_DIR}/bin/simgrid_update_xml
)
		
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/gras_stub_generator
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/)

# libraries
install(TARGETS simgrid gras 
        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
	
if(enable_smpi)	
  install(TARGETS smpi
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
endif(enable_smpi)

if(enable_lib_static AND NOT WIN32)
	install(TARGETS simgrid_static 
	        DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
	if(enable_smpi)	
  		install(TARGETS smpi_static
          		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
	endif(enable_smpi)
endif(enable_lib_static AND NOT WIN32)

# include files
set(HEADERS
    ${headers_to_install}
    ${generated_headers_to_install}
    )
foreach(file ${HEADERS})
  get_filename_component(location ${file} PATH)
  string(REPLACE "${CMAKE_CURRENT_BINARY_DIR}/" "" location "${location}")
  install(FILES ${file}
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${location})
endforeach(file ${HEADERS})

# example files
foreach(file ${examples_to_install_in_doc})
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/examples/" "" file ${file})
  get_filename_component(location ${file} PATH)
  install(FILES "examples/${file}"
          DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/doc/simgrid/examples/${location})
endforeach(file ${examples_to_install_in_doc})

# bindings cruft

if(HAVE_LUA)
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib/lua/5.1")
	add_custom_target(simgrid_lua ALL
  		DEPENDS simgrid 
  				${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
		)
	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
		COMMAND ${CMAKE_COMMAND} -E create_symlink ../../libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
	)
	install(FILES ${CMAKE_BINARY_DIR}/lib/lua/5.1/simgrid.${LIB_EXE}
		DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/lua/5.1
		)
endif(HAVE_LUA)

###########################################
### Fill in the "make uninstall" target ###
###########################################

add_custom_target(uninstall
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/doc/simgrid
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall doc ok"
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/lib/libgras*
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/lib/libsimgrid*
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/lib/libsmpi*
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall lib ok"
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpicc
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpif2c
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpiff
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/smpirun
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/tesh
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/simgrid-colorizer
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/simgrid_update_xml
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/bin/gras_stub_generator
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall bin ok"
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/amok
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/gras
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/instr
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/msg 
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/simdag
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/smpi
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/surf
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/xbt
COMMAND ${CMAKE_COMMAND} -E	remove_directory ${CMAKE_INSTALL_PREFIX}/include/mc
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/include/simgrid_config.h
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/include/gras.h 
COMMAND ${CMAKE_COMMAND} -E	remove -f ${CMAKE_INSTALL_PREFIX}/include/xbt.h
COMMAND ${CMAKE_COMMAND} -E	echo "uninstall include ok"
WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)

if(HAVE_LUA)
	add_custom_command(TARGET uninstall
	COMMAND ${CMAKE_COMMAND} -E echo "uninstall binding lua ok"
	COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_INSTALL_PREFIX}/lib/lua/5.1/simgrid.${LIB_EXE}
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/"
	)
endif(HAVE_LUA)

################################################################
## Build a sain "make dist" target to build a source package ###
##   containing only the files that I explicitely state      ###
##   (instead of any cruft laying on my disk as CPack does)  ###
################################################################

##########################################
### Fill in the "make dist-dir" target ###
##########################################

add_custom_target(dist-dir
  COMMENT "Generating the distribution directory"
  COMMAND test -e simgrid-${release_version}/ && chmod -R a+w simgrid-${release_version}/ || true
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/doc/html/
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_HOME_DIRECTORY}/doc/html/ simgrid-${release_version}/doc/html/
)
add_dependencies(dist-dir simgrid_documentation)
add_dependencies(dist-dir maintainer_files)

set(dirs_in_tarball "")
foreach(file ${source_to_pack})
  #message(${file})
  # This damn prefix is still set somewhere (seems to be in subdirs)
  string(REPLACE "${CMAKE_HOME_DIRECTORY}/" "" file "${file}")
  
  # Create the directory on need
  get_filename_component(file_location ${file} PATH)
  string(REGEX MATCH ";${file_location};" OPERATION "${dirs_in_tarball}")
  if(NOT OPERATION)
       set(dirs_in_tarball "${dirs_in_tarball};${file_location};")
       add_custom_command(
         TARGET dist-dir
         COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/${file_location}/
       )       
   endif(NOT OPERATION)
   
   # Actually copy the file
   add_custom_command(
     TARGET dist-dir
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/${file} simgrid-${release_version}/${file_location}/
   )
   
   add_custom_command(
     TARGET dist-dir
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Makefile.default simgrid-${release_version}/Makefile
   )
endforeach(file ${source_to_pack})

######################################
### Fill in the "make dist" target ###
######################################

add_custom_target(dist
  DEPENDS ${CMAKE_BINARY_DIR}/simgrid-${release_version}.tar.gz
)
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/simgrid-${release_version}.tar.gz	
	COMMENT "Compressing the archive from the distribution directory"
	COMMAND ${CMAKE_COMMAND} -E tar cf simgrid-${release_version}.tar simgrid-${release_version}/
  	COMMAND gzip -9v simgrid-${release_version}.tar
  	COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/
)
add_dependencies(dist dist-dir)

###########################################
### Fill in the "make distcheck" target ###
###########################################

# Allow to test the "make dist"
add_custom_target(distcheck
  COMMAND ${CMAKE_COMMAND} -E echo "XXX remove old copy"
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}.cpy 
  COMMAND ${CMAKE_COMMAND} -E echo "XXX copy again the source tree"
  COMMAND ${CMAKE_COMMAND} -E copy_directory simgrid-${release_version}/ simgrid-${release_version}.cpy 
  COMMAND ${CMAKE_COMMAND} -E echo "XXX create build and install subtrees"
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/_build
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/_inst
 
  # This stupid cmake creates a directory in source, killing the purpose of the chmod
  # (tricking around)
  COMMAND ${CMAKE_COMMAND} -E echo "XXX change the modes of directories"
  COMMAND ${CMAKE_COMMAND} -E make_directory simgrid-${release_version}/CMakeFiles 
  COMMAND chmod -R a-w simgrid-${release_version}/ # FIXME: we should pass without commenting that line
  COMMAND chmod -R a+w simgrid-${release_version}/_build
  COMMAND chmod -R a+w simgrid-${release_version}/_inst
  COMMAND chmod -R a+w simgrid-${release_version}/CMakeFiles
  
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Configure"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ${CMAKE_COMMAND} build ..  -DCMAKE_INSTALL_PREFIX=../_inst -Wno-dev -Denable_doc=OFF
#  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make dist-dir
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Build"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make VERBOSE=1
  
  # This fails, unfortunately, because GRAS is broken for now
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ctest -j5 --output-on-failure

  COMMAND ${CMAKE_COMMAND} -E echo "XXX Check that cleaning works"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build make clean
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Display what is remaining after make clean"
  COMMAND ${CMAKE_COMMAND} -E chdir simgrid-${release_version}/_build ls -lR
  COMMAND ${CMAKE_COMMAND} -E echo "XXX Remove _build and _inst directories"
  COMMAND chmod a+w simgrid-${release_version}/
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/_build
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}/_inst
  COMMAND ${CMAKE_COMMAND} -E echo "XXX The output of the diff follows"
  COMMAND diff -ruN simgrid-${release_version}.cpy simgrid-${release_version}
  COMMAND ${CMAKE_COMMAND} -E echo "XXX end of the diff, random cleanups now"
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}.cpy 
  COMMAND ${CMAKE_COMMAND} -E remove_directory simgrid-${release_version}
)
add_dependencies(distcheck dist-dir)

#######################################
### Fill in the "make check" target ###
#######################################

if(enable_memcheck)
	add_custom_target(check
	COMMAND ctest -D ExperimentalMemCheck
	)
else(enable_memcheck)
	add_custom_target(check
	COMMAND make test
	)
endif(enable_memcheck)

#######################################
### Fill in the "make xxx-clean" target ###
#######################################

add_custom_target(maintainer-clean
COMMAND ${CMAKE_COMMAND} -E remove -f src/config_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/cunit_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/dict_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/dynar_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/ex_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/set_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/simgrid_units_main.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/swag_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_sha_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_str_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_strbuff_unit.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/xbt_synchro_unit.c
WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
)

add_custom_target(supernovae-clean
COMMAND ${CMAKE_COMMAND} -E remove -f src/supernovae_gras.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/supernovae_sg.c
COMMAND ${CMAKE_COMMAND} -E remove -f src/supernovae_smpi.c
WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
)

#############################################
### Fill in the "make sync-gforge" target ###
#############################################

add_custom_target(sync-gforge-doc
COMMAND chmod g+rw -R doc/
COMMAND chmod a+rX -R doc/
COMMAND rsync --verbose --cvs-exclude --compress --delete --delete-excluded --rsh=ssh --ignore-times --recursive --links --perms --times --omit-dir-times 
doc/html/ scm.gforge.inria.fr:/home/groups/simgrid/htdocs/${release_version}/doc/ || true
COMMAND scp doc/html/simgrid_modules2.png doc/html/simgrid_modules.png doc/webcruft/simgrid_logo.png  doc/webcruft/simgrid_logo_small.png scm.gforge.inria.fr:/home/groups/simgrid/htdocs/${release_version}/
WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
)
add_dependencies(sync-gforge-doc simgrid_documentation)

add_custom_target(sync-gforge-dtd
COMMAND scp src/surf/simgrid.dtd scm.gforge.inria.fr:/home/groups/simgrid/htdocs/${release_version}/
WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
)

#PIPOL
add_custom_target(sync-pipol
COMMAND scp -r Experimental_bindings.sh Experimental.sh  MemCheck.sh pre-simgrid.sh navarro@pipol.inria.fr:~/
COMMAND scp -r rc.* navarro@pipol.inria.fr:~/.pipol/
COMMAND scp -r Nightly* navarro@pipol.inria.fr:~/.pipol/nightly
COMMAND ssh navarro@pipol.inria.fr "chmod a=rwx ~/* ~/.pipol/rc.* ~/.pipol/nightly/*"
WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/buildtools/pipol/"
)

include(CPack)
