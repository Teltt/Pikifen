/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Logic about mob movement, gravity, wall collision, etc.
 */

#include <algorithm>

#include "mob.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/general_utils.h"


using std::set;


/**
 * @brief Returns which walkable mob this mob should be considered to be on
 * top of.
 *
 * @return The mob to walk on, or nullptr if none is found.
 */
Mob* Mob::getMobToWalkOn() const {
    //Can't walk on anything if it's moving upwards.
    if(speedZ > 0.0f) return nullptr;
    
    Mob* best_candidate = nullptr;
    for(size_t m = 0; m < game.states.gameplay->mobs.walkables.size(); m++) {
        Mob* m_ptr = game.states.gameplay->mobs.walkables[m];
        if(m_ptr == this) {
            continue;
        }
        if(fabs(z - (m_ptr->z + m_ptr->height)) > GEOMETRY::STEP_HEIGHT) {
            continue;
        }
        if(best_candidate && m_ptr->z <= best_candidate->z) {
            continue;
        }
        
        //Check if they collide on X+Y.
        if(
            rectangularDim.x != 0 &&
            m_ptr->rectangularDim.x != 0
        ) {
            //Rectangle vs rectangle.
            if(
                !rectanglesIntersect(
                    pos, rectangularDim, angle,
                    m_ptr->pos, m_ptr->rectangularDim, m_ptr->angle
                )
            ) {
                continue;
            }
        } else if(rectangularDim.x != 0) {
            //Rectangle vs circle.
            if(
                !circleIntersectsRectangle(
                    m_ptr->pos, m_ptr->radius,
                    pos, rectangularDim,
                    angle
                )
            ) {
                continue;
            }
        } else if(m_ptr->rectangularDim.x != 0) {
            //Circle vs rectangle.
            if(
                !circleIntersectsRectangle(
                    pos, radius,
                    m_ptr->pos, m_ptr->rectangularDim,
                    m_ptr->angle
                )
            ) {
                continue;
            }
        } else {
            //Circle vs circle.
            if(
                Distance(pos, m_ptr->pos) >
                (radius + m_ptr->radius)
            ) {
                continue;
            }
        }
        best_candidate = m_ptr;
    }
    return best_candidate;
}


/**
 * @brief Calculates which edges the mob is intersecting with for
 * horizontal movement physics logic.
 *
 * @param new_pos Position to check.
 * @param intersecting_edges List of edges it is intersecting with.
 * @return H_MOVE_RESULT_OK if everything is okay, H_MOVE_RESULT_FAIL if movement is
 * impossible.
 */
H_MOVE_RESULT Mob::getMovementEdgeIntersections(
    const Point &new_pos, vector<Edge*>* intersecting_edges
) const {
    //Before checking the edges, let's consult the blockmap and look at
    //the edges in the same blocks the mob is on.
    //This way, we won't check for edges that are really far away.
    //Use the bounding box to know which blockmap blocks the mob will be on.
    set<Edge*> candidate_edges;
    //Use the terrain radius if the mob is moving about and alive.
    //Otherwise if it's a corpse, it can use the regular radius.
    float radius_to_use =
        (type->terrainRadius < 0 || health <= 0) ?
        radius :
        type->terrainRadius;
        
    if(
        !game.curAreaData->bmap.getEdgesInRegion(
            new_pos - radius_to_use,
            new_pos + radius_to_use,
            candidate_edges
        )
    ) {
        //Somehow out of bounds. No movement.
        return H_MOVE_RESULT_FAIL;
    }
    
    //Go through each edge, and figure out if it is a valid wall for our mob.
    for(auto &e_ptr : candidate_edges) {
    
        bool is_edge_blocking = false;
        
        if(
            !circleIntersectsLineSeg(
                new_pos, radius_to_use,
                v2p(e_ptr->vertexes[0]), v2p(e_ptr->vertexes[1]),
                nullptr, nullptr
            )
        ) {
            //No intersection? Well, obviously this one doesn't count.
            continue;
        }
        
        if(!e_ptr->sectors[0] || !e_ptr->sectors[1]) {
            //If we're on the edge of out-of-bounds geometry,
            //block entirely.
            return H_MOVE_RESULT_FAIL;
        }
        
        for(unsigned char s = 0; s < 2; s++) {
            if(e_ptr->sectors[s]->type == SECTOR_TYPE_BLOCKING) {
                is_edge_blocking = true;
                break;
            }
        }
        
        if(!is_edge_blocking) {
            if(e_ptr->sectors[0]->z == e_ptr->sectors[1]->z) {
                //No difference in floor height = no wall.
                //Ignore this.
                continue;
            }
            if(
                e_ptr->sectors[0]->z < z &&
                e_ptr->sectors[1]->z < z
            ) {
                //An edge whose sectors are below the mob?
                //No collision here.
                continue;
            }
        }
        
        if(
            e_ptr->sectors[0]->z > z &&
            e_ptr->sectors[1]->z > z
        ) {
            //If both floors of this edge are above the mob...
            //then what does that mean? That the mob is under the ground?
            //Nonsense! Throw this edge away!
            //It's a false positive, and it's likely behind a more logical
            //edge that we actually did collide against.
            continue;
        }
        
        if(
            e_ptr->sectors[0]->type == SECTOR_TYPE_BLOCKING &&
            e_ptr->sectors[1]->type == SECTOR_TYPE_BLOCKING
        ) {
            //Same logic as the previous check.
            continue;
        }
        
        //Add this edge to the list of intersections, then.
        intersecting_edges->push_back(e_ptr);
    }
    
    return H_MOVE_RESULT_OK;
}


