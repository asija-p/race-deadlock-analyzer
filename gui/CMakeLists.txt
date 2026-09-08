find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(analyzer_gui
    main.cpp
    MainWindow.cpp
    AnalyzerRunner.cpp
    GraphView.cpp
    LineNumberEditor.cpp
    resources.qrc
)

target_link_libraries(analyzer_gui PRIVATE Qt6::Widgets)

set_target_properties(analyzer_gui PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)