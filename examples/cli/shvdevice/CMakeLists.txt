qt_add_executable(shvdevice
	src/main.cpp
	src/appclioptions.cpp
	src/application.cpp
	)

target_link_libraries(shvdevice PRIVATE libshviotqt)

install(TARGETS shvdevice)