/**
 * @brief Calculates how much the mob is going to move horizontally,
 * for the purposes of movement physics calculation.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 * @param move_speed_mult Movement speed is multiplied by this.
 * @param move_speed The calculated move speed is placed in this struct.
 * @return H_MOVE_RESULT_OK on normal movement, H_MOVE_RESULT_TELEPORTED if the mob's X
 * and Y have been set and movement logic can be skipped, and H_MOVE_RESULT_FAIL if
 * movement is entirely impossible this frame.
 */
H_MOVE_RESULT Mob::getPhysicsHorizontalMovement(
    float delta_t, float move_speed_mult, Point* move_speed
) {
    //Held by another mob.
    if(holder.m) {
        Point final_pos = holder.getFinalPos(&z);
        speedZ = 0;
        chase(final_pos, z, CHASE_FLAG_TELEPORT);
    }
    
    //Chasing.
    if(chaseInfo.state == CHASE_STATE_CHASING) {
        Point final_target_pos = getChaseTarget();
        
        if(hasFlag(chaseInfo.flags, CHASE_FLAG_TELEPORT)) {
        
            Sector* sec =
                getSector(final_target_pos, nullptr, true);
                
            if(!sec) {
                //No sector, invalid teleport. No move.
                return H_MOVE_RESULT_FAIL;
            }
            
            z = chaseInfo.offsetZ;
            if(chaseInfo.origZ) {
                z += *chaseInfo.origZ;
            }
            
            groundSector = sec;
            centerSector = sec;
            speed.x = speed.y = 0;
            pos = final_target_pos;
            
            if(!hasFlag(chaseInfo.flags, CHASE_FLAG_TELEPORTS_CONSTANTLY)) {
                chaseInfo.state = CHASE_STATE_FINISHED;
            }
            return H_MOVE_RESULT_TELEPORTED;
            
        } else {
        
            //Make it go to the direction it wants.
            float d = Distance(pos, final_target_pos).toFloat();
            
            chaseInfo.curSpeed +=
                chaseInfo.acceleration * delta_t;
            chaseInfo.curSpeed =
                std::min(chaseInfo.curSpeed, chaseInfo.maxSpeed);
                
            float move_amount =
                std::min(
                    (double) (d / delta_t),
                    (double) chaseInfo.curSpeed * move_speed_mult
                );
                
            bool can_free_move =
                hasFlag(chaseInfo.flags, CHASE_FLAG_ANY_ANGLE) ||
                d <= MOB::FREE_MOVE_THRESHOLD;
                
            float movement_angle =
                can_free_move ?
                getAngle(pos, final_target_pos) :
                angle;
                
            move_speed->x = cos(movement_angle) * move_amount;
            move_speed->y = sin(movement_angle) * move_amount;
        }
        
    } else {
        chaseInfo.acceleration = 0.0f;
        chaseInfo.curSpeed = 0.0f;
        chaseInfo.maxSpeed = 0.0f;
        
    }
    
    //If another mob is pushing it.
    if(pushAmount != 0.0f) {
        //Overly-aggressive pushing results in going through walls.
        //Let's place a cap.
        pushAmount =
            std::min(pushAmount, (float) (radius / delta_t) * 4);
            
        move_speed->x +=
            cos(pushAngle) * (pushAmount + MOB::PUSH_EXTRA_AMOUNT);
        move_speed->y +=
            sin(pushAngle) * (pushAmount + MOB::PUSH_EXTRA_AMOUNT);
    }
    
    //Scrolling floors.
    if(
        (groundSector->scroll.x != 0 || groundSector->scroll.y != 0) &&
        z <= groundSector->z
    ) {
        (*move_speed) += groundSector->scroll;
    }
    
    //On top of a mob.
    if(standingOnMob) {
        (*move_speed) += standingOnMob->walkableMoved;
    }
    
    return H_MOVE_RESULT_OK;
}


