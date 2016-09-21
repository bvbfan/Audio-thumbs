
cmake_minimum_required(VERSION 2.8.12)
project (AudioThumbs)

set(REQUIRED_QT_VERSION 5.3.0)
set(KF5_MIN_VERSION 5.8.0)

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED
    Core
)

find_package(KF5KIO ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    KIOWidgets
)

include(FindPackageHandleStandardArgs)

find_path(TAGLIB_INCLUDES
            NAMES tag.h
            PATH_SUFFIXES taglib
            PATHS
            ${INCLUDE_INSTALL_DIR}
)

find_library(TAGLIB_LIBRARIES
                NAMES tag
                PATHS
                ${LIB_INSTALL_DIR}
)

find_package_handle_standard_args(Taglib DEFAULT_MSG TAGLIB_INCLUDES TAGLIB_LIBRARIES)

if(TAGLIB_FOUND)
    message(STATUS "Found TagLib: ${TAGLIB_LIBRARIES}")
else(TAGLIB_FOUND)
    message(FATAL_ERROR "Could not find Taglib")
endif(TAGLIB_FOUND)

add_subdirectory(src)
