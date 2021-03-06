foreach(x actor comm-pt2pt comm-waitany concurrent_rw host_on_off_wait listen_async pid storage_client_server)
  add_executable       (${x}  ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

## Add the tests.
## Some need to be run with all factories, some need not tesh to run
foreach(x actor concurrent_rw)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  ADD_TESH_FACTORIES(tesh-s4u-${x} "thread;boost;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/s4u/${x} --cd ${CMAKE_BINARY_DIR}/teshsuite/s4u/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/s4u/${x}/${x}.tesh)
endforeach()

foreach(x host_on_off_wait listen_async pid storage_client_server)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  ADD_TESH(tesh-s4u-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/s4u/${x} --cd ${CMAKE_BINARY_DIR}/teshsuite/s4u/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/s4u/${x}/${x}.tesh)
endforeach()

# The output is not relevant
ADD_TEST(tesh-s4u-comm-pt2pt   ${CMAKE_BINARY_DIR}/teshsuite/s4u/comm-pt2pt/comm-pt2pt     ${CMAKE_HOME_DIRECTORY}/examples/platforms/cluster.xml)
ADD_TEST(tesh-s4u-comm-waitany ${CMAKE_BINARY_DIR}/teshsuite/s4u/comm-waitany/comm-waitany ${CMAKE_HOME_DIRECTORY}/examples/platforms/two_hosts.xml)



set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)
set(xml_files     ${xml_files}      PARENT_SCOPE)
