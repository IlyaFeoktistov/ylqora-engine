add_executable(yqlora_tests
    main.cpp
    test_node.cpp
    test_camera.cpp
    test_localization.cpp
    test_settings.cpp
)
target_link_libraries(yqlora_tests PRIVATE yqlora::core doctest::doctest)
target_compile_definitions(yqlora_tests PRIVATE
    YQLORA_LOCALES_DIR="${PROJECT_SOURCE_DIR}/assets/locales"
)
yqlora_set_warnings(yqlora_tests)

add_test(NAME yqlora_tests COMMAND yqlora_tests)
