include(ExternalProject)

ExternalProject_Add(BoostSML
    URL "https://raw.githubusercontent.com/boost-experimental/sml/master/include/boost/sml.hpp"
    DOWNLOAD_NO_EXTRACT 1
    DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/boost/"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )
