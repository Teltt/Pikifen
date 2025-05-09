/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * General GUI manager and GUI item classes.
 * These are used during gameplay and menus, and are not related to Dear ImGui,
 * which is the GUI library used for the editors.
 */

#include <algorithm>

#include "gui.h"

#include "../../core/drawing.h"
#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/string_utils.h"


using DrawInfo = GuiItem::DrawInfo;


namespace GUI {

//Interval between auto-repeat activations, at the slowest speed.
const float AUTO_REPEAT_MAX_INTERVAL = 0.3f;

//Interval between auto-repeat activations, at the fastest speed.
const float AUTO_REPEAT_MIN_INTERVAL = 0.011f;

//How long it takes for the auto-repeat activations to reach max speed.
const float AUTO_REPEAT_RAMP_TIME = 0.9f;

//Padding before/after the circle in a bullet point item.
const float BULLET_PADDING = 6.0f;

//Radius of the circle that represents the bullet in a bullet point item.
const float BULLET_RADIUS = 4.0f;

//When an item does a juicy grow, this is the full effect duration.
const float JUICY_GROW_DURATION = 0.3f;

//When an item does a juicy elastic grow, this is the full effect duration.
const float JUICY_GROW_ELASTIC_DURATION = 0.4f;

//Grow scale multiplier for a juicy icon grow animation.
const float JUICY_GROW_ICON_MULT = 5.0f;

//Grow scale multiplier for a juicy text high grow animation.
const float JUICY_GROW_TEXT_HIGH_MULT = 0.15f;

//Grow scale multiplier for a juicy text low grow animation.
const float JUICY_GROW_TEXT_LOW_MULT = 0.02f;

//Grow scale multiplier for a juicy text medium grow animation.
const float JUICY_GROW_TEXT_MEDIUM_MULT = 0.05f;

//Standard size of the content inside of a GUI item, in ratio.
const Point STANDARD_CONTENT_SIZE = Point(0.95f, 0.80f);

}


/**
 * @brief Constructs a new bullet point gui item object.
 *
 * @param text Text to display on the bullet point.
 * @param font Font for the button's text.
 * @param color Color of the button's text.
 */
BulletGuiItem::BulletGuiItem(
    const string &text, ALLEGRO_FONT* font, const ALLEGRO_COLOR &color
) :
    GuiItem(true),
    text(text),
    font(font),
    color(color) {
    
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
}


/**
 * @brief Default bullet GUI item draw code.
 */
void BulletGuiItem::defDrawCode(
    const DrawInfo &draw
) {
    float item_x_start = draw.center.x - draw.size.x * 0.5;
    float text_x_offset =
        GUI::BULLET_RADIUS * 2 +
        GUI::BULLET_PADDING * 2;
    Point text_space(
        std::max(1.0f, draw.size.x - text_x_offset),
        draw.size.y
    );
    
    drawBitmap(
        game.sysContent.bmpHardBubble,
        Point(
            item_x_start + GUI::BULLET_RADIUS + GUI::BULLET_PADDING,
            draw.center.y
        ),
        Point(GUI::BULLET_RADIUS * 2),
        0.0f, this->color
    );
    float juicy_grow_amount = getJuiceValue();
    drawText(
        this->text, this->font,
        Point(item_x_start + text_x_offset, draw.center.y),
        text_space * GUI::STANDARD_CONTENT_SIZE,
        this->color, ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_CENTER,
        TEXT_SETTING_FLAG_CANT_GROW,
        Point(1.0 + juicy_grow_amount)
    );
    if(selected) {
        drawTexturedBox(
            draw.center,
            draw.size + 10.0 + sin(game.timePassed * TAU) * 2.0f,
            game.sysContent.bmpFocusBox
        );
    }
}


/**
 * @brief Constructs a new button gui item object.
 *
 * @param text Text to display on the button.
 * @param font Font for the button's text.
 * @param color Color of the button's text.
 */
ButtonGuiItem::ButtonGuiItem(
    const string &text, ALLEGRO_FONT* font, const ALLEGRO_COLOR &color
) :
    GuiItem(true),
    text(text),
    font(font),
    color(color) {
    
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
}


/**
 * @brief Default button GUI item draw code.
 */
void ButtonGuiItem::defDrawCode(
    const DrawInfo  &draw
) {
    drawButton(
        draw.center, draw.size,
        this->text, this->font, this->color, selected,
        getJuiceValue()
    );
}


/**
 * @brief Constructs a new check gui item object.
 *
 * @param value Current value.
 * @param text Text to display on the checkbox.
 * @param font Font for the checkbox's text.
 * @param color Color of the checkbox's text.
 */
CheckGuiItem::CheckGuiItem(
    bool value, const string &text, ALLEGRO_FONT* font,
    const ALLEGRO_COLOR &color
) :
    GuiItem(true),
    value(value),
    text(text),
    font(font),
    color(color) {
    
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
    
    onActivate =
    [this] (const Point &) {
        this->defActivateCode();
    };
}


/**
 * @brief Constructs a new check gui item object.
 *
 * @param value_ptr Pointer to the boolean that stores the current value.
 * @param text Text to display on the checkbox.
 * @param font Font for the checkbox's text.
 * @param color Color of the checkbox's text.
 */
CheckGuiItem::CheckGuiItem(
    bool* value_ptr, const string &text, ALLEGRO_FONT* font,
    const ALLEGRO_COLOR &color
) :
    CheckGuiItem(*value_ptr, text, font, color) {
    
    this->valuePtr = value_ptr;
}


/**
 * @brief Default check GUI item activation code.
 */
void CheckGuiItem::defActivateCode() {
    value = !value;
    if(valuePtr) (*valuePtr) = !(*valuePtr);
    startJuiceAnimation(JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM);
}


/**
 * @brief Default check GUI item draw code.
 */