/**
 * @brief Calculates the angle at which the mob should slide against this wall,
 * for the purposes of movement physics calculations.
 *
 * @param e_ptr Pointer to the edge in question.
 * @param wall_sector Side index of the sector that actually makes a wall
 * (i.e. the highest).
 * @param move_angle Angle at which the mob is going to move.
 * @param slide_angle Holds the calculated slide angle.
 * @return H_MOVE_RESULT_OK on success, H_MOVE_RESULT_FAIL if the mob can't
 * slide against this wall.
 */
H_MOVE_RESULT Mob::getWallSlideAngle(
    const Edge* e_ptr, unsigned char wall_sector, float move_angle,
    float* slide_angle
) const {
    //The wall's normal is the direction the wall is facing.
    //i.e. the direction from the top floor to the bottom floor.
    //We know which side of an edge is which sector because of
    //the vertexes. Imagine you're in first person view,
    //following the edge as a line on the ground.
    //You start on vertex 0 and face vertex 1.
    //Sector 0 will always be on your left.
    
    float wall_normal;
    float wall_angle =
        getAngle(v2p(e_ptr->vertexes[0]), v2p(e_ptr->vertexes[1]));
        
    if(wall_sector == 0) {
        wall_normal = normalizeAngle(wall_angle + TAU / 4);
    } else {
        wall_normal = normalizeAngle(wall_angle - TAU / 4);
    }
    
    float nd = getAngleCwDiff(wall_normal, move_angle);
    if(nd < TAU * 0.25 || nd > TAU * 0.75) {
        //If the difference between the movement and the wall's
        //normal is this, that means we came FROM the wall.
        //No way! There has to be an edge that makes more sense.
        return H_MOVE_RESULT_FAIL;
    }
    
    //If we were to slide on this edge, this would be
    //the slide angle.
    if(nd < TAU / 2) {
        //Coming in from the "left" of the normal. Slide right.
        *slide_angle = wall_normal + TAU / 4;
    } else {
        //Coming in from the "right" of the normal. Slide left.
        *slide_angle = wall_normal - TAU / 4;
    }
    
    return H_MOVE_RESULT_OK;
}


/**
 * @brief Ticks physics logic regarding the mob's horizontal movement.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 * @param attempted_move_speed Movement speed to calculate with.
 * @param touched_wall Holds whether or not the mob touched a wall in this move.
 */
