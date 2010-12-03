find_program(VALGRIND_EXE 
	NAME valgrind
    PATH_SUFFIXES bin/
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

if(VALGRIND_EXE)
message(STATUS "Found valgrind: ${VALGRIND_EXE}")
SET(VALGRIND_COMMAND "${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/my_valgrind.pl")
SET(MEMORYCHECK_COMMAND "${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/my_valgrind.pl")
endif(VALGRIND_EXE)

if(VALGRIND_EXE)
	exec_program("${VALGRIND_EXE} --version " OUTPUT_VARIABLE "VALGRIND_VERSION")
	string(REGEX MATCH "[0-9].[0-9].[0-9]" NEW_VALGRIND_VERSION "${VALGRIND_VERSION}")
	if(NEW_VALGRIND_VERSION)
		message(STATUS "Valgrind version: ${NEW_VALGRIND_VERSION}")
		exec_program("${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/generate_memcheck_tests.pl ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/AddTests.cmake > ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/memcheck_tests.cmake" OUTPUT_VARIABLE SHUTT)
		set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --show-reachable=yes --track-origins=yes --read-var-info=no")
		message(STATUS "Valgrind options: ${MEMORYCHECK_COMMAND_OPTIONS}")
	else(NEW_VALGRIND_VERSION)
		set(enable_memcheck false)
		message(STATUS "Error: Command valgrind not found --> enable_memcheck autoset to false.")
	endif(NEW_VALGRIND_VERSION)
else(VALGRIND_EXE)
	set(enable_memcheck false)
	message(FATAL_ERROR "Command valgrind not found --> enable_memcheck autoset to false.")
endif(VALGRIND_EXE)