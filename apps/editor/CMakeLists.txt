# Программа yqlora: движок с редактором. Имя цели другое, потому что
# yqlora уже занято библиотекой движка.
add_executable(yqlora_app main.cpp)
# WIN32_EXECUTABLE: на Windows без окна консоли, она открывается флагом --debug.
set_target_properties(yqlora_app PROPERTIES OUTPUT_NAME yqlora WIN32_EXECUTABLE TRUE)
if(MSVC)
    # Оконное приложение MSVC по умолчанию начинается с WinMain, у нас обычный main.
    target_link_options(yqlora_app PRIVATE /ENTRY:mainCRTStartup)
endif()
target_link_libraries(yqlora_app PRIVATE yqlora::editor_ui)
yqlora_set_warnings(yqlora_app)

# Ресурсы движка кладём рядом с исполняемым файлом.
add_custom_command(TARGET yqlora_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:yqlora_app>/assets
)
# Редактор собирает модули игры против SDK рядом с собой.
add_dependencies(yqlora_app yqlora_sdk)
