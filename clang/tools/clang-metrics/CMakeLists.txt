add_clang_executable(clang-metrics
  main.cpp
  )
  
target_link_libraries(clang-metrics
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  clangMetrics
  )
  
install(TARGETS clang-metrics
  RUNTIME DESTINATION bin)