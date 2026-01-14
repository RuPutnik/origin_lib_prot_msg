# Фильтровать повторное подключение таргета
include_guard(GLOBAL)

# Определить имя таргета проекта
set(TARGET_NAME Protocol)

# Подключить модуль работы с исходниками
include(GeneralSettings)

#=========================== Настройки таргетов =================================

# Подключить модуль работы с исходниками
include(SourceSettings)

# Определить таргет проекта
add_kivk_library(TARGET_NAME ${TARGET_NAME} STATIC)

# Задать исходники таргету
set_sources_to_target(PRIVATE TARGET_NAME ${TARGET_NAME})

# Пробросить интерфейс
set_interface_to_target(PUBLIC TARGET_NAME ${TARGET_NAME} INTERFACE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/interface)