void CheckGuiItem::defDrawCode(const DrawInfo &draw) {
    float juicy_grow_amount = getJuiceValue();
    drawText(
        this->text, this->font,
        Point(draw.center.x - draw.size.x * 0.45, draw.center.y),
        Point(draw.size.x * 0.95, draw.size.y) * GUI::STANDARD_CONTENT_SIZE,
        this->color, ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_CENTER,
        TEXT_SETTING_FLAG_CANT_GROW,
        Point(1.0f + juicy_grow_amount)
    );
    
    drawBitmap(
        this->value ?
        game.sysContent.bmpCheckboxCheck :
        game.sysContent.bmpCheckboxNoCheck,
        this->text.empty() ?
        draw.center :
        Point((draw.center.x + draw.size.x * 0.5) - 40, draw.center.y),
        Point(32, -1)
    );
    
    ALLEGRO_COLOR box_tint =
        selected ? al_map_rgb(87, 200, 208) : COLOR_WHITE;
        
    drawTexturedBox(
        draw.center, draw.size, game.sysContent.bmpBubbleBox, box_tint
    );
    
    if(selected) {
        drawTexturedBox(
            draw.center,
            draw.size + 10.0 + sin(game.timePassed * TAU) * 2.0f,
            game.sysContent.bmpFocusBox
        );
    }
}


/**
 * @brief Constructs a new GUI item object.
 *
 * @param selectable Can the item be selected by the player?
 */
GuiItem::GuiItem(bool selectable) :
    selectable(selectable) {
    
}


/**
 * @brief Activates the item.
 *
 * @param cursor_pos Cursor coordinates, if applicable.
 * @return Whether it could activate it.
 */
bool GuiItem::activate(const Point &cursor_pos) {
    if(!onActivate) return false;
    onActivate(cursor_pos);
    
    ALLEGRO_SAMPLE* sample =
        this == manager->backItem ?
        game.sysContent.sndMenuBack :
        game.sysContent.sndMenuActivate;
    SoundSourceConfig activate_sound_config;
    activate_sound_config.gain = 0.75f;
    game.audio.createUiSoundsource(sample, activate_sound_config);
    
    return true;
}


/**
 * @brief Adds a child item.
 *
 * @param item Item to add as a child item.
 */
void GuiItem::addChild(GuiItem* item) {
    children.push_back(item);
    item->parent = this;
}


/**
 * @brief Removes and deletes all children items.
 */
void GuiItem::deleteAllChildren() {
    while(!children.empty()) {
        GuiItem* i_ptr = children[0];
        removeChild(i_ptr);
        manager->removeItem(i_ptr);
        delete i_ptr;
    }
}


/**
 * @brief Returns the bottommost Y coordinate, in height ratio,
 * of the item's children items.
 *
 * @return The Y coordinate.
 */
float GuiItem::getChildBottom() {
    float bottommost = 0.0f;
    for(size_t c = 0; c < children.size(); c++) {
        GuiItem* c_ptr = children[c];
        bottommost =
            std::max(
                bottommost,
                c_ptr->ratioCenter.y + (c_ptr->ratioSize.y / 2.0f)
            );
    }
    return bottommost;
}


/**
 * @brief Returns the value related to the current juice animation.
 *
 * @return The juice value, or 0 if there's no animation.
 */
float GuiItem::getJuiceValue() {
    switch(juiceType) {
    case JUICE_TYPE_GROW_TEXT_LOW: {
        float anim_ratio =
            1.0f - (juiceTimer / GUI::JUICY_GROW_DURATION);
        return
            ease(EASE_METHOD_UP_AND_DOWN, anim_ratio) *
            GUI::JUICY_GROW_TEXT_LOW_MULT;
    }
    case JUICE_TYPE_GROW_TEXT_MEDIUM: {
        float anim_ratio =
            1.0f - (juiceTimer / GUI::JUICY_GROW_DURATION);
        return
            ease(EASE_METHOD_UP_AND_DOWN, anim_ratio) *
            GUI::JUICY_GROW_TEXT_MEDIUM_MULT;
    }
    case JUICE_TYPE_GROW_TEXT_HIGH: {
        float anim_ratio =
            1.0f - (juiceTimer / GUI::JUICY_GROW_DURATION);
        return
            ease(EASE_METHOD_UP_AND_DOWN, anim_ratio) *
            GUI::JUICY_GROW_TEXT_HIGH_MULT;
    }
    case JUICE_TYPE_GROW_TEXT_ELASTIC_LOW: {
        float anim_ratio =
            1.0f - (juiceTimer / GUI::JUICY_GROW_ELASTIC_DURATION);
        return
            ease(EASE_METHOD_UP_AND_DOWN_ELASTIC, anim_ratio) *
            GUI::JUICY_GROW_TEXT_LOW_MULT;
    }
    case JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM: {
        float anim_ratio =
            1.0f - (juiceTimer / GUI::JUICY_GROW_ELASTIC_DURATION);
        return
            ease(EASE_METHOD_UP_AND_DOWN_ELASTIC, anim_ratio) *
            GUI::JUICY_GROW_TEXT_MEDIUM_MULT;
    }
    case JUICE_TYPE_GROW_TEXT_ELASTIC_HIGH: {
        float anim_ratio =
            1.0f - (juiceTimer / GUI::JUICY_GROW_ELASTIC_DURATION);
        return
            ease(EASE_METHOD_UP_AND_DOWN_ELASTIC, anim_ratio) *
            GUI::JUICY_GROW_TEXT_HIGH_MULT;
    }
    case JUICE_TYPE_GROW_ICON: {
        float anim_ratio =
            1.0f - (juiceTimer / GUI::JUICY_GROW_DURATION);
        return
            ease(EASE_METHOD_UP_AND_DOWN, anim_ratio) *
            GUI::JUICY_GROW_ICON_MULT;
    }
    default: {
        return 0.0f;
    }
    }
}


/**
 * @brief Returns the reference center coordinates,
 * i.e. used when not animating.
 *
 * @return The center.
 */
Point GuiItem::getReferenceCenter() {
    if(parent) {
        Point parent_s =
            parent->getReferenceSize() - (parent->padding * 2.0f);
        Point parent_c =
            parent->getReferenceCenter();
        Point result = ratioCenter * parent_s;
        result.x += parent_c.x - parent_s.x / 2.0f;
        result.y += parent_c.y - parent_s.y / 2.0f;
        result.y -= parent_s.y * parent->offset;
        return result;
    } else {
        return Point(ratioCenter.x * game.winW, ratioCenter.y * game.winH);
    }
}


/**
 * @brief Returns the reference width and height, i.e. used when not animating.
 *
 * @return The size.
 */
Point GuiItem::getReferenceSize() {
    Point mult;
    if(parent) {
        mult = parent->getReferenceSize() - (parent->padding * 2.0f);
    } else {
        mult.x = game.winW;
        mult.y = game.winH;
    }
    return ratioSize * mult;
}


