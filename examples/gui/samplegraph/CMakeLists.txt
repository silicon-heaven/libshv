qt_add_executable(samplegraph
	main.cpp
	mainwindow.cpp
	timelinegraphwidget.h timelinegraphwidget.cpp timelinegraphwidget.ui
	histogramgraphwidget.h histogramgraphwidget.cpp histogramgraphwidget.ui
	)

target_link_libraries(samplegraph PRIVATE libshvvisu)

target_include_directories(samplegraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
	install(TARGETS samplegraph LIBRARY DESTINATION lib)
else()
	install(TARGETS samplegraph)
endif()
