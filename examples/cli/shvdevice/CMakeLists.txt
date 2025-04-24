qt_add_executable(shvdevice
	src/main.cpp
	src/appclioptions.cpp
	src/application.cpp
	)

target_link_libraries(shvdevice PRIVATE libshviotqt Qt::Gui)

if(ANDROID)
	install(TARGETS shvdevice LIBRARY DESTINATION lib)
else()
	install(TARGETS shvdevice)
endif()
