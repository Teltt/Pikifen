/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Drop type class and drop type-related functions.
 */

#include "drop_type.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../mob_script/drop_fsm.h"


/**
 * @brief Constructs a new drop type object.
 */
DropType::DropType() :
    MobType(MOB_CATEGORY_DROPS) {
    
    targetType = MOB_TARGET_FLAG_NONE;
    height = 8.0f;
    
    drop_fsm::createFsm(this);
}


/**
 * @brief Returns the vector of animation conversions.
 */
anim_conversion_vector DropType::getAnimConversions() const {
    anim_conversion_vector v;
    v.push_back(std::make_pair(DROP_ANIM_IDLING, "idling"));
    v.push_back(std::make_pair(DROP_ANIM_FALLING, "falling"));
    v.push_back(std::make_pair(DROP_ANIM_LANDING, "landing"));
    v.push_back(std::make_pair(DROP_ANIM_BUMPED, "bumped"));
    return v;
}


/**
 * @brief Loads properties from a data file.
 *
 * @param file File to read from.
 */
void DropType::loadCatProperties(DataNode* file) {
    ReaderSetter rs(file);
    
    string consumer_str;
    string effect_str;
    string spray_name_str;
    string status_name_str;
    DataNode* consumer_node = nullptr;
    DataNode* effect_node = nullptr;
    DataNode* spray_name_node = nullptr;
    DataNode* status_name_node = nullptr;
    DataNode* total_doses_node = nullptr;
    
    rs.set("consumer", consumer_str, &consumer_node);
    rs.set("effect", effect_str, &effect_node);
    rs.set("increase_amount", increaseAmount);
    rs.set("shrink_speed", shrinkSpeed);
    rs.set("spray_type_to_increase", spray_name_str, &spray_name_node);
    rs.set("status_to_give", status_name_str, &status_name_node);
    rs.set("total_doses", totalDoses, &total_doses_node);
    
    if(consumer_str == "pikmin") {
        consumer = DROP_CONSUMER_PIKMIN;
    } else if(consumer_str == "leaders") {
        consumer = DROP_CONSUMER_LEADERS;
    } else {
        game.errors.report(
            "Unknown consumer \"" + consumer_str + "\"!", consumer_node
        );
    }
    
    if(effect_str == "maturate") {
        effect = DROP_EFFECT_MATURATE;
    } else if(effect_str == "increase_sprays") {
        effect = DROP_EFFECT_INCREASE_SPRAYS;
    } else if(effect_str == "give_status") {
        effect = DROP_EFFECT_GIVE_STATUS;
    } else {
        game.errors.report(
            "Unknown drop effect \"" + effect_str + "\"!", effect_node
        );
    }
    
    if(effect == DROP_EFFECT_INCREASE_SPRAYS) {
        for(size_t s = 0; s < game.config.misc.sprayOrder.size(); s++) {
            if(
                game.config.misc.sprayOrder[s]->manifest->internalName ==
                spray_name_str
            ) {
                sprayTypeToIncrease = s;
                break;
            }
        }
        if(sprayTypeToIncrease == INVALID) {
            game.errors.report(
                "Unknown spray type \"" + spray_name_str + "\"!",
                spray_name_node
            );
        }
    }
    
    if(status_name_node) {
        auto s = game.content.statusTypes.list.find(status_name_str);
        if(s != game.content.statusTypes.list.end()) {
            statusToGive = s->second;
        } else {
            game.errors.report(
                "Unknown status type \"" + status_name_str + "\"!",
                status_name_node
            );
        }
    }
    
    if(totalDoses == 0) {
        game.errors.report(
            "The number of total doses cannot be zero!", total_doses_node
        );
    }
    
    shrinkSpeed /= 100.0f;
}