/**
 * @brief Returns whether the mouse cursor is on top of it.
 *
 * @param cursor_pos Position of the mouse cursor, in screen coordinates.
 * @return Whether the cursor is on top.
 */
bool GuiItem::isMouseOn(const Point &cursor_pos) {
    if(parent && !parent->isMouseOn(cursor_pos)) {
        return false;
    }
    
    Point c = getReferenceCenter();
    Point s = getReferenceSize();
    return
        (
            cursor_pos.x >= c.x - s.x * 0.5 &&
            cursor_pos.x <= c.x + s.x * 0.5 &&
            cursor_pos.y >= c.y - s.y * 0.5 &&
            cursor_pos.y <= c.y + s.y * 0.5
        );
}


/**
 * @brief Returns whether or not it is responsive, and also checks the parents.
 *
 * @return Whether it is responsive.
 */
bool GuiItem::isResponsive() {
    if(parent) return parent->isResponsive();
    return responsive;
}


/**
 * @brief Returns whether or not it is visible, and also checks the parents.
 *
 * @return Whether it is visible.
 */
bool GuiItem::isVisible() {
    if(parent) return parent->isVisible();
    return visible;
}


/**
 * @brief Removes an item from the list of children, without deleting it.
 *
 * @param item Child item to remove.
 */
void GuiItem::removeChild(GuiItem* item) {
    for(size_t c = 0; c < children.size(); c++) {
        if(children[c] == item) {
            children.erase(children.begin() + c);
        }
    }
    
    item->parent = nullptr;
}


/**
 * @brief Starts some juice animation.
 *
 * @param type Type of juice animation.
 */
void GuiItem::startJuiceAnimation(JUICE_TYPE type) {
    juiceType = type;
    switch(type) {
    case JUICE_TYPE_GROW_TEXT_LOW:
    case JUICE_TYPE_GROW_TEXT_MEDIUM:
    case JUICE_TYPE_GROW_TEXT_HIGH:
    case JUICE_TYPE_GROW_ICON: {
        juiceTimer = GUI::JUICY_GROW_DURATION;
        break;
    }
    case JUICE_TYPE_GROW_TEXT_ELASTIC_LOW:
    case JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM:
    case JUICE_TYPE_GROW_TEXT_ELASTIC_HIGH: {
        juiceTimer = GUI::JUICY_GROW_ELASTIC_DURATION;
        break;
    }
    default: {
        break;
    }
    }
}


/**
 * @brief Constructs a new gui manager object.
 */
GuiManager::GuiManager() :
    autoRepeater(&autoRepeaterSettings) {
    
    animTimer =
        Timer(
    0.0f, [this] () {
        switch(animType) {
        case GUI_MANAGER_ANIM_IN_TO_OUT:
        case GUI_MANAGER_ANIM_CENTER_TO_UP:
        case GUI_MANAGER_ANIM_CENTER_TO_DOWN:
        case GUI_MANAGER_ANIM_CENTER_TO_LEFT:
        case GUI_MANAGER_ANIM_CENTER_TO_RIGHT: {
            visible = false;
            break;
        }
        default: {
            visible = true;
            break;
        }
        }
    }
        );
}


/**
 * @brief Add an item to the list.
 *
 * @param item Pointer to the new item.
 * @param id If this item has an associated ID, specify it here.
 * Empty string if none.
 */
void GuiManager::addItem(GuiItem* item, const string &id) {
    auto c = registeredCenters.find(id);
    if(c != registeredCenters.end()) {
        item->ratioCenter = c->second;
    }
    auto s = registeredSizes.find(id);
    if(s != registeredSizes.end()) {
        item->ratioSize = s->second;
    }
    
    items.push_back(item);
    item->manager = this;
}


/**
 * @brief Destroys and deletes all items and information.
 */
void GuiManager::destroy() {
    setSelectedItem(nullptr);
    backItem = nullptr;
    for(size_t i = 0; i < items.size(); i++) {
        delete items[i];
    }
    items.clear();
    registeredCenters.clear();
    registeredSizes.clear();
}


/**
 * @brief Draws all items on-screen.
 */
void GuiManager::draw() {
    if(!visible) return;
    
    int ocr_x = 0;
    int ocr_y = 0;
    int ocr_w = 0;
    int ocr_h = 0;
    
    for(size_t i = 0; i < items.size(); i++) {
    
        GuiItem* i_ptr = items[i];
        
        if(!i_ptr->onDraw) continue;
        
        DrawInfo draw;
        draw.center = i_ptr->getReferenceCenter();
        draw.size = i_ptr->getReferenceSize();
        
        if(!getItemDrawInfo(i_ptr, &draw)) continue;
        
        if(i_ptr->parent) {
            DrawInfo parent_draw;
            if(!getItemDrawInfo(i_ptr->parent, &parent_draw)) {
                continue;
            }
            al_get_clipping_rectangle(&ocr_x, &ocr_y, &ocr_w, &ocr_h);
            al_set_clipping_rectangle(
                (parent_draw.center.x - parent_draw.size.x / 2.0f) + 1,
                (parent_draw.center.y - parent_draw.size.y / 2.0f) + 1,
                parent_draw.size.x - 2,
                parent_draw.size.y - 2
            );
        }
        
        i_ptr->onDraw(draw);
        
        if(i_ptr->parent) {
            al_set_clipping_rectangle(ocr_x, ocr_y, ocr_w, ocr_h);
        }
    }
}


/**
 * @brief Returns the currently selected item's tooltip, if any.
 *
 * @return The tooltip.
 */
string GuiManager::getCurrentTooltip() {
    if(!selectedItem) return string();
    if(!selectedItem->onGetTooltip) return string();
    return selectedItem->onGetTooltip();
}


/**
 * @brief Returns a given item's drawing information.
 *
 * @param item What item to check.
 * @param draw_center The drawing center coordinates to use.
 * @param draw_size The drawing width and height to use.
 * @return True if the item exists and is meant to be drawn, false otherwise.
 */
