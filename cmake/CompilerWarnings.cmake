function(yqlora_set_warnings target)
    if(MSVC)
        # /utf-8: исходники и строки в UTF-8 (кириллица в коде и логах).
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    endif()
endfunction()
