#pragma once

#include <string_view>

// Иконки из шрифта Lucide (assets/fonts/lucide.ttf, пакет lucide-static 1.54.0).
// Шрифт подмешан к основному, поэтому иконку можно ставить прямо в текст:
// std::string(icons::kFolder) + " Assets". Коды взяты из font/info.json пакета,
// в комментарии - имя иконки на lucide.dev.
namespace yqlora::icons {

// Инструменты окна сцены.
inline constexpr const char* kSelect = "\uE1C3"; // mouse-pointer-2
inline constexpr const char* kMove = "\uE2E5";   // move-3d
inline constexpr const char* kRotate = "\uE2EA"; // rotate-3d
inline constexpr const char* kScale = "\uE2EB";  // scale-3d
inline constexpr const char* kLocal = "\uE061";  // box
inline constexpr const char* kGlobal = "\uE0E8"; // globe

// Типы узлов в дереве сцены.
inline constexpr const char* kNode = "\uE345";           // circle-dot
inline constexpr const char* kNode3D = "\uE2FE";         // axis-3d
inline constexpr const char* kCamera3D = "\uE1A5";       // video
inline constexpr const char* kMeshInstance3D = "\uE524"; // cuboid

// Файлы и действия.
inline constexpr const char* kFolder = "\uE0D7";      // folder
inline constexpr const char* kFolderOpen = "\uE247";  // folder-open
inline constexpr const char* kImport = "\uE334";      // folder-input
inline constexpr const char* kFile = "\uE0C0";        // file
inline constexpr const char* kScene = "\uE29B";       // clapperboard
inline constexpr const char* kMainScene = "\uE176";   // star
inline constexpr const char* kAdd = "\uE13D";         // plus
inline constexpr const char* kAddFolder = "\uE0D9";   // folder-plus
inline constexpr const char* kAddFile = "\uE0C9";     // file-plus
inline constexpr const char* kRefresh = "\uE145";     // refresh-cw
inline constexpr const char* kDelete = "\uE18E";      // trash-2
inline constexpr const char* kRename = "\uE1F9";      // pencil
inline constexpr const char* kSave = "\uE14D";        // save
inline constexpr const char* kSearch = "\uE151";      // search

// Иконка для типа узла; для неизвестного типа - иконка простого Node.
inline const char* forNodeType(std::string_view type)
{
    if (type == "Camera3D") {
        return kCamera3D;
    }
    if (type == "MeshInstance3D") {
        return kMeshInstance3D;
    }
    if (type == "Node3D") {
        return kNode3D;
    }

    return kNode;
}

} // namespace yqlora::icons
