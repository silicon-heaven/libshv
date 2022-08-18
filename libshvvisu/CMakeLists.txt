add_library(libshvvisu SHARED
    images/images.qrc
    src/logview/dlgloginspector.cpp
    src/logview/logmodel.cpp
    src/logview/logsortfilterproxymodel.cpp
    src/logwidget/errorlogmodel.cpp
    src/logwidget/logtablemodelbase.cpp
    src/logwidget/logwidget.cpp
    src/svgscene/graphicsview.cpp
    src/svgscene/groupitem.cpp
    src/svgscene/saxhandler.cpp
    src/svgscene/simpletextitem.cpp
    src/svgscene/types.cpp
    src/svgscene/visucontroller.cpp
    src/timeline/channelfilter.cpp
    src/timeline/channelfilterdialog.cpp
    src/timeline/channelfiltermodel.cpp
    src/timeline/channelfiltersortfilterproxymodel.cpp
    src/timeline/channelprobe.cpp
    src/timeline/channelprobewidget.cpp
    src/timeline/fulltextfilter.cpp
    src/timeline/graph.cpp
    src/timeline/graphbuttonbox.cpp
    src/timeline/graphchannel.cpp
    src/timeline/graphmodel.cpp
    src/timeline/graphview.cpp
    src/timeline/graphwidget.cpp
    src/timeline/sample.cpp
    src/widgets/timezonecombobox.cpp
    )

target_link_libraries(libshvvisu PUBLIC Qt::Widgets libshviotqt)
if(USE_QT6)
    target_link_libraries(libshvvisu PUBLIC Qt::Core5Compat)
endif()
target_include_directories(libshvvisu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(libshvvisu PUBLIC include)
target_compile_definitions(libshvvisu PRIVATE SHVVISU_BUILD_DLL)

install(TARGETS libshvvisu)
