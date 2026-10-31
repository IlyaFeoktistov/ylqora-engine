#pragma once

// YQLORA_API помечает то, что видно снаружи библиотеки движка: редактору,
// запуску игры и dll со скриптами. Остальное остаётся внутри. На Linux
// библиотека собирается со скрытыми по умолчанию символами, поэтому забытая
// пометка видна сразу, а не только на Windows.
#if defined(_WIN32)
#if defined(YQLORA_BUILDING_ENGINE)
#define YQLORA_API __declspec(dllexport)
#else
#define YQLORA_API __declspec(dllimport)
#endif
#else
#define YQLORA_API __attribute__((visibility("default")))
#endif

// YQLORA_MODULE_EXPORT помечает входную функцию модуля игры (YQLORA_MODULE_ENTRY):
// её ищет по имени движок, загрузивший библиотеку.
#if defined(_WIN32)
#define YQLORA_MODULE_EXPORT __declspec(dllexport)
#else
#define YQLORA_MODULE_EXPORT __attribute__((visibility("default")))
#endif
