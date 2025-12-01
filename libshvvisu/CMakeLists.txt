qt_add_library(libshvvisu
	images/images.qrc
	src/logview/dataviewfilterselector.cpp
	src/logview/dataviewfilterselector.ui
	src/logview/dlgloginspector.cpp
	src/logview/dlgloginspector.ui
	src/logview/logmodel.cpp
	src/logview/logsortfilterproxymodel.cpp
	src/logwidget/errorlogmodel.cpp
	src/logwidget/logtablemodelbase.cpp
	src/logwidget/logwidget.cpp
	src/logwidget/logwidget.ui
	src/svgscene/graphicsview.cpp
	src/svgscene/groupitem.cpp
	src/svgscene/saxhandler.cpp
	src/svgscene/simpletextitem.cpp
	src/svgscene/types.cpp
	src/svgscene/visucontroller.cpp
	src/svgscene/rectitem.cpp

	src/timeline/channelfilter.cpp
	src/timeline/channelfilterdialog.cpp
	src/timeline/channelfilterdialog.ui
	src/timeline/channelfiltermodel.cpp
	src/timeline/channelfiltersortfilterproxymodel.cpp
	src/timeline/channelprobe.cpp
	src/timeline/channelprobewidget.cpp
	src/timeline/channelprobewidget.ui
	src/timeline/fulltextfilter.cpp
	src/timeline/graph.cpp
	src/timeline/graphbuttonbox.cpp
	src/timeline/graphchannel.cpp
	src/timeline/graphmodel.cpp
	src/timeline/graphview.cpp
	src/timeline/graphwidget.cpp
	src/timeline/sample.cpp

	src/widgets/timezonecombobox.cpp
	src/widgets/dlggetsinceuntil.cpp
	src/widgets/dlggetsinceuntil.ui
	src/widgets/sinceuntildatetimeedit.cpp

	include/shv/visu/logview/dataviewfilterselector.h
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
	include/shv/visu/svgscene/visucontroller.h
	include/shv/visu/svgscene/graphicsview.h
	include/shv/visu/svgscene/types.h
	include/shv/visu/svgscene/saxhandler.h
	include/shv/visu/svgscene/simpletextitem.h
	include/shv/visu/svgscene/groupitem.h
	include/shv/visu/svgscene/rectitem.h

	include/shv/visu/widgets/timezonecombobox.h
	include/shv/visu/widgets/dlggetsinceuntil.h
	include/shv/visu/widgets/sinceuntildatetimeedit.h

	include/shv/visu/errorlogmodel.h
	)

libshv_set_translations(libshvvisu "${LIBSHV_PROJECT_TRANSLATIONS}")

add_library(libshv::libshvvisu ALIAS libshvvisu)

target_link_libraries(libshvvisu PUBLIC Qt::Widgets libshviotqt)
target_include_directories(libshvvisu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(libshvvisu PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
	)
target_compile_definitions(libshvvisu PRIVATE SHVVISU_BUILD_DLL)

function(add_shvvisu_test test_name)
	add_executable(test_visu_${test_name}
		tests/test_${test_name}.cpp
		)
	target_link_libraries(test_visu_${test_name} libshvvisu doctest::doctest)
	add_test(NAME test_visu_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_visu_${test_name}>)
endfunction(add_shvvisu_test)

if(BUILD_TESTING)
	add_shvvisu_test(timeline)
endif()

if(LIBSHV_WITH_SVG)
	target_compile_definitions(libshvvisu PUBLIC WITH_SHV_SVG)
	target_link_libraries(libshvvisu PUBLIC Qt::Svg)
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/shv" TYPE INCLUDE)

# install(TARGETS libshvvisu EXPORT libshvConfig)
# if(NOT EMSCRIPTEN)
# 	export(TARGETS libshvvisu NAMESPACE libshv:: FILE "${CMAKE_CURRENT_BINARY_DIR}/libshvConfig.cmake")
# endif()
