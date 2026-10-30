#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/reflection/TypeName.hpp"

#include <string_view>

namespace yqlora {

// Раздел настроек проекта (см. docs/architecture/project-settings.md):
// класс со свойствами в реестре типов, у каждого раздела свой файл.
// Раздел объявляет та подсистема, которой он нужен:
//
//   class PhysicsSettings final : public SettingsSection {
//   public:
//       YQLORA_TYPE(PhysicsSettings, SettingsSection)
//       Vec3 gravity{0.0f, -9.81f, 0.0f};
//   };
//
//   registry.registerSettings<PhysicsSettings>("physics")   // settings/physics.json
//       .property("gravity", &PhysicsSettings::gravity);
//
//   project.settings().get<PhysicsSettings>().gravity
class YQLORA_API SettingsSection {
public:
    SettingsSection() = default;
    virtual ~SettingsSection();

    SettingsSection(const SettingsSection&) = delete;
    SettingsSection& operator=(const SettingsSection&) = delete;

    // Корень иерархии разделов, как Node у узлов: typeName чисто виртуальный,
    // поэтому без YQLORA_TYPE. Базы в реестре нет.
    using YqloraThisType = SettingsSection;
    using YqloraBaseType = void;

    static constexpr std::string_view staticTypeName()
    {
        return "SettingsSection";
    }

    virtual std::string_view typeName() const = 0;
};

} // namespace yqlora
