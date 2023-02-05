add_executable(rxsc 
  rxs_cli.c
  )

find_package (Threads)

target_link_libraries(rxsc  
  rxs_container
  rxs_protocol
  rxs_logger)
