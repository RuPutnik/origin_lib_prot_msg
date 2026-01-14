# Фильтровать повторное подключение таргета
include_guard(GLOBAL)

project("Executable"
        VERSION 1.0
        DESCRIPTION "Пример библиотеки"
        LANGUAGES CXX
)

# Определить имя таргета проекта
set(TARGET_NAME "${PROJECT_NAME}")

# Подключить модуль работы с исходниками
include(GeneralSettings)

#=========================== Настройки таргетов =================================

# Подключить модуль работы с исходниками
include(SourceSettings)

# Определить таргет проекта
add_kivk_executable(TARGET_NAME ${TARGET_NAME})

# Задать исходники таргету
set_sources_to_target(PRIVATE TARGET_NAME ${TARGET_NAME})

## Инициализировать и задать таргету flex/bison
#set_flex_bison_to_target(TARGET_NAME ${TARGET_NAME}
#                         LEXER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/scanner.l
#                         PARSER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
#                         INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/<flex_bison_directories>"
#)

#============================= Библиотеки =======================================

target_link_libraries(${TARGET_NAME} PRIVATE "Example")

# Подключить модуль библиотек
include(LibsSettingsQt)

# Подключить необходимые библиотеки Qt
link_qt_libraries(TARGET_NAME ${TARGET_NAME} QT_LIBS "Core" "Widgets")

## Подключить сторонние библиотеки
#link_subdir_libraries(
#    PUBLIC
#    TARGET_NAME ${TARGET_NAME}
#    MODULE_PATH ${__PATH_TO_MODULES__}/some_lib
#    MODULE_TARGETS "SomeLib1" "SomeLib2"
#)
