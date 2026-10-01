add_library(coverage_core STATIC
    include/coverage/types.h
    include/coverage/models.h
    include/coverage/app_config.h
    include/coverage/project.h
    include/coverage/static_analyzer.h
    include/coverage/instrumenter.h
    include/coverage/coverage_engine.h
    include/coverage/stimulus_engine.h
    include/coverage/backend_service.h
    include/coverage/graph_generator.h
    include/coverage/graph_layout.h
    include/coverage/logger.h
    include/coverage/report_generator.h
    include/coverage/stack_analyzer.h

    src/models.cpp
    src/app_config.cpp
    src/project.cpp
    src/static_analyzer.cpp
    src/instrumenter.cpp
    src/coverage_engine.cpp
    src/stimulus_engine.cpp
    src/backend_service.cpp
    src/graph_generator.cpp
    src/graph_layout.cpp
    src/logger.cpp
    src/report_generator.cpp
    src/stack_analyzer.cpp
)

target_include_directories(coverage_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tools/graphviz/Graphviz-12.2.1-win64/include
)

target_link_libraries(coverage_core PUBLIC
    Qt6::Core
    ${CMAKE_SOURCE_DIR}/tools/graphviz/Graphviz-12.2.1-win64/lib/gvc.lib
    ${CMAKE_SOURCE_DIR}/tools/graphviz/Graphviz-12.2.1-win64/lib/cgraph.lib
    ${CMAKE_SOURCE_DIR}/tools/graphviz/Graphviz-12.2.1-win64/lib/cdt.lib
)
