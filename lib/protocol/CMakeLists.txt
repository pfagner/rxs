add_library(rxs_protocol
  internal_types.c
  protocol_rxs.c
  protocol_rxs_client.c
  protocol_rxs_server.c
  parser.c
  generic.c
  )

target_link_libraries(rxs_protocol
  rxs_container
  rxs_logger
  )
