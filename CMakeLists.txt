# Определить минимально допустимую версию CMake для данного проекта
cmake_minimum_required(VERSION 3.16)

# Определить текущий проект
project("Проект"
        VERSION 1.0
        DESCRIPTION "Описание"
        LANGUAGES CXX
)

# Определить путь к модулям и библиотекам
set(__PATH_TO_MODULES__ "${PROJECT_SOURCE_DIR}/modules" CACHE STRING "Путь к используемым внешним модулям")

# Настроить возможность подключения модулей cmake
include(${__PATH_TO_MODULES__}/tlk_build/cmake/Init.cmake)

# Подключить исходники
add_subdirectory(src)

# Подключить тесты
if(NOT CMAKE_BUILD_TYPE MATCHES "ReleaseInstall")
    add_subdirectory(tests)
endif()
