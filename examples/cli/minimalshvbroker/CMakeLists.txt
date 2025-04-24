qt_add_executable(minimalshvbroker
	src/main.cpp
	src/application.cpp
	)

target_link_libraries(minimalshvbroker PRIVATE libshvbroker Qt::Gui)

if(ANDROID)
	install(TARGETS minimalshvbroker LIBRARY DESTINATION lib)
else()
	install(TARGETS minimalshvbroker)
endif()