void Mob::tickHorizontalMovementPhysics(
    float delta_t, const Point &attempted_move_speed,
    bool* touched_wall
) {
    if(attempted_move_speed.x == 0 && attempted_move_speed.y == 0) {
        //No movement. Nothing to do here.
        return;
    }
    
    //Setup.
    bool finished_moving = false;
    bool doing_slide = false;
    
    Point new_pos = pos;
    Point move_speed = attempted_move_speed;
    
    //Try placing it in the place it should be at, judging
    //from the movement speed.
    while(!finished_moving) {
    
        //Start by checking sector collisions.
        //For this, we will only check if the mob is intersecting
        //with any edge. With this, we trust that mobs can't go so fast
        //that they're fully on one side of an edge in one frame,
        //and the other side on the next frame.
        //It's pretty naive...but it works!
        bool successful_move = true;
        
        new_pos.x = pos.x + delta_t* move_speed.x;
        new_pos.y = pos.y + delta_t* move_speed.y;
        float new_z = z;
        
        //Get the sector the mob will be on.
        Sector* new_center_sector = getSector(new_pos, nullptr, true);
        Sector* new_ground_sector = new_center_sector;
        Sector* step_sector = new_center_sector;
        
        if(!new_center_sector) {
            //Out of bounds. No movement.
            return;
        }
        if(z + GEOMETRY::STEP_HEIGHT < new_center_sector->z) {
            //We can't walk onto this sector. Refuse the move.
            return;
        }
        //Get all edges it collides against in this new position.
        vector<Edge*> intersecting_edges;
        if(
            getMovementEdgeIntersections(new_pos, &intersecting_edges) ==
            H_MOVE_RESULT_FAIL
        ) {
            return;
        }
        
        //For every sector in the new position, let's figure out
        //the ground sector, and also a stepping sector, if possible.
        for(size_t e = 0; e < intersecting_edges.size(); e++) {
            Edge* e_ptr = intersecting_edges[e];
            Sector* tallest_sector = groundSector; //Tallest of the two.
            if(
                e_ptr->sectors[0]->type != SECTOR_TYPE_BLOCKING &&
                e_ptr->sectors[1]->type != SECTOR_TYPE_BLOCKING
            ) {
                if(e_ptr->sectors[0]->z > e_ptr->sectors[1]->z) {
                    tallest_sector = e_ptr->sectors[0];
                } else {
                    tallest_sector = e_ptr->sectors[1];
                }
            }
            
            if(
                tallest_sector->z > new_ground_sector->z &&
                tallest_sector->z <= z
            ) {
                new_ground_sector = tallest_sector;
            }
            
            //Check if it can go up this step.
            //It can go up this step if the floor is within
            //stepping distance of the mob's current Z,
            //and if this step is larger than any step
            //encountered of all edges crossed.
            if(
                !hasFlag(flags, MOB_FLAG_WAS_THROWN) &&
                tallest_sector->z <= z + GEOMETRY::STEP_HEIGHT &&
                tallest_sector->z > step_sector->z
            ) {
                step_sector = tallest_sector;
            }
        }
        
        //Mosey on up to the step sector, if any.
        if(step_sector->z > new_ground_sector->z) {
            new_ground_sector = step_sector;
        }
        if(z < step_sector->z) new_z = step_sector->z;
        
        //Figure out sliding logic now, if needed.
        float move_angle;
        float total_move_speed;
        coordinatesToAngle(
            move_speed, &move_angle, &total_move_speed
        );
        
        //Angle to slide towards.
        float slide_angle = move_angle;
        //Difference between the movement angle and the slide.
        float slide_angle_dif = 0;
        
        //Check the sector heights of the intersecting edges to figure out
        //which are really walls, and how to slide against them.
        for(size_t e = 0; e < intersecting_edges.size(); e++) {
            Edge* e_ptr = intersecting_edges[e];
            bool is_edge_wall = false;
            unsigned char wall_sector = 0;
            
            for(unsigned char s = 0; s < 2; s++) {
                if(e_ptr->sectors[s]->type == SECTOR_TYPE_BLOCKING) {
                    is_edge_wall = true;
                    wall_sector = s;
                }
            }
            
            if(!is_edge_wall) {
                for(unsigned char s = 0; s < 2; s++) {
                    if(e_ptr->sectors[s]->z > new_z) {
                        is_edge_wall = true;
                        wall_sector = s;
                    }
                }
            }
            
            //This isn't a wall... Get out of here, faker.
            if(!is_edge_wall) continue;
            
            //Ok, there's obviously been a collision, so let's work out what
            //wall the mob will slide on.
            
            if(!doing_slide) {
                float tentative_slide_angle;
                if(
                    getWallSlideAngle(
                        e_ptr, wall_sector, move_angle, &tentative_slide_angle
                    ) == H_MOVE_RESULT_FAIL
                ) {
                    continue;
                }
                
                float sd =
                    getAngleSmallestDiff(move_angle, tentative_slide_angle);
                if(sd > slide_angle_dif) {
                    slide_angle_dif = sd;
                    slide_angle = tentative_slide_angle;
                }
            }
            
            //By the way, if we got to this point, that means there are real
            //collisions happening. Let's mark this move as unsuccessful.
            successful_move = false;
            (*touched_wall) = true;
        }
        
        //If the mob is just slamming against the wall head-on, perpendicularly,
        //then forget any idea about sliding.
        //It'd just be awkwardly walking in place.
        //Reset its horizontal position, but keep calculations for
        //everything else.
        if(!successful_move && slide_angle_dif > TAU / 4 - 0.05) {
            new_pos = pos;
            successful_move = true;
        }
        
        
        //We're done checking. If the move was unobstructed, good, go there.
        //If not, we'll use the info we gathered before to calculate sliding,
        //and try again.
        
        if(successful_move) {
            //Good news, the mob can be placed in this new spot freely.
            pos = new_pos;
            z = new_z;
            groundSector = new_ground_sector;
            centerSector = new_center_sector;
            finished_moving = true;
            
        } else {
        
            //Try sliding.
            if(doing_slide) {
                //We already tried sliding, and we still hit something...
                //Let's just stop completely. This mob can't go forward.
                finished_moving = true;
                
            } else {
                doing_slide = true;
                //To limit the speed, we should use a cross-product of the
                //movement and slide vectors.
                //But nuts to that, this is just as nice, and a lot simpler!
                total_move_speed *= 1 - (slide_angle_dif / TAU / 2);
                move_speed =
                    angleToCoordinates(slide_angle, total_move_speed);
                    
            }
        }
    }
}


