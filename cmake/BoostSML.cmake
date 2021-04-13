include(ExternalProject)

ExternalProject_Add(BoostSML
    URL "https://raw.githubusercontent.com/boost-ext/sml/ebd6f54bc3a9c3afec2787b5ad629426815a279f/include/boost/sml.hpp"
    DOWNLOAD_NO_EXTRACT 1
    DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/boost/"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )
