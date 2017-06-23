add_clang_tool(clang-metrics
  main.cpp
  )
  
target_link_libraries(clang-metrics
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  clangMetrics
  )
  
install(TARGETS clang-metrics
  RUNTIME DESTINATION bin)