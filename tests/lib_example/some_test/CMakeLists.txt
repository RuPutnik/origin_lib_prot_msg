generate_qtest(NAME_TEST "some_tests" USED_LIBS "${TARGET_NAME}" INCLUDE_DIRS "${LIB_PATH}" #[[ SOURCES "some_tests.cpp" "some_tests.h" ]] )

# Подключить модуль работы с исходниками
include(SourceSettings)

# Задать исходники таргету
set_sources_to_target(TARGET_NAME "some_tests" SOURCE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/../common")
