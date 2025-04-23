qt_add_executable(shvcall
    src/appclioptions.cpp
    src/application.cpp
    src/main.cpp
    )

target_link_libraries(shvcall PRIVATE libshviotqt Qt::Gui)

if(ANDROID)
	install(TARGETS shvcall LIBRARY DESTINATION lib)
else()
	install(TARGETS shvcall)
endif()
