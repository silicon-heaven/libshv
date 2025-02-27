qt_add_executable(minimalshvbroker
	src/main.cpp
	src/application.cpp
	)

target_link_libraries(minimalshvbroker PRIVATE libshvbroker)

install(TARGETS minimalshvbroker)
