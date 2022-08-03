add_executable(test test.cpp)
target_link_libraries(test PUBLIC ${PROJECT_NAME})
set_target_properties(test PROPERTIES CXX_STANDARD 20)