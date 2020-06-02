
file(GLOB CPPFiles
  "*.cpp"
  )

include_directories(${PYTHON_INCLUDE_DIRS})

add_library(gimlet SHARED ${CPPFiles})
target_include_directories(gimlet PUBLIC .)
target_link_libraries(gimlet stdc++fs ${Boost_LIBRARIES})

# Installation targets

install (TARGETS gimlet DESTINATION lib)
