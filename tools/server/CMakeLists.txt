add_executable(rxsd 
  rxs_srv.c
  )

find_package (Threads)

target_link_libraries(rxsd  
  rxs_container
  rxs_protocol
  rxs_logger)