bool GuiManager::getItemDrawInfo(
    GuiItem* item, DrawInfo* draw
) {
    if(!item->isVisible()) return false;
    if(item->ratioSize.x == 0.0f) return false;
    
    Point final_center = item->getReferenceCenter();
    Point final_size = item->getReferenceSize();
    
    if(animTimer.timeLeft > 0.0f) {
        switch(animType) {
        case GUI_MANAGER_ANIM_OUT_TO_IN: {
            Point start_center;
            float angle =
                getAngle(
                    Point(game.winW, game.winH) / 2.0f,
                    final_center
                );
            start_center.x = final_center.x + cos(angle) * game.winW;
            start_center.y = final_center.y + sin(angle) * game.winH;
            
            final_center.x =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, start_center.x, final_center.x
                );
            final_center.y =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, start_center.y, final_center.y
                );
            break;
            
        } case GUI_MANAGER_ANIM_IN_TO_OUT: {
            Point end_center;
            float angle =
                getAngle(
                    Point(game.winW, game.winH) / 2.0f,
                    final_center
                );
            end_center.x = final_center.x + cos(angle) * game.winW;
            end_center.y = final_center.y + sin(angle) * game.winH;
            
            final_center.x =
                interpolateNumber(
                    ease(EASE_METHOD_IN, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.x, end_center.x
                );
            final_center.y =
                interpolateNumber(
                    ease(EASE_METHOD_IN, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.y, end_center.y
                );
            break;
            
        } case GUI_MANAGER_ANIM_UP_TO_CENTER: {
            final_center.y =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.y - game.winH, final_center.y
                );
            break;
            
        } case GUI_MANAGER_ANIM_CENTER_TO_UP: {
            final_center.y =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.y, final_center.y - game.winH
                );
            break;
            
        } case GUI_MANAGER_ANIM_DOWN_TO_CENTER: {
            final_center.y =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.y + game.winH, final_center.y
                );
            break;
            
        } case GUI_MANAGER_ANIM_CENTER_TO_DOWN: {
            final_center.y =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.y, final_center.y + game.winH
                );
            break;
            
        } case GUI_MANAGER_ANIM_LEFT_TO_CENTER: {
            final_center.x =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.x - game.winW, final_center.x
                );
            break;
            
        } case GUI_MANAGER_ANIM_CENTER_TO_LEFT: {
            final_center.x =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.x, final_center.x - game.winW
                );
            break;
            
        } case GUI_MANAGER_ANIM_RIGHT_TO_CENTER: {
            final_center.x =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.x + game.winW, final_center.x
                );
            break;
            
        } case GUI_MANAGER_ANIM_CENTER_TO_RIGHT: {
            final_center.x =
                interpolateNumber(
                    ease(EASE_METHOD_OUT, 1.0f - animTimer.getRatioLeft()),
                    0.0f, 1.0f, final_center.x, final_center.x + game.winW
                );
            break;
            
        } default: {
            break;
            
        }
        }
    }
    
    draw->center = final_center;
    draw->size = final_size;
    return true;
}


/**
 * @brief Handle an Allegro event.
 * Controls are handled in handlePlayerAction.
 *
 * @param ev Event.
 */
void GuiManager::handleAllegroEvent(const ALLEGRO_EVENT &ev) {
    if(!responsive) return;
    if(animTimer.getRatioLeft() > 0.0f && ignoreInputOnAnimation) return;
    
    bool mouse_moved = false;
    
    //Mousing over an item and clicking.
    if(
        ev.type == ALLEGRO_EVENT_MOUSE_AXES ||
        ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN
    ) {
        GuiItem* selection_result = nullptr;
        for(size_t i = 0; i < items.size(); i++) {
            GuiItem* i_ptr = items[i];
            if(
                i_ptr->isMouseOn(Point(ev.mouse.x, ev.mouse.y)) &&
                i_ptr->isResponsive() &&
                i_ptr->selectable
            ) {
                selection_result = i_ptr;
                if(i_ptr->onMouseOver) {
                    i_ptr->onMouseOver(Point(ev.mouse.x, ev.mouse.y));
                }
                break;
            }
        }
        setSelectedItem(selection_result);
        mouse_moved = true;
    }
    
    if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && ev.mouse.button == 1) {
        if(
            selectedItem &&
            selectedItem->isResponsive() &&
            selectedItem->onActivate
        ) {
            selectedItem->activate(Point(ev.mouse.x, ev.mouse.y));
            autoRepeater.start();
        }
        mouse_moved = true;
    }
    
    if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP && ev.mouse.button == 1) {
        autoRepeater.stop();
        mouse_moved = true;
    }
    
    for(size_t i = 0; i < items.size(); i++) {
        if(items[i]->isResponsive() && items[i]->onAllegroEvent) {
            items[i]->onAllegroEvent(ev);
        }
    }
    
    if(mouse_moved) lastInputWasMouse = true;
}


/**
 * @brief Handles a player input.
 *
 * @param action Data about the player action.
 * @return Whether the input was used.
 */