/**
 * @brief Ticks the mob's actual physics procedures:
 * falling because of gravity, moving forward, etc.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void Mob::tickPhysics(float delta_t) {
    if(!groundSector) {
        //Object is placed out of bounds.
        return;
    }
    
    //Initial setup.
    float move_speed_mult = getSpeedMultiplier();
    
    Point pre_move_pos = pos;
    Point move_speed = speed;
    bool touched_wall = false;
    float pre_move_ground_z = groundSector->z;
    
    //Rotation logic.
    tickRotationPhysics(delta_t, move_speed_mult);
    
    //What type of horizontal movement is this?
    H_MOVE_RESULT h_mov_type =
        getPhysicsHorizontalMovement(delta_t, move_speed_mult, &move_speed);
        
    switch (h_mov_type) {
    case H_MOVE_RESULT_FAIL: {
        return;
    } case H_MOVE_RESULT_TELEPORTED: {
        break;
    } case H_MOVE_RESULT_OK: {
        //Horizontal movement time!
        tickHorizontalMovementPhysics(
            delta_t, move_speed, &touched_wall
        );
        break;
    }
    }
    
    //Vertical movement.
    tickVerticalMovementPhysics(
        delta_t, pre_move_ground_z, h_mov_type == H_MOVE_RESULT_TELEPORTED
    );
    
    //Walk on top of another mob, if possible.
    if(type->canWalkOnOthers) tickWalkableRidingPhysics(delta_t);
    
    //Final setup.
    pushAmount = 0;
    
    if(touched_wall) {
        fsm.runEvent(MOB_EV_TOUCHED_WALL);
    }
    
    if(type->walkable) {
        walkableMoved = (pos - pre_move_pos) / delta_t;
    }
}


/**
 * @brief Ticks physics logic regarding the mob rotating.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 * @param move_speed_mult Movement speed is multiplied by this.
 */
