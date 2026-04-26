#pragma once

namespace WzLibCpp {

/// <summary>
/// 标识Wz_File的内部类型。
/// </summary>
enum class Wz_Type {
    Unknown = 0,
    Base,
    Character,
    Effect,
    Etc,
    Item,
    Map,
    Mob,
    Morph,
    Npc,
    Quest,
    Reactor,
    Skill,
    Sound,
    String,
    TamingMob,
    UI,
};

} // namespace WzLibCpp