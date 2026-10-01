add_executable(coverage_cli
    src/main.cpp
)

target_link_libraries(coverage_cli PRIVATE
    coverage_core
    Qt6::Core
)