bool GuiManager::handlePlayerAction(const PlayerAction &action) {
    if(!responsive) {
        return false;
    }
    if(
        animTimer.getRatioLeft() > 0.0f &&
        ignoreInputOnAnimation
    ) {
        return false;
    }
    
    bool is_down = (action.value >= 0.5);
    bool button_recognized = true;
    
    switch(action.actionTypeId) {
    case PLAYER_ACTION_TYPE_MENU_RIGHT:
    case PLAYER_ACTION_TYPE_MENU_UP:
    case PLAYER_ACTION_TYPE_MENU_LEFT:
    case PLAYER_ACTION_TYPE_MENU_DOWN: {

        //Selecting a different item with the arrow keys.
        size_t pressed = PLAYER_ACTION_TYPE_NONE;
        
        switch(action.actionTypeId) {
        case PLAYER_ACTION_TYPE_MENU_RIGHT: {
            if(!rightPressed && is_down) {
                pressed = PLAYER_ACTION_TYPE_MENU_RIGHT;
            }
            rightPressed = is_down;
            break;
        } case PLAYER_ACTION_TYPE_MENU_UP: {
            if(!upPressed && is_down) {
                pressed = PLAYER_ACTION_TYPE_MENU_UP;
            }
            upPressed = is_down;
            break;
        } case PLAYER_ACTION_TYPE_MENU_LEFT: {
            if(!leftPressed && is_down) {
                pressed = PLAYER_ACTION_TYPE_MENU_LEFT;
            }
            leftPressed = is_down;
            break;
        } case PLAYER_ACTION_TYPE_MENU_DOWN: {
            if(!downPressed && is_down) {
                pressed = PLAYER_ACTION_TYPE_MENU_DOWN;
            }
            downPressed = is_down;
            break;
        } default: {
            break;
        }
        }
        
        if(pressed == PLAYER_ACTION_TYPE_NONE) break;
        
        if(!selectedItem) {
            for(size_t i = 0; i < items.size(); i++) {
                if(items[i]->isResponsive() && items[i]->selectable) {
                    setSelectedItem(items[i]);
                    break;
                }
            }
            if(selectedItem) {
                break;
            }
        }
        if(!selectedItem) {
            //No item can be selected.
            break;
        }
        
        vector<Point> selectables;
        vector<GuiItem*> selectable_ptrs;
        size_t selectable_idx = INVALID;
        float direction = 0.0f;
        
        switch(pressed) {
        case PLAYER_ACTION_TYPE_MENU_DOWN: {
            direction = TAU * 0.25f;
            break;
        }
        case PLAYER_ACTION_TYPE_MENU_LEFT: {
            direction = TAU * 0.50f;
            break;
        }
        case PLAYER_ACTION_TYPE_MENU_UP: {
            direction = TAU * 0.75f;
            break;
        }
        }
        
        if(
            selectedItem &&
            selectedItem->isResponsive() &&
            selectedItem->onMenuDirButton
        ) {
            if(selectedItem->onMenuDirButton(pressed)) {
                //If it returned true, that means the following logic about
                //changing the current item needs to be skipped.
                break;
            }
        }
        
        float min_y = 0;
        float max_y = game.winH;
        
        for(size_t i = 0; i < items.size(); i++) {
            GuiItem* i_ptr = items[i];
            if(i_ptr->isResponsive() && i_ptr->selectable) {
                Point i_center = i_ptr->getReferenceCenter();
                if(i_ptr == selectedItem) {
                    selectable_idx = selectables.size();
                }
                
                min_y = std::min(min_y, i_center.y);
                max_y = std::max(max_y, i_center.y);
                
                selectable_ptrs.push_back(i_ptr);
                selectables.push_back(i_ptr->getReferenceCenter());
            }
        }
        
        size_t new_selectable_idx =
            selectNextItemDirectionally(
                selectables,
                selectable_idx,
                direction,
                Point(game.winW, max_y - min_y)
            );
            
        if(new_selectable_idx != selectable_idx) {
            setSelectedItem(selectable_ptrs[new_selectable_idx]);
            if(
                selectedItem->parent &&
                selectedItem->parent->onChildSelected
            ) {
                selectedItem->parent->onChildSelected(
                    selectedItem
                );
            }
        }
        
        break;
        
    } case PLAYER_ACTION_TYPE_MENU_OK: {
        if(
            is_down &&
            selectedItem &&
            selectedItem->onActivate &&
            selectedItem->isResponsive()
        ) {
            selectedItem->activate(Point(LARGE_FLOAT));
            autoRepeater.start();
        } else if(!is_down) {
            autoRepeater.stop();
        }
        break;
        
    } case PLAYER_ACTION_TYPE_MENU_BACK: {
        if(is_down && backItem && backItem->isResponsive()) {
            backItem->activate(Point(LARGE_FLOAT));
        }
        break;
        
    } default: {
        button_recognized = false;
        break;
        
    }
    }
    
    if(button_recognized) {
        lastInputWasMouse = false;
    }
    return button_recognized;
}


/**
 * @brief Hides all items until an animation shows them again.
 */
void GuiManager::hideItems() {
    visible = false;
}


/**
 * @brief Reads item default centers and sizes from a data node.
 *
 * @param node Data node to read from.
 */
void GuiManager::readCoords(DataNode* node) {
    size_t n_items = node->getNrOfChildren();
    for(size_t i = 0; i < n_items; i++) {
        DataNode* item_node = node->getChild(i);
        vector<string> words = split(item_node->value);
        if(words.size() < 4) {
            continue;
        }
        registerCoords(
            item_node->name,
            s2f(words[0]), s2f(words[1]), s2f(words[2]), s2f(words[3])
        );
    }
}


/**
 * @brief Registers an item's default center and size.
 *
 * @param id String ID of the item.
 * @param cx Center X, in screen percentage.
 * @param cy Center Y, in screen percentage.
 * @param w Width, in screen percentage.
 * @param h Height, in screen percentage.
 */
void GuiManager::registerCoords(
    const string &id,
    float cx, float cy, float w, float h
) {
    registeredCenters[id] =
        Point(cx / 100.0f, cy / 100.0f);
    registeredSizes[id] =
        Point(w / 100.0f, h / 100.0f);
}


/**
 * @brief Removes an item from the list.
 *
 * @param item Item to remove.
 */
void GuiManager::removeItem(GuiItem* item) {
    if(selectedItem == item) {
        setSelectedItem(nullptr);
    }
    if(backItem == item) {
        backItem = nullptr;
    }
    
    for(size_t i = 0; i < items.size(); i++) {
        if(items[i] == item) {
            items.erase(items.begin() + i);
        }
    }
    item->manager = nullptr;
}


/**
 * @brief Sets the given item as the one that is selected, or none.
 *
 * @param item Item to select, or nullptr for none.
 * @param silent If true, no sound effect will play.
 * Useful if you want the item to be selected not because of user input,
 * but because it's the default selected item when the GUI loads.
 */
void GuiManager::setSelectedItem(GuiItem* item, bool silent) {
    if(selectedItem == item) {
        return;
    }
    
    autoRepeater.stop();
    
    if(selectedItem) {
        selectedItem->selected = false;
    }
    selectedItem = item;
    if(selectedItem) {
        selectedItem->selected = true;
    }
    
    if(onSelectionChanged) onSelectionChanged();
    if(selectedItem) {
        if(selectedItem->onSelected) {
            selectedItem->onSelected();
        }
    }
    
    if(selectedItem && !silent) {
        SoundSourceConfig select_sound_config;
        select_sound_config.gain = 0.5f;
        select_sound_config.speedDeviation = 0.1f;
        select_sound_config.stackMinPos = 0.01f;
        game.audio.createUiSoundsource(
            game.sysContent.sndMenuSelect,
            select_sound_config
        );
    }
}


/**
 * @brief Shows all items, if they were hidden.
 */
void GuiManager::showItems() {
    visible = true;
}


/**
 * @brief Starts an animation that affects all items.
 *
 * @param type Type of aniimation to start.
 * @param duration Total duration of the animation.
 */
void GuiManager::startAnimation(
    const GUI_MANAGER_ANIM type, float duration
) {
    animType = type;
    animTimer.start(duration);
    visible = true;
}