void Mob::tickRotationPhysics(
    float delta_t, float move_speed_mult
) {
    //Change the facing angle to the angle the mob wants to face.
    if(angle > TAU / 2)  angle -= TAU;
    if(angle < -TAU / 2) angle += TAU;
    if(intendedTurnPos) {
        intendedTurnAngle = getAngle(pos, *intendedTurnPos);
    }
    if(intendedTurnAngle > TAU / 2)  intendedTurnAngle -= TAU;
    if(intendedTurnAngle < -TAU / 2) intendedTurnAngle += TAU;
    
    float angle_dif = intendedTurnAngle - angle;
    if(angle_dif > TAU / 2)  angle_dif -= TAU;
    if(angle_dif < -TAU / 2) angle_dif += TAU;
    
    angle +=
        sign(angle_dif) * std::min(
            (double) (type->rotationSpeed * move_speed_mult * delta_t),
            (double) fabs(angle_dif)
        );
        
    if(holder.m) {
        switch(holder.rotationMethod) {
        case HOLD_ROTATION_METHOD_FACE_HOLDER: {
            float dummy;
            Point final_pos = holder.getFinalPos(&dummy);
            angle = getAngle(final_pos, holder.m->pos);
            stopTurning();
            break;
        } case HOLD_ROTATION_METHOD_COPY_HOLDER: {
            angle = holder.m->angle;
            stopTurning();
            break;
        } default: {
            break;
        }
        }
    }
    
    angleCos = cos(angle);
    angleSin = sin(angle);
}


/**
 * @brief Ticks physics logic regarding the mob's vertical movement.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 * @param pre_move_ground_z Z of the floor before horizontal movement started.
 * @param was_teleport Did the mob just teleport previously in the
 * movement logic?
 */
