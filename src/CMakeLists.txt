find_package (Boost COMPONENTS program_options log REQUIRED)

set (SRCS
  xilinx_analog.cpp
  adc_driver.c
  )

set (LIBS
  i3ds
  zmq
  pthread
  ${Boost_LIBRARIES}
  )

include_directories ("../include/")

add_executable (i3ds_piap_analog i3ds_piap_analog.cpp ${SRCS})
target_link_libraries (i3ds_piap_analog ${LIBS})
install(TARGETS i3ds_piap_analog DESTINATION bin)