/**
 * @brief Ticks the time of all items by one frame of logic.
 *
 * @param delta_t How long the frame's tick is, in seconds.
 */
void GuiManager::tick(float delta_t) {
    //Tick the animation.
    animTimer.tick(delta_t);
    
    //Tick all items.
    for(size_t i = 0; i < items.size(); i++) {
        GuiItem* i_ptr = items[i];
        if(i_ptr->onTick) {
            i_ptr->onTick(delta_t);
        }
        if(i_ptr->juiceTimer > 0) {
            i_ptr->juiceTimer =
                std::max(0.0f, i_ptr->juiceTimer - delta_t);
        } else {
            i_ptr->juiceType = GuiItem::JUICE_TYPE_NONE;
        }
    }
    
    //Auto-repeat activations of the selected item, if applicable.
    size_t auto_repeat_triggers = autoRepeater.tick(delta_t);
    if(
        selectedItem &&
        selectedItem->canAutoRepeat && selectedItem->onActivate
    ) {
        for(size_t r = 0; r < auto_repeat_triggers; r++) {
            selectedItem->activate(Point(LARGE_FLOAT));
        }
    }
}


/**
 * @brief Returns whether the last input was a mouse input.
 *
 * @return Whether it was a mouse input.
 */
bool GuiManager::wasLastInputMouse() {
    return lastInputWasMouse;
}


/**
 * @brief Constructs a new list gui item object.
 */
ListGuiItem::ListGuiItem() :
    GuiItem() {
    
    padding = 8.0f;
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
    onTick =
    [this] (float delta_t) {
        this->defTickCode(delta_t);
    };
    onAllegroEvent =
    [this] (const ALLEGRO_EVENT & ev) {
        this->defEventCode(ev);
    };
    onChildSelected =
    [this] (const GuiItem * child) {
        this->defChildSelectedCode(child);
    };
}


/**
 * @brief Default list GUI item child selected code.
 */
void ListGuiItem::defChildSelectedCode(const GuiItem* child) {
    //Try to center the child.
    float child_bottom = getChildBottom();
    if(child_bottom <= 1.0f && offset == 0.0f) {
        return;
    }
    targetOffset =
        std::clamp(
            child->ratioCenter.y - 0.5f,
            0.0f,
            child_bottom - 1.0f
        );
}


/**
 * @brief Default list GUI item draw code.
 */
void ListGuiItem::defDrawCode(const DrawInfo &draw) {
    drawTexturedBox(
        draw.center, draw.size, game.sysContent.bmpFrameBox,
        COLOR_TRANSPARENT_WHITE
    );
    if(offset > 0.0f) {
        //Shade effect at the top.
        ALLEGRO_VERTEX vertexes[8];
        for(size_t v = 0; v < 8; v++) {
            vertexes[v].z = 0.0f;
        }
        float y1 = draw.center.y - draw.size.y / 2.0f;
        float y2 = y1 + 20.0f;
        ALLEGRO_COLOR c_opaque = al_map_rgba(255, 255, 255, 64);
        ALLEGRO_COLOR c_empty = al_map_rgba(255, 255, 255, 0);
        vertexes[0].x = draw.center.x - draw.size.x * 0.49;
        vertexes[0].y = y1;
        vertexes[0].color = c_empty;
        vertexes[1].x = draw.center.x - draw.size.x * 0.49;
        vertexes[1].y = y2;
        vertexes[1].color = c_empty;
        vertexes[2].x = draw.center.x - draw.size.x * 0.47;
        vertexes[2].y = y1;
        vertexes[2].color = c_opaque;
        vertexes[3].x = draw.center.x - draw.size.x * 0.47;
        vertexes[3].y = y2;
        vertexes[3].color = c_empty;
        vertexes[4].x = draw.center.x + draw.size.x * 0.47;
        vertexes[4].y = y1;
        vertexes[4].color = c_opaque;
        vertexes[5].x = draw.center.x + draw.size.x * 0.47;
        vertexes[5].y = y2;
        vertexes[5].color = c_empty;
        vertexes[6].x = draw.center.x + draw.size.x * 0.49;
        vertexes[6].y = y1;
        vertexes[6].color = c_empty;
        vertexes[7].x = draw.center.x + draw.size.x * 0.49;
        vertexes[7].y = y2;
        vertexes[7].color = c_empty;
        al_draw_prim(
            vertexes, nullptr, nullptr, 0, 8, ALLEGRO_PRIM_TRIANGLE_STRIP
        );
    }
    float child_bottom = getChildBottom();
    if(child_bottom > 1.0f && offset < child_bottom - 1.0f) {
        //Shade effect at the bottom.
        ALLEGRO_VERTEX vertexes[8];
        for(size_t v = 0; v < 8; v++) {
            vertexes[v].z = 0.0f;
        }
        float y1 = draw.center.y + draw.size.y / 2.0f;
        float y2 = y1 - 20.0f;
        ALLEGRO_COLOR c_opaque = al_map_rgba(255, 255, 255, 64);
        ALLEGRO_COLOR c_empty = al_map_rgba(255, 255, 255, 0);
        vertexes[0].x = draw.center.x - draw.size.x * 0.49;
        vertexes[0].y = y1;
        vertexes[0].color = c_empty;
        vertexes[1].x = draw.center.x - draw.size.x * 0.49;
        vertexes[1].y = y2;
        vertexes[1].color = c_empty;
        vertexes[2].x = draw.center.x - draw.size.x * 0.47;
        vertexes[2].y = y1;
        vertexes[2].color = c_opaque;
        vertexes[3].x = draw.center.x - draw.size.x * 0.47;
        vertexes[3].y = y2;
        vertexes[3].color = c_empty;
        vertexes[4].x = draw.center.x + draw.size.x * 0.47;
        vertexes[4].y = y1;
        vertexes[4].color = c_opaque;
        vertexes[5].x = draw.center.x + draw.size.x * 0.47;
        vertexes[5].y = y2;
        vertexes[5].color = c_empty;
        vertexes[6].x = draw.center.x + draw.size.x * 0.49;
        vertexes[6].y = y1;
        vertexes[6].color = c_empty;
        vertexes[7].x = draw.center.x + draw.size.x * 0.49;
        vertexes[7].y = y2;
        vertexes[7].color = c_empty;
        al_draw_prim(
            vertexes, nullptr, nullptr, 0, 8, ALLEGRO_PRIM_TRIANGLE_STRIP
        );
    }
}


