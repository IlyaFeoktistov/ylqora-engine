add_executable(yqlora_tests
    main.cpp
    test_transform.cpp
    test_component.cpp
    test_node.cpp
    test_camera.cpp
    test_scene.cpp
    test_reflection.cpp
    test_serializer.cpp
    test_mesh.cpp
    test_input.cpp
    test_editor.cpp
    test_project.cpp
    test_session.cpp
    test_log.cpp
    test_world_transform.cpp
    test_history.cpp
    test_resources.cpp
    test_methods.cpp
    test_runtime_types.cpp
    test_project_files.cpp
    test_lifecycle.cpp
    test_localization.cpp
    test_settings.cpp
    test_component_rules.cpp
    test_node_tree.cpp
    test_file_format.cpp
    test_play_document.cpp
    test_project_manager.cpp
    test_frame_clock.cpp
)
target_link_libraries(yqlora_tests PRIVATE yqlora::internal yqlora::editor doctest::doctest)
target_compile_definitions(yqlora_tests PRIVATE
    YQLORA_LOCALES_DIR="${PROJECT_SOURCE_DIR}/assets/locales"
)
yqlora_set_warnings(yqlora_tests)

add_test(NAME yqlora_tests COMMAND yqlora_tests)
