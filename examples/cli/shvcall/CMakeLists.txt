qt_add_executable(shvcall
    src/appclioptions.cpp
    src/application.cpp
    src/main.cpp
    )

target_link_libraries(shvcall PRIVATE libshviotqt)

install(TARGETS shvcall)
