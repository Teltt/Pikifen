/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Pile finite state machine logic.
 */

#include <algorithm>

#include "pile_fsm.h"

#include "../../core/const.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/string_utils.h"
#include "../mob/pile.h"
#include "../mob/resource.h"
#include "gen_mob_fsm.h"


using std::size_t;


/**
 * @brief Creates the finite state machine for the pile's logic.
 *
 * @param typ Mob type to create the finite state machine for.
 */
void pile_fsm::createFsm(MobType* typ) {
    EasyFsmCreator efc;
    
    efc.newState("idling", PILE_STATE_IDLING); {
        efc.newEvent(MOB_EV_ON_ENTER); {
            efc.run(pile_fsm::becomeIdle);
        }
        efc.newEvent(MOB_EV_HITBOX_TOUCH_N_A); {
            efc.run(pile_fsm::beAttacked);
        }
    }
    
    
    typ->states = efc.finish();
    typ->firstStateIdx = fixStates(typ->states, "idling", typ);
    
    //Check if the number in the enum and the total match up.
    engineAssert(
        typ->states.size() == N_PILE_STATES,
        i2s(typ->states.size()) + " registered, " +
        i2s(N_PILE_STATES) + " in enum."
    );
}


/**
 * @brief Handles being attacked, and checks if it must drop another
 * resource or not.
 *
 * @param m The mob.
 * @param info1 Unused.
 * @param info2 Unused.
 */
void pile_fsm::beAttacked(Mob* m, void* info1, void* info2) {
    gen_mob_fsm::beAttacked(m, info1, info2);
    
    HitboxInteraction* info = (HitboxInteraction*) info1;
    Pile* pil_ptr = (Pile*) m;
    
    size_t amount_before = pil_ptr->amount;
    int intended_amount =
        ceil(pil_ptr->health / pil_ptr->pilType->healthPerResource);
    int amount_to_spawn = (int) pil_ptr->amount - intended_amount;
    amount_to_spawn = std::max((int) 0, amount_to_spawn);
    
    if(amount_to_spawn == 0) return;
    
    if(amount_to_spawn > 1 && !pil_ptr->pilType->canDropMultiple) {
        //Can't drop multiple? Let's knock that number down.
        amount_to_spawn = 1;
        intended_amount = (int) (pil_ptr->amount - 1);
        pil_ptr->health =
            pil_ptr->pilType->healthPerResource * intended_amount;
    }
    
    Resource* resource_to_pick_up = nullptr;
    Pikmin* pikmin_to_start_carrying = nullptr;
    
    for(size_t r = 0; r < (size_t) amount_to_spawn; r++) {
        Point spawn_pos;
        float spawn_z = 0;
        float spawn_angle = 0;
        float spawn_h_speed = 0;
        float spawn_v_speed = 0;
        
        if(r == 0 && info->mob2->type->category->id == MOB_CATEGORY_PIKMIN) {
            pikmin_to_start_carrying = (Pikmin*) (info->mob2);
            //If this was a Pikmin's attack, spawn the first resource nearby
            //so it can pick it up.
            spawn_angle =
                getAngle(pil_ptr->pos, pikmin_to_start_carrying->pos);
            spawn_pos =
                pikmin_to_start_carrying->pos +
                angleToCoordinates(
                    spawn_angle, game.config.pikmin.standardRadius * 1.5
                );
        } else {
            spawn_pos = pil_ptr->pos;
            spawn_z = pil_ptr->height + 32.0f;
            spawn_angle = game.rng.f(0, TAU);
            spawn_h_speed = pil_ptr->radius * 3;
            spawn_v_speed = 600.0f;
        }
        
        Resource* new_resource =
            (
                (Resource*)
                createMob(
                    game.mobCategories.get(MOB_CATEGORY_RESOURCES),
                    spawn_pos, pil_ptr->pilType->contents,
                    spawn_angle, "",
        [pil_ptr] (Mob * m) { ((Resource*) m)->originPile = pil_ptr; }
                )
            );
            
        new_resource->z = spawn_z;
        new_resource->speed.x = cos(spawn_angle) * spawn_h_speed;
        new_resource->speed.y = sin(spawn_angle) * spawn_h_speed;
        new_resource->speedZ = spawn_v_speed;
        new_resource->links = pil_ptr->links;
        
        if(r == 0) {
            resource_to_pick_up = new_resource;
        }
    }
    
    if(pikmin_to_start_carrying) {
        pikmin_to_start_carrying->forceCarry(resource_to_pick_up);
    }
    
    pil_ptr->amount = intended_amount;
    
    if(amount_before == pil_ptr->pilType->maxAmount) {
        pil_ptr->rechargeTimer.start();
    }
    pil_ptr->update();
}


/**
 * @brief When a pile starts idling.
 *
 * @param m The mob.
 * @param info1 Unused.
 * @param info2 Unused.
 */
void pile_fsm::becomeIdle(Mob* m, void* info1, void* info2) {
    Pile* pil_ptr = (Pile*) m;
    pil_ptr->update();
}
