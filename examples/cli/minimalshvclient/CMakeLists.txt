qt_add_executable(minimalshvclient
	src/main.cpp
	src/application.cpp
	)

target_link_libraries(minimalshvclient PRIVATE libshviotqt Qt::Gui)

if(ANDROID)
	install(TARGETS minimalshvclient LIBRARY DESTINATION lib)
else()
	install(TARGETS minimalshvclient)
endif()
