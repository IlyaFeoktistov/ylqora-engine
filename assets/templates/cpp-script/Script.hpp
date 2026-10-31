#pragma once

#include <yqlora/scripting/Script.hpp>

class $SCRIPT_NAME$ : public yqlora::Script {
public:
    YQLORA_SCRIPT($SCRIPT_NAME$)

    float speed = 1.0f;

    YQLORA_FIELDS(field("speed", &$SCRIPT_NAME$::speed))

protected:
    void onStart() override;
    void onUpdate(float deltaSeconds) override;
};
