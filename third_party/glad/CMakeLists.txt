# Сгенерировано glad2: glad --api gl:core=3.3 --extensions "" c
add_library(glad STATIC src/gl.c)
target_include_directories(glad PUBLIC include)
set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)
