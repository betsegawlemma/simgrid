###################################################################
### Load all files declaring binaries (tools, examples and tests) #
###################################################################
add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools/gras)

if(WIN32)
	add_custom_target(tesh ALL
	DEPENDS ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/tesh.pl
	COMMENT "Install ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/tesh.pl"
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/tesh.pl ${CMAKE_BINARY_DIR}/bin/tesh
	)
else(WIN32)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools/tesh)
endif(WIN32)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools/graphicator/)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/testsuite/xbt)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/testsuite/surf)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/xbt)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/datadesc)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/msg_handle)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/empty_main)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/small_sleep)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network/p2p)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network/mxn)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/partask)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/msg)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/ping)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/rpc)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/spawn)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/timer)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/chrono)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/mutual_exclusion/simple_token)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/mmrpc)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/all2all)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/pmm)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/synchro)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/properties)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/console)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/properties)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/actions)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/migration)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/sendrecv)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/suspend)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/parallel_task)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/priority)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/trace)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/tracing)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/icomms)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/chord)

if(HAVE_MC)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/mc)
endif(HAVE_MC)

if(HAVE_GTNETS)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets)
endif(HAVE_GTNETS)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/amok/bandwidth)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/amok/saturate)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/dax)
if(HAVE_GRAPHVIZ)
  add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/dot)
endif(HAVE_GRAPHVIZ)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/metaxml)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/properties)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/scheduling)

if(enable_smpi)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/smpi)
endif(enable_smpi)