void Mob::tickVerticalMovementPhysics(
    float delta_t, float pre_move_ground_z,
    bool was_teleport
) {
    bool apply_gravity = true;
    
    if(!standingOnMob) {
        //If the current ground is one step (or less) below
        //the previous ground, just instantly go down the step.
        if(
            pre_move_ground_z - groundSector->z <= GEOMETRY::STEP_HEIGHT &&
            z == pre_move_ground_z
        ) {
            z = groundSector->z;
        }
    }
    
    //Vertical chasing.
    if(
        chaseInfo.state == CHASE_STATE_CHASING &&
        hasFlag(flags, MOB_FLAG_CAN_MOVE_MIDAIR) &&
        !hasFlag(chaseInfo.flags, CHASE_FLAG_TELEPORT)
    ) {
        apply_gravity = false;
        
        float target_z = chaseInfo.offsetZ;
        if(chaseInfo.origZ) target_z += *chaseInfo.origZ;
        float diff_z = fabs(target_z - z);
        
        speedZ =
            std::min((float) (diff_z / delta_t), chaseInfo.curSpeed);
        if(target_z < z) {
            speedZ = -speedZ;
        }
        
        z += speedZ * delta_t;
    }
    
    //Gravity.
    if(
        apply_gravity && !hasFlag(flags, MOB_FLAG_CAN_MOVE_MIDAIR) &&
        !holder.m && !was_teleport
    ) {
        //Use Velocity Verlet for better results.
        //https://youtu.be/hG9SzQxaCm8
        z +=
            (speedZ * delta_t) +
            ((MOB::GRAVITY_ADDER * gravityMult / 2.0f) * delta_t* delta_t);
        speedZ += MOB::GRAVITY_ADDER * delta_t* gravityMult;
    }
    
    //Landing.
    Hazard* new_on_hazard = nullptr;
    if(speedZ <= 0) {
        if(standingOnMob) {
            z = standingOnMob->z + standingOnMob->height;
            speedZ = 0;
            disableFlag(flags, MOB_FLAG_WAS_THROWN);
            fsm.runEvent(MOB_EV_LANDED);
            stopHeightEffect();
            highestMidairZ = FLT_MAX;
            
        } else if(z <= groundSector->z) {
            z = groundSector->z;
            speedZ = 0;
            disableFlag(flags, MOB_FLAG_WAS_THROWN);
            fsm.runEvent(MOB_EV_LANDED);
            stopHeightEffect();
            highestMidairZ = FLT_MAX;
            
            if(groundSector->isBottomlessPit) {
                fsm.runEvent(MOB_EV_BOTTOMLESS_PIT);
            }
            
            for(size_t h = 0; h < groundSector->hazards.size(); h++) {
                fsm.runEvent(
                    MOB_EV_TOUCHED_HAZARD,
                    (void*) groundSector->hazards[h]
                );
                new_on_hazard = groundSector->hazards[h];
            }
        }
    }
    
    if(z > groundSector->z) {
        if(highestMidairZ == FLT_MAX) highestMidairZ = z;
        else highestMidairZ = std::max(z, highestMidairZ);
    }
    
    //Held Pikmin are also touching the same hazards as the leader.
    if(holder.m == game.states.gameplay->curLeaderPtr) {
        Sector* leader_ground =
            game.states.gameplay->curLeaderPtr->groundSector;
        if(
            leader_ground &&
            game.states.gameplay->curLeaderPtr->z <= leader_ground->z
        ) {
            for(size_t h = 0; h < leader_ground->hazards.size(); h++) {
                fsm.runEvent(
                    MOB_EV_TOUCHED_HAZARD,
                    (void*) leader_ground->hazards[h]
                );
                new_on_hazard = leader_ground->hazards[h];
            }
        }
    }
    
    //Due to framerate imperfections, thrown Pikmin/leaders can reach higher
    //than intended. z_cap forces a cap. FLT_MAX = no cap.
    if(speedZ <= 0) {
        zCap = FLT_MAX;
    } else if(zCap < FLT_MAX) {
        z = std::min(z, zCap);
    }
    
    //On a sector that has a hazard that is not on the floor.
    if(z > groundSector->z && !groundSector->hazardFloor) {
        for(size_t h = 0; h < groundSector->hazards.size(); h++) {
            fsm.runEvent(
                MOB_EV_TOUCHED_HAZARD,
                (void*) groundSector->hazards[h]
            );
            new_on_hazard = groundSector->hazards[h];
        }
    }
    
    //Check if any hazards have been left.
    if(new_on_hazard != onHazard && onHazard != nullptr) {
        fsm.runEvent(
            MOB_EV_LEFT_HAZARD,
            (void*) onHazard
        );
        
        for(size_t s = 0; s < statuses.size(); s++) {
            if(statuses[s].type->removeOnHazardLeave) {
                statuses[s].toDelete = true;
            }
        }
        deleteOldStatusEffects();
    }
    onHazard = new_on_hazard;
    
    //Quick panic check: if it's somehow inside the ground, pop it out.
    z = std::max(z, groundSector->z);
}


/**
 * @brief Ticks physics logic regarding landing on top of a walkable mob.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void Mob::tickWalkableRidingPhysics(float delta_t) {
    Mob* rider_added_ev_mob = nullptr;
    Mob* rider_removed_ev_mob = nullptr;
    Mob* new_standing_on_mob = getMobToWalkOn();
    
    //Check which mob it is on top of, if any.
    if(new_standing_on_mob) {
        z = new_standing_on_mob->z + new_standing_on_mob->height;
    }
    
    if(new_standing_on_mob != standingOnMob) {
        if(standingOnMob) {
            rider_removed_ev_mob = standingOnMob;
        }
        if(new_standing_on_mob) {
            rider_added_ev_mob = new_standing_on_mob;
        }
    }
    
    standingOnMob = new_standing_on_mob;
    
    if(rider_removed_ev_mob) {
        rider_removed_ev_mob->fsm.runEvent(
            MOB_EV_RIDER_REMOVED, (void*) this
        );
        if(type->weight != 0.0f) {
            rider_removed_ev_mob->fsm.runEvent(
                MOB_EV_WEIGHT_REMOVED, (void*) this
            );
        }
    }
    if(rider_added_ev_mob) {
        rider_added_ev_mob->fsm.runEvent(
            MOB_EV_RIDER_ADDED, (void*) this
        );
        if(type->weight != 0.0f) {
            rider_added_ev_mob->fsm.runEvent(
                MOB_EV_WEIGHT_ADDED, (void*) this
            );
        }
    }
}
