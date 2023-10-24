add_library(libshvvisu
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

	include/shv/visu/logview/dlgloginspector.h
	include/shv/visu/logview/logsortfilterproxymodel.h
	include/shv/visu/logview/logmodel.h
	include/shv/visu/timeline/sample.h
	include/shv/visu/timeline/graphview.h
	include/shv/visu/timeline/channelprobe.h
	include/shv/visu/timeline/graphwidget.h
	include/shv/visu/timeline/graph.h
	include/shv/visu/timeline/channelprobewidget.h
	include/shv/visu/timeline/graphmodel.h
	include/shv/visu/timeline/channelfiltermodel.h
	include/shv/visu/timeline/fulltextfilter.h
	include/shv/visu/timeline/channelfilterdialog.h
	include/shv/visu/timeline/graphbuttonbox.h
	include/shv/visu/timeline/graphchannel.h
	include/shv/visu/timeline/channelfilter.h
	include/shv/visu/shvvisuglobal.h
	include/shv/visu/logtablemodelbase.h
	include/shv/visu/logwidget.h
	include/shv/visu/timezonecombobox.h
	include/shv/visu/svgscene/visucontroller.h
	include/shv/visu/svgscene/graphicsview.h
	include/shv/visu/svgscene/types.h
	include/shv/visu/svgscene/saxhandler.h
	include/shv/visu/svgscene/simpletextitem.h
	include/shv/visu/svgscene/groupitem.h
	include/shv/visu/errorlogmodel.h
	)

target_link_libraries(libshvvisu PUBLIC Qt::Widgets libshviotqt)
target_include_directories(libshvvisu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(libshvvisu PUBLIC include)
target_compile_definitions(libshvvisu PRIVATE SHVVISU_BUILD_DLL)

install(TARGETS libshvvisu)
