/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Onion type class and Onion type-related functions.
 */

#include "onion_type.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../mob_script/onion_fsm.h"
#include "../mob/onion.h"


/**
 * @brief Constructs a new Onion type object.
 */
OnionType::OnionType() :
    MobType(MOB_CATEGORY_ONIONS) {
    
    nest = new PikminNestType();
    
    targetType = MOB_TARGET_FLAG_NONE;
    
    AreaEditorProp aep_pik_inside;
    aep_pik_inside.name = "Pikmin inside";
    aep_pik_inside.var = "pikmin_inside";
    aep_pik_inside.type = AEMP_TYPE_TEXT;
    aep_pik_inside.defValue = "";
    aep_pik_inside.tooltip =
        "How many Pikmin are inside. One word per maturity.\n"
        "The first three words are for the first type, "
        "then three more for the second type, and so on. "
        "e.g.: \"8 0 1\" means it has 8 leaf Pikmin inside, and 1 flower.";
    areaEditorProps.push_back(aep_pik_inside);
    
    onion_fsm::createFsm(this);
}


/**
 * @brief Destroys the Onion type object.
 */
OnionType::~OnionType() {
    delete nest;
}


/**
 * @brief Returns the vector of animation conversions.
 *
 * @return The vector.
 */
anim_conversion_vector OnionType::getAnimConversions() const {
    anim_conversion_vector v;
    v.push_back(std::make_pair(ONION_ANIM_IDLING, "idling"));
    v.push_back(std::make_pair(ONION_ANIM_GENERATING, "generating"));
    v.push_back(
        std::make_pair(ONION_ANIM_STOPPING_GENERATION, "stopping_generation")
    );
    return v;
}


/**
 * @brief Loads properties from a data file.
 *
 * @param file File to read from.
 */
void OnionType::loadCatProperties(DataNode* file) {
    nest->loadProperties(file);
    
    for(size_t s = 0; s < sounds.size(); s++) {
        if(sounds[s].name == "pop") {
            soundPopIdx = s;
        }
    }
}


/**
 * @brief Loads resources into memory.
 *
 * @param file File to read from.
 */
void OnionType::loadCatResources(DataNode* file) {
    //We don't actually need to load any, but we know that if this function
    //is run, then the animations are definitely loaded.
    //Now's a good time to check the leg body parts.
    for(size_t b = 0; b < nest->leg_body_parts.size(); b++) {
        if(animDb->findBodyPart(nest->leg_body_parts[b]) == INVALID) {
            game.errors.report(
                "The Onion type \"" + name + "\" specifies a leg body part "
                "called \"" + nest->leg_body_parts[b] + "\", "
                "but no such body part exists!"
            );
        }
    }
}
