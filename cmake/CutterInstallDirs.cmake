set(CUTTER_DIR_NAME "rizin/cutter")
if(WIN32)
    set(CMAKE_INSTALL_BINDIR "." CACHE PATH "Executable install directory")
    set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "Include install directory")
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Library install directory")
    set(CMAKE_INSTALL_DATAROOTDIR "./" CACHE PATH "Resource installation directory")
    set(CUTTER_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}" CACHE PATH "Resource installation directory")
elseif(APPLE)
    if (CUTTER_ENABLE_PACKAGING)
        set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "Include install directory")
        set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Library install directory")
        set(CMAKE_INSTALL_DATAROOTDIR "./" CACHE PATH "Resource installation directory")
        set(CMAKE_INSTALL_BINDIR "../MacOS" CACHE PATH "Executable install directory") # BUNDLE step sets prefix to Resources
        set(CUTTER_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}" CACHE PATH "Resource installation directory")
    else()
        include(GNUInstallDirs)
        set(CUTTER_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/${CUTTER_DIR_NAME}" CACHE PATH "Resource installation directory")
    endif()
else()
    include(GNUInstallDirs)
    set(CUTTER_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}/${CUTTER_DIR_NAME}" CACHE PATH "Resource installation directory")
endif()
set(CUTTER_INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Cutter" CACHE PATH "CMake file install location")