/**
 * @brief Default list GUI item event code.
 */
void ListGuiItem::defEventCode(const ALLEGRO_EVENT  &ev) {
    if(
        ev.type == ALLEGRO_EVENT_MOUSE_AXES &&
        isMouseOn(Point(ev.mouse.x, ev.mouse.y)) &&
        ev.mouse.dz != 0.0f
    ) {
        float child_bottom = getChildBottom();
        if(child_bottom <= 1.0f && offset == 0.0f) {
            return;
        }
        targetOffset =
            std::clamp(
                targetOffset + (-ev.mouse.dz) * 0.2f,
                0.0f,
                child_bottom - 1.0f
            );
    }
}


/**
 * @brief Default list GUI item tick code.
 */
void ListGuiItem::defTickCode(float delta_t) {
    float child_bottom = getChildBottom();
    if(child_bottom < 1.0f) {
        targetOffset = 0.0f;
        offset = 0.0f;
    } else {
        targetOffset = std::clamp(targetOffset, 0.0f, child_bottom - 1.0f);
        offset += (targetOffset - offset) * (10.0f * delta_t);
        offset = std::clamp(offset, 0.0f, child_bottom - 1.0f);
        if(offset <= 0.01f) offset = 0.0f;
        if(child_bottom > 1.0f) {
            if(child_bottom - offset - 1.0f <= 0.01f) {
                offset = child_bottom - 1.0f;
            }
        }
    }
}


/**
 * @brief Constructs a new picker gui item object.
 *
 * @param base_text Text to display before the current option's name.
 * @param option Text that matches the current option.
 * @param nr_options Total amount of options.
 * @param cur_option_idx Index of the currently selected option.
 */
PickerGuiItem::PickerGuiItem(
    const string &base_text, const string &option,
    size_t nr_options, size_t cur_option_idx
) :
    GuiItem(true),
    baseText(base_text),
    option(option),
    nrOptions(nr_options),
    curOptionIdx(cur_option_idx) {
    
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
    
    onActivate =
    [this] (const Point & cursor_pos) {
        this->defActivateCode(cursor_pos);
    };
    
    onMenuDirButton =
    [this] (size_t button_id) -> bool{
        return this->defMenuDirCode(button_id);
    };
    
    onMouseOver =
    [this] (const Point & cursor_pos) {
        this->defMouseOverCode(cursor_pos);
    };
}


/**
 * @brief Default picker GUI item activate code.
 */
void PickerGuiItem::defActivateCode(const Point &cursor_pos) {
    if(cursor_pos.x >= getReferenceCenter().x) {
        onNext();
    } else {
        onPrevious();
    }
}


/**
 * @brief Default picker GUI item draw code.
 */
void PickerGuiItem::defDrawCode(const DrawInfo &draw) {
    if(this->nrOptions != 0 && selected) {
        Point option_boxes_start(
            draw.center.x - draw.size.x / 2.0f + 20.0f,
            draw.center.y + draw.size.y / 2.0f - 12.0f
        );
        float option_boxes_interval =
            (draw.size.x - 40.0f) / (this->nrOptions - 0.5f);
        for(size_t o = 0; o < this->nrOptions; o++) {
            float x1 = option_boxes_start.x + o * option_boxes_interval;
            float y1 = option_boxes_start.y;
            al_draw_filled_rectangle(
                x1, y1,
                x1 + option_boxes_interval * 0.5f, y1 + 4.0f,
                this->curOptionIdx == o ?
                al_map_rgba(255, 255, 255, 160) :
                al_map_rgba(255, 255, 255, 64)
            );
        }
    }
    
    unsigned char real_arrow_highlight = 255;
    if(
        selected &&
        manager &&
        manager->wasLastInputMouse()
    ) {
        real_arrow_highlight = arrowHighlight;
    }
    ALLEGRO_COLOR arrow_highlight_color = al_map_rgb(87, 200, 208);
    ALLEGRO_COLOR arrow_regular_color = COLOR_WHITE;
    Point arrow_highlight_scale = Point(1.4f);
    Point arrow_regular_scale = Point(1.0f);
    
    Point arrow_box(
        draw.size.x * 0.10 * GUI::STANDARD_CONTENT_SIZE.x,
        draw.size.y * GUI::STANDARD_CONTENT_SIZE.y
    );
    drawText(
        "<",
        game.sysContent.fntStandard,
        Point(draw.center.x - draw.size.x * 0.45, draw.center.y),
        arrow_box,
        real_arrow_highlight == 0 ?
        arrow_highlight_color :
        arrow_regular_color,
        ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER,
        TEXT_SETTING_FLAG_CANT_GROW,
        real_arrow_highlight == 0 ?
        arrow_highlight_scale :
        arrow_regular_scale
    );
    drawText(
        ">",
        game.sysContent.fntStandard,
        Point(draw.center.x + draw.size.x * 0.45, draw.center.y),
        arrow_box,
        real_arrow_highlight == 1 ?
        arrow_highlight_color :
        arrow_regular_color,
        ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER,
        TEXT_SETTING_FLAG_CANT_GROW,
        real_arrow_highlight == 1 ?
        arrow_highlight_scale :
        arrow_regular_scale
    );
    
    float juicy_grow_amount = this->getJuiceValue();
    
    Point text_box(draw.size.x * 0.80, draw.size.y * GUI::STANDARD_CONTENT_SIZE.y);
    drawText(
        this->baseText + this->option,
        game.sysContent.fntStandard,
        Point(draw.center.x - draw.size.x * 0.40, draw.center.y),
        text_box,
        COLOR_WHITE,
        ALLEGRO_ALIGN_LEFT, V_ALIGN_MODE_CENTER,
        TEXT_SETTING_FLAG_CANT_GROW,
        Point(1.0f + juicy_grow_amount)
    );
    
    ALLEGRO_COLOR box_tint =
        selected ? al_map_rgb(87, 200, 208) : COLOR_WHITE;
        
    drawTexturedBox(
        draw.center, draw.size, game.sysContent.bmpBubbleBox, box_tint
    );
    
    if(selected) {
        drawTexturedBox(
            draw.center,
            draw.size + 10.0 + sin(game.timePassed * TAU) * 2.0f,
            game.sysContent.bmpFocusBox
        );
    }
}


/**
 * @brief Default picker GUI item menu dir code.
 */
