qt_add_library(libshvcoreqt SHARED
    src/data/valuechange.cpp
    src/exception.cpp
    src/log.cpp
    src/utils.cpp
    src/utils/clioptions.cpp
    src/utils/versioninfo.cpp
    )
target_link_libraries(libshvcoreqt PUBLIC Qt::Core libshvcore)
target_include_directories(libshvcoreqt PUBLIC include)
target_compile_definitions(libshvcoreqt PRIVATE SHVCOREQT_BUILD_DLL)

if(WIN32)
    # Need these for some WinAPI calls
    target_compile_definitions(libshvcoreqt PRIVATE UNICODE)
    target_compile_definitions(libshvcoreqt PRIVATE _UNICODE)
endif()

install(TARGETS libshvcoreqt)
