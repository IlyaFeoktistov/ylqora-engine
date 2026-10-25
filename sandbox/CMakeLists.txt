add_executable(sandbox main.cpp)
target_link_libraries(sandbox PRIVATE yqlora::yqlora)
yqlora_set_warnings(sandbox)

# Ресурсы движка кладём рядом с исполняемым файлом.
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:sandbox>/assets
)
