/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Decoration type class and decoration type-related functions.
 */

#include "decoration_type.h"

#include "../mob_script/decoration_fsm.h"
#include "../mob/decoration.h"
#include "../other/mob_script.h"


/**
 * @brief Constructs a new decoration type object.
 */
DecorationType::DecorationType() :
    MobType(MOB_CATEGORY_DECORATIONS) {
    
    targetType = MOB_TARGET_FLAG_NONE;
    
    AreaEditorProp aep_random_anim_delay;
    aep_random_anim_delay.name = "Random animation delay";
    aep_random_anim_delay.var = "random_animation_delay";
    aep_random_anim_delay.type = AEMP_TYPE_BOOL;
    aep_random_anim_delay.defValue = "true";
    aep_random_anim_delay.tooltip =
        "If this decoration type can have a random animation delay,\n"
        "this property makes this decoration use it or not.";
    areaEditorProps.push_back(aep_random_anim_delay);
    
    AreaEditorProp aep_random_tint;
    aep_random_tint.name = "Random tint";
    aep_random_tint.var = "random_tint";
    aep_random_tint.type = AEMP_TYPE_BOOL;
    aep_random_tint.defValue = "true";
    aep_random_tint.tooltip =
        "If this decoration type can have a random color tint,\n"
        "this property makes this decoration use it or not.";
    areaEditorProps.push_back(aep_random_tint);
    
    AreaEditorProp aep_random_scale;
    aep_random_scale.name = "Random scale";
    aep_random_scale.var = "random_scale";
    aep_random_scale.type = AEMP_TYPE_BOOL;
    aep_random_scale.defValue = "true";
    aep_random_scale.tooltip =
        "If this decoration type can have a random scale,\n"
        "this property makes this decoration use it or not.";
    areaEditorProps.push_back(aep_random_scale);
    
    AreaEditorProp aep_random_rotation;
    aep_random_rotation.name = "Random rotation";
    aep_random_rotation.var = "random_rotation";
    aep_random_rotation.type = AEMP_TYPE_BOOL;
    aep_random_rotation.defValue = "true";
    aep_random_rotation.tooltip =
        "If this decoration type can have a random scale,\n"
        "this property makes this decoration use it or not.";
    areaEditorProps.push_back(aep_random_rotation);
    
    blackoutRadius = 0.0f;
    
    decoration_fsm::createFsm(this);
}


/**
 * @brief Returns the vector of animation conversions.
 */
anim_conversion_vector DecorationType::getAnimConversions() const {
    anim_conversion_vector v;
    v.push_back(std::make_pair(DECORATION_ANIM_IDLING, "idling"));
    v.push_back(std::make_pair(DECORATION_ANIM_BUMPED, "bumped"));
    return v;
}


/**
 * @brief Loads properties from a data file.
 *
 * @param file File to read from.
 */
void DecorationType::loadCatProperties(DataNode* file) {
    ReaderSetter rs(file);
    
    rs.set("random_animation_delay", randomAnimationDelay);
    rs.set("rotation_random_variation", rotationRandomVariation);
    rs.set("scale_random_variation", scaleRandomVariation);
    rs.set("tint_random_maximum", tintRandomMaximum);
    
    rotationRandomVariation = degToRad(rotationRandomVariation);
}
