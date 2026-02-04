cmake_minimum_required(VERSION 3.10)
project(curious-cat)

find_package(libobs REQUIRED)

add_library(curious-cat MODULE
    curious-cat.c
    cat-behavior.c
)

target_link_libraries(curious-cat
    libobs::libobs
)

target_include_directories(curious-cat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Windows-specific: link user32 for global mouse tracking
if(WIN32)
    target_link_libraries(curious-cat user32)
endif()

install(TARGETS curious-cat
    LIBRARY DESTINATION obs-plugins/${CMAKE_SYSTEM_NAME}
    RUNTIME DESTINATION obs-plugins/${CMAKE_SYSTEM_NAME}
)

install(FILES resources/default-cat.png
    DESTINATION obs-plugins/${CMAKE_SYSTEM_NAME}/data
)
