include_directories(${Boost_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/common)

if (DEBUG_COUNTS)
  add_definitions(-DDEBUG_COUNTS)
endif()

add_executable (mine-smi mine-smi.cpp FPTree.cpp) 
target_link_libraries(mine-smi stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options ${CMAKE_THREAD_LIBS_INIT})

add_executable (mine-rfi mine-rfi.cpp FPTree.cpp) 
target_link_libraries(mine-rfi stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options ${CMAKE_THREAD_LIBS_INIT})

add_executable (mine-afi mine-afi.cpp FPTree.cpp) 
target_link_libraries(mine-afi stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options ${CMAKE_THREAD_LIBS_INIT})

add_executable (mine-mfi mine-mfi.cpp FPTree.cpp) 
target_link_libraries(mine-mfi stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options ${CMAKE_THREAD_LIBS_INIT})

add_executable (mine-vert-topK-AFD mine-vert-topK-AFD.cpp) 
target_link_libraries(mine-vert-topK-AFD stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options ${CMAKE_THREAD_LIBS_INIT})

add_executable (mine-vert-rho-AFD mine-vert-rho-AFD.cpp) 
target_link_libraries(mine-vert-rho-AFD stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options ${CMAKE_THREAD_LIBS_INIT})

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mine-smi
  DESTINATION bin
  RENAME ${CMAKE_PROJECT_NAME}-mine-smi)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mine-rfi
  DESTINATION bin
  RENAME ${CMAKE_PROJECT_NAME}-mine-rfi)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mine-afi
  DESTINATION bin
  RENAME ${CMAKE_PROJECT_NAME}-mine-afi)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mine-mfi
  DESTINATION bin
  RENAME ${CMAKE_PROJECT_NAME}-mine-mfi)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mine-vert-topK-AFD
  DESTINATION bin
  RENAME ${CMAKE_PROJECT_NAME}-mine-vert-topK-AFD)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mine-vert-rho-AFD
  DESTINATION bin
  RENAME ${CMAKE_PROJECT_NAME}-mine-vert-rho-AFD)
