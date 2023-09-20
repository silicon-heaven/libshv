add_executable(shvcall
    src/appclioptions.cpp
    src/application.cpp
    src/main.cpp
    )

target_link_libraries(shvcall libshviotqt)

install(TARGETS shvcall)
