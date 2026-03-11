#include "test_framework.h"
#include "upe/upe_character.h"

TEST(TestCharacterInit) {
    UpeCharacterDesc desc;
    desc.height = 2.0f;
    desc.radius = 0.5f;
    desc.step_height = 0.3f;
    desc.max_slope_angle = 45.0f;

    UpeCharacter character;
    UpeCharacter_Init(&character, &desc);
    REQUIRE(character.height == 2.0f);
    REQUIRE(character.radius == 0.5f);
}

TEST(TestCharacterMove) {
    UpeCharacterDesc desc;
    desc.height = 2.0f;
    desc.radius = 0.5f;
    desc.step_height = 0.3f;
    desc.max_slope_angle = 45.0f;

    UpeCharacter character;
    UpeCharacter_Init(&character, &desc);
    
    UpeVec3 dir = {1.0f, 0.0f, 0.0f};
    UpeCharacter_Move(&character, dir, 0.1f);
    REQUIRE(character.velocity.x > 0.0f);
}
