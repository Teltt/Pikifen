/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Track type class and track type-related functions.
 */

#include "track_type.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/general_utils.h"
#include "../../util/string_utils.h"
#include "../mob_script/track_fsm.h"


/**
 * @brief Constructs a new track type object.
 */
TrackType::TrackType() :
    MobType(MOB_CATEGORY_TRACKS) {
    
    targetType = MOB_TARGET_FLAG_NONE;
    
    track_fsm::createFsm(this);
}


/**
 * @brief Returns the vector of animation conversions.
 *
 * @return The vector.
 */
anim_conversion_vector TrackType::getAnimConversions() const {
    anim_conversion_vector v;
    v.push_back(std::make_pair(DROP_ANIM_IDLING, "idling"));
    return v;
}


/**
 * @brief Loads properties from a data file.
 *
 * @param file File to read from.
 */
void TrackType::loadCatProperties(DataNode* file) {
    ReaderSetter rs(file);
    
    string riders_str;
    string riding_pose_str;
    DataNode* riders_node = nullptr;
    DataNode* riding_pose_node = nullptr;
    
    rs.set("cancellable_with_whistle", cancellableWithWhistle);
    rs.set("ride_speed", rideSpeed);
    rs.set("riders", riders_str, &riders_node);
    rs.set("riding_pose", riding_pose_str, &riding_pose_node);
    
    if(riders_node) {
        riders = 0;
        vector<string> riders_str_words = split(riders_str);
        for(size_t r = 0; r < riders_str_words.size(); r++) {
            if(riders_str_words[r] == "pikmin") {
                enableFlag(riders, TRACK_RIDER_FLAG_PIKMIN);
            } else if(riders_str_words[r] == "leaders") {
                enableFlag(riders, TRACK_RIDER_FLAG_LEADERS);
            } else {
                game.errors.report(
                    "Unknown type of rider \"" + riders_str_words[r] + "\"!",
                    riders_node
                );
            }
        }
    }
    
    if(riding_pose_node) {
        if(riding_pose_str == "stopped") {
            ridingPose = TRACK_RIDING_POSE_STOPPED;
        } else if(riding_pose_str == "sliding") {
            ridingPose = TRACK_RIDING_POSE_SLIDING;
        } else if(riding_pose_str == "climbing") {
            ridingPose = TRACK_RIDING_POSE_CLIMBING;
        } else {
            game.errors.report(
                "Unknown type of riding pose \"" + riding_pose_str + "\"!",
                riding_pose_node
            );
        }
    }
    
}


/**
 * @brief Loads resources into memory.
 *
 * @param file File to read from.
 */
void TrackType::loadCatResources(DataNode* file) {
    //We don't actually need to load any, but we know that if this function
    //is run, then the animations are definitely loaded.
    //Now's a good time to check if the track has 2+ checkpoints.
    if(animDb->bodyParts.size() < 2) {
        game.errors.report(
            "The track type \"" + name + "\" needs to have at least 2 "
            "checkpoints (body parts), but it only has " +
            i2s(animDb->bodyParts.size()) + "!"
        );
    }
}
