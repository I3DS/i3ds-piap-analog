find_package (Boost COMPONENTS program_options log REQUIRED)

set (SRCS
  xilinx_analog.cpp
  )

set (LIBS
  i3ds
  zmq
  pthread
  ${Boost_LIBRARIES}
  )

include_directories ("../include/")

add_executable (i3ds_piap_analog i3ds_piap_analog.cpp ${SRCS})
target_compile_options(i3ds_piap_analog PRIVATE ${BASLER_HR_CPPFLAGS} -Wno-unknown-pragmas)
target_link_libraries (i3ds_piap_analog ${BASLER_HR_LIB_FLAGS} ${BASLER_HR_LDFLAGS} ${LIBS})
install(TARGETS i3ds_piap_analog DESTINATION bin)