bool PickerGuiItem::defMenuDirCode(size_t button_id) {
    if(button_id == PLAYER_ACTION_TYPE_MENU_RIGHT) {
        onNext();
        return true;
    } else if(button_id == PLAYER_ACTION_TYPE_MENU_LEFT) {
        onPrevious();
        return true;
    }
    return false;
}


/**
 * @brief Default picker GUI item mouse over code.
 */
void PickerGuiItem::defMouseOverCode(const Point  &cursor_pos) {
    arrowHighlight =
        cursor_pos.x >= getReferenceCenter().x ? 1 : 0;
}


/**
 * @brief Constructs a new scroll gui item object.
 */
ScrollGuiItem::ScrollGuiItem() :
    GuiItem() {
    
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
    onAllegroEvent =
    [this] (const ALLEGRO_EVENT & ev) {
        this->defEventCode(ev);
    };
}


/**
 * @brief Default scroll GUI item draw code.
 */
void ScrollGuiItem::defDrawCode(const DrawInfo &draw) {
    float bar_y = 0.0f; //Top, in height ratio.
    float bar_h = 0.0f; //In height ratio.
    float list_bottom = listItem->getChildBottom();
    unsigned char alpha = 48;
    if(list_bottom > 1.0f) {
        float offset = std::min(listItem->offset, list_bottom - 1.0f);
        bar_y = offset / list_bottom;
        bar_h = 1.0f / list_bottom;
        alpha = 128;
    }
    
    drawTexturedBox(
        draw.center, draw.size, game.sysContent.bmpFrameBox,
        al_map_rgba(255, 255, 255, alpha)
    );
    
    if(bar_h != 0.0f) {
        drawTexturedBox(
            Point(
                draw.center.x,
                (draw.center.y - draw.size.y * 0.5) +
                (draw.size.y * bar_y) +
                (draw.size.y * bar_h * 0.5f)
            ),
            Point(draw.size.x, (draw.size.y * bar_h)),
            game.sysContent.bmpBubbleBox
        );
    }
}


/**
 * @brief Default scroll GUI item event code.
 */
void ScrollGuiItem::defEventCode(const ALLEGRO_EVENT  &ev) {
    if(
        ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
        ev.mouse.button == 1 &&
        isMouseOn(Point(ev.mouse.x, ev.mouse.y))
    ) {
        float list_bottom = listItem->getChildBottom();
        if(list_bottom <= 1.0f) {
            return;
        }
        
        Point c = getReferenceCenter();
        Point s = getReferenceSize();
        float bar_h = (1.0f / list_bottom) * s.y;
        float y1 = (c.y - s.y / 2.0f) + bar_h / 2.0f;
        float y2 = (c.y + s.y / 2.0f) - bar_h / 2.0f;
        float click = (ev.mouse.y - y1) / (y2 - y1);
        click = std::clamp(click, 0.0f, 1.0f);
        
        listItem->targetOffset = click * (list_bottom - 1.0f);
    }
}


/**
 * @brief Constructs a new text gui item object.
 *
 * @param text Text to display.
 * @param font Font to use for the text.
 * @param color Color to use for the text.
 * @param flags Allegro text flags to use.
 */
TextGuiItem::TextGuiItem(
    const string &text, ALLEGRO_FONT* font, const ALLEGRO_COLOR &color,
    int flags
) :
    GuiItem(),
    text(text),
    font(font),
    color(color),
    flags(flags) {
    
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
}


/**
 * @brief Default text GUI item draw code.
 */
void TextGuiItem::defDrawCode(const DrawInfo &draw) {
    int text_x = draw.center.x;
    switch(this->flags) {
    case ALLEGRO_ALIGN_LEFT: {
        text_x = draw.center.x - draw.size.x * 0.5;
        break;
    } case ALLEGRO_ALIGN_RIGHT: {
        text_x = draw.center.x + draw.size.x * 0.5;
        break;
    }
    }
    
    float juicy_grow_amount = getJuiceValue();
    int text_y = draw.center.y;
    
    if(lineWrap) {
    
        text_y = draw.center.y - draw.size.y / 2.0f;
        int line_height = al_get_font_line_height(this->font);
        vector<StringToken> tokens =
            tokenizeString(this->text);
        setStringTokenWidths(
            tokens, this->font, game.sysContent.fntSlim, line_height, false
        );
        vector<vector<StringToken> > tokens_per_line =
            splitLongStringWithTokens(tokens, draw.size.x);
            
        for(size_t l = 0; l < tokens_per_line.size(); l++) {
            drawStringTokens(
                tokens_per_line[l], this->font, game.sysContent.fntSlim,
                false,
                Point(
                    text_x,
                    text_y + l * line_height
                ),
                this->flags,
                Point(draw.size.x, line_height),
                Point(1.0f + juicy_grow_amount)
            );
        }
        
    } else {
    
        drawText(
            this->text, this->font, Point(text_x, text_y), draw.size,
            this->color, this->flags, V_ALIGN_MODE_CENTER,
            TEXT_SETTING_FLAG_CANT_GROW,
            Point(1.0 + juicy_grow_amount)
        );
        
    }
    
    if(selected && showSelectionBox) {
        drawTexturedBox(
            draw.center,
            draw.size + 10.0 + sin(game.timePassed * TAU) * 2.0f,
            game.sysContent.bmpFocusBox
        );
    }
}


/**
 * @brief Constructs a new tooltip gui item object.
 *
 * @param gui Pointer to the GUI it belongs to.
 */
TooltipGuiItem::TooltipGuiItem(GuiManager* gui) :
    GuiItem(),
    gui(gui) {
    
    onDraw =
    [this] (const DrawInfo & draw) {
        this->defDrawCode(draw);
    };
}


/**
 * @brief Default tooltip GUI item draw code.
 */
void TooltipGuiItem::defDrawCode(const DrawInfo &draw) {
    string cur_text = this->gui->getCurrentTooltip();
    if(cur_text != this->prevText) {
        this->startJuiceAnimation(JUICE_TYPE_GROW_TEXT_LOW);
        this->prevText = cur_text;
    }
    float juicy_grow_amount = getJuiceValue();
    drawText(
        cur_text, game.sysContent.fntStandard,
        draw.center, draw.size,
        COLOR_WHITE, ALLEGRO_ALIGN_CENTER, V_ALIGN_MODE_CENTER,
        TEXT_SETTING_FLAG_CANT_GROW,
        Point(0.7f + juicy_grow_amount)
    );
}
