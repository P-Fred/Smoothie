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
