find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(analyzer_gui
    main.cpp
    widgets/MainWindow.cpp
    widgets/GraphView.cpp
    widgets/LineNumberEditor.cpp
    core/AnalyzerRunner.cpp
    resources/resources.qrc
)

target_link_libraries(analyzer_gui PRIVATE Qt6::Widgets)

set_target_properties(analyzer_gui PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)