foreach(x check_defaults stack_overflow)
  add_executable       (${x}  ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

foreach (factory raw thread boost ucontext)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/check_defaults/factory_${factory}.tesh)

  string (TOUPPER have_${factory}_contexts VARNAME)
  if (${VARNAME} AND release AND (WIN32 OR CMAKE_SYSTEM_NAME MATCHES "Darwin"))
     SET_TESTS_PROPERTIES(stack-overflow-${factory} PROPERTIES WILL_FAIL true)
  endif()
  if (${VARNAME})
    ADD_TESH(tesh-simix-factory-${factory} --cfg contexts/factory:${factory} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_${factory}.tesh)
  endif()
endforeach()

IF(HAVE_RAW_CONTEXTS)
  ADD_TESH(tesh-simix-factory-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_raw.tesh)
ELSEIF(HAVE_UCONTEXT_CONTEXTS)
  ADD_TESH(tesh-simix-factory-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_ucontext.tesh)
ELSEIF(HAVE_BOOST_CONTEXTS)
  ADD_TESH(tesh-simix-factory-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_boost.tesh)
ELSE()
  ADD_TESH(tesh-simix-factory-default --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_thread.tesh)
ENDIF()

ADD_TESH_FACTORIES(stack-overflow "thread;ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/stack_overflow --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/stack_overflow stack_overflow.tesh)

set(teshsuite_src  ${teshsuite_src}                                                                        PARENT_SCOPE)
set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/stack_overflow/stack_overflow.tesh        PARENT_SCOPE)