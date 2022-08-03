add_executable(example example.cpp)
target_link_libraries(example PUBLIC ${PROJECT_NAME})
set_target_properties(example PROPERTIES CXX_STANDARD 20)
