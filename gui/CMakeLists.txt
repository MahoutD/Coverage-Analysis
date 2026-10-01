add_executable(coverage_gui WIN32
    include/main_window.h
    include/code_editor.h
    include/syntax_highlighter.h
    include/static_analysis_view.h
    include/coverage_view.h
    include/stimulus_view.h
    include/theme_manager.h
    include/graph_view.h
    include/graph_items.h
    include/graph_canvas_view.h
    include/localization_manager.h
    include/user_manual_dialog.h
    include/report_preview_dialog.h
    include/static_analysis_explorer_view.h
    include/splash_screen.h
    include/project_hub_dialog.h
    include/welcome_view.h
    include/stack_analysis_view.h

    src/main.cpp
    src/main_window.cpp
    src/code_editor.cpp
    src/syntax_highlighter.cpp
    src/static_analysis_view.cpp
    src/coverage_view.cpp
    src/stimulus_view.cpp
    src/theme_manager.cpp
    src/graph_view.cpp
    src/graph_items.cpp
    src/graph_canvas_view.cpp
    src/localization_manager.cpp
    src/user_manual_dialog.cpp
    src/report_preview_dialog.cpp
    src/static_analysis_explorer_view.cpp
    src/splash_screen.cpp
    src/project_hub_dialog.cpp
    src/welcome_view.cpp
    src/stack_analysis_view.cpp

    resources/resources.qrc
)

target_include_directories(coverage_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(coverage_gui PRIVATE
    coverage_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
)
