cmake_minimum_required(VERSION 3.16)
project(KmlArcGenerator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)

add_executable(KmlArcGenerator
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MapWidget.cpp
    MapWidget.h
    GeoMath.h
    DataModels.h
)

target_link_libraries(KmlArcGenerator PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# Bundle resources for Mac
if(APPLE)
    set_target_properties(KmlArcGenerator PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "KmlArcGenerator"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.KmlArcGenerator"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    )

    # Copy the JSON files to the Resources directory of the App bundle
    add_custom_command(TARGET KmlArcGenerator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/countries.json
        $<TARGET_FILE_DIR:KmlArcGenerator>/../Resources/countries.json
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/cities.json
        $<TARGET_FILE_DIR:KmlArcGenerator>/../Resources/cities.json
    )
elseif(WIN32)
    add_custom_command(TARGET KmlArcGenerator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/countries.json
        $<TARGET_FILE_DIR:KmlArcGenerator>/countries.json
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/cities.json
        $<TARGET_FILE_DIR:KmlArcGenerator>/cities.json
    )
else()
    add_custom_command(TARGET KmlArcGenerator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/countries.json
        $<TARGET_FILE_DIR:KmlArcGenerator>/countries.json
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/cities.json
        $<TARGET_FILE_DIR:KmlArcGenerator>/cities.json
    )
endif()
