/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Onion menu classes and functions.
 */

#include <algorithm>

#include "../../drawing.h"
#include "../../functions.h"
#include "../../game.h"
#include "../../utils/string_utils.h"
#include "gameplay.h"


namespace ONION_MENU {
//Path to the GUI information file.
const string GUI_FILE_PATH = GUI_FOLDER_PATH + "/Onion_menu.txt";
//How long to let text turn red for.
const float RED_TEXT_DURATION = 1.0f;
//The Onion menu can only show, at most, these many Pikmin types per page.
const size_t TYPES_PER_PAGE = 5;
}


/* ----------------------------------------------------------------------------
 * Creates an Onion menu struct.
 * n_ptr:
 *   Pointer to the nest information struct.
 * l_ptr:
 *   Leader responsible.
 */
onion_menu_struct::onion_menu_struct(
    pikmin_nest_struct* n_ptr, leader* l_ptr
) :
    n_ptr(n_ptr),
    l_ptr(l_ptr),
    select_all(false),
    page(0),
    nr_pages(0),
    onion_all_button(nullptr),
    group_all_button(nullptr),
    onion_more_l_icon(nullptr),
    onion_more_r_icon(nullptr),
    group_more_l_icon(nullptr),
    group_more_r_icon(nullptr),
    prev_page_button(nullptr),
    next_page_button(nullptr),
    field_amount_text(nullptr),
    bg_alpha_mult(0.0f),
    closing_timer(0.0f),
    to_delete(false),
    closing(false) {
    
    for(size_t t = 0; t < n_ptr->nest_type->pik_types.size(); ++t) {
        types.push_back(
            onion_menu_type_struct(t, n_ptr->nest_type->pik_types[t])
        );
    }
    
    nr_pages = ceil(types.size() / (float) ONION_MENU::TYPES_PER_PAGE);
    
    gui.register_coords("instructions",     50,  7, 90, 20);
    gui.register_coords("cancel",           16, 85, 18, 11);
    gui.register_coords("ok",               84, 85, 18, 11);
    gui.register_coords("field",            50, 75, 18,  4);
    gui.register_coords("select_all",       50, 87, 24,  6);
    gui.register_coords("onion_1_button",   50, 20,  9, 12);
    gui.register_coords("onion_2_button",   50, 20,  9, 12);
    gui.register_coords("onion_3_button",   50, 20,  9, 12);
    gui.register_coords("onion_4_button",   50, 20,  9, 12);
    gui.register_coords("onion_5_button",   50, 20,  9, 12);
    gui.register_coords("onion_1_amount",   50, 29, 12,  4);
    gui.register_coords("onion_2_amount",   50, 29, 12,  4);
    gui.register_coords("onion_3_amount",   50, 29, 12,  4);
    gui.register_coords("onion_4_amount",   50, 29, 12,  4);
    gui.register_coords("onion_5_amount",   50, 29, 12,  4);
    gui.register_coords("group_1_button",   50, 60,  9, 12);
    gui.register_coords("group_2_button",   50, 60,  9, 12);
    gui.register_coords("group_3_button",   50, 60,  9, 12);
    gui.register_coords("group_4_button",   50, 60,  9, 12);
    gui.register_coords("group_5_button",   50, 60,  9, 12);
    gui.register_coords("group_1_amount",   50, 51, 12,  4);
    gui.register_coords("group_2_amount",   50, 51, 12,  4);
    gui.register_coords("group_3_amount",   50, 51, 12,  4);
    gui.register_coords("group_4_amount",   50, 51, 12,  4);
    gui.register_coords("group_5_amount",   50, 51, 12,  4);
    gui.register_coords("onion_all",        50, 20,  9, 12);
    gui.register_coords("group_all",        50, 60,  9, 12);
    gui.register_coords("prev_page",         5, 40,  8, 10);
    gui.register_coords("next_page",        95, 40,  8, 11);
    gui.register_coords("onion_left_more",   5, 20,  3,  4);
    gui.register_coords("onion_right_more", 95, 20,  3,  4);
    gui.register_coords("group_left_more",   5, 60,  3,  4);
    gui.register_coords("group_right_more", 95, 60,  3,  4);
    gui.register_coords("tooltip",          50, 95, 95,  8);
    gui.read_coords(
        data_node(ONION_MENU::GUI_FILE_PATH).get_child_by_name("positions")
    );
    
    //Instructions text.
    text_gui_item* instructions_text =
        new text_gui_item(
        "Call or store Pikmin", game.fonts.standard, al_map_rgb(188, 230, 230)
    );
    gui.add_item(instructions_text, "instructions");
    
    //Cancel button.
    gui.back_item =
        new button_gui_item(
        "Cancel", game.fonts.standard, al_map_rgb(226, 112, 112)
    );
    gui.back_item->on_activate =
    [this] (const point &) {
        start_closing();
    };
    gui.back_item->on_get_tooltip =
    [] () { return "Forget all changes and leave the Onion menu."; };
    gui.add_item(gui.back_item, "cancel");
    
    //Ok button.
    button_gui_item* ok_button =
        new button_gui_item(
        "Ok", game.fonts.standard, al_map_rgb(96, 226, 80)
    );
    ok_button->on_activate =
    [this] (const point &) {
        confirm();
        start_closing();
    };
    ok_button->on_get_tooltip =
    [] () { return "Confirm changes."; };
    gui.add_item(ok_button, "ok");
    
    //Field amount text.
    field_amount_text =
        new text_gui_item("", game.fonts.standard);
    field_amount_text->on_draw =
    [this] (const point & center, const point & size) {
        int total_delta = 0;
        for(size_t t = 0; t < this->types.size(); ++t) {
            total_delta += this->types[t].delta;
        }
        
        draw_filled_rounded_rectangle(
            center, size, game.win_w * 0.01,
            al_map_rgba(188, 230, 230, 128)
        );
        
        ALLEGRO_COLOR color = al_map_rgb(188, 230, 230);
        const auto &red_it = this->red_items.find(field_amount_text);
        if(red_it != this->red_items.end()) {
            color =
                interpolate_color(
                    red_it->second,
                    0, ONION_MENU::RED_TEXT_DURATION,
                    color, al_map_rgb(224, 0, 0)
                );
        }
        
        float juicy_grow_amount = field_amount_text->get_juice_value();
        draw_compressed_scaled_text(
            game.fonts.standard,
            color,
            center,
            point(1.0f + juicy_grow_amount, 1.0f + juicy_grow_amount),
            ALLEGRO_ALIGN_CENTER, TEXT_VALIGN_CENTER,
            size, true,
            "Field: " +
            i2s(game.states.gameplay->mobs.pikmin_list.size() + total_delta)
        );
    };
    gui.add_item(field_amount_text, "field");
    
    //Select all checkbox.
    check_gui_item* select_all_check =
        new check_gui_item(
        &select_all,
        "Select all", game.fonts.standard, al_map_rgb(188, 230, 230)
    );
    select_all_check->on_activate =
    [this, select_all_check] (const point &) {
        select_all = !select_all;
        grow_buttons();
        select_all_check->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_ELASTIC_MEDIUM
        );
        update();
    };
    select_all_check->visible = types.size() > 1;
    select_all_check->selectable = types.size() > 1;
    select_all_check->on_get_tooltip =
    [] () { return "Control all Pikmin numbers at once?"; };
    gui.add_item(select_all_check, "select_all");
    
    //Onion icons and buttons.
    for(size_t t = 0; t < ONION_MENU::TYPES_PER_PAGE; ++t) {
        string id = "onion_" + i2s(t + 1) + "_button";
        
        gui_item* onion_icon = new gui_item(false);
        onion_icon->on_draw =
        [this, t, onion_icon] (const point & center, const point & size) {
            onion_menu_type_struct* t_ptr = this->on_screen_types[t];
            if(t_ptr->pik_type->bmp_onion_icon) {
                float juicy_grow_amount = onion_icon->get_juice_value();
                draw_bitmap_in_box(
                    t_ptr->pik_type->bmp_onion_icon, center,
                    (size * 0.8f) + juicy_grow_amount, true
                );
            }
        };
        gui.add_item(onion_icon, id);
        onion_icon_items.push_back(onion_icon);
        
        button_gui_item* onion_button =
            new button_gui_item("", game.fonts.standard);
        onion_button->on_draw =
        [this, onion_button] (const point & center, const point & size) {
            float juicy_grow_amount = onion_button->get_juice_value();
            draw_button(
                center,
                size + juicy_grow_amount,
                "", game.fonts.standard, COLOR_WHITE,
                onion_button->selected
            );
        };
        onion_button->on_activate =
        [this, t] (const point &) {
            add_to_onion(on_screen_types[t]->type_idx);
        };
        onion_button->can_auto_repeat = true;
        onion_button->on_get_tooltip =
        [this, t] () {
            onion_menu_type_struct* t_ptr = this->on_screen_types[t];
            return "Store one " + t_ptr->pik_type->name + " inside.";
        };
        gui.add_item(onion_button, id);
        onion_button_items.push_back(onion_button);
    }
    
    //Onion's all button.
    onion_all_button =
        new button_gui_item("", game.fonts.standard);
    onion_all_button->on_draw =
    [this] (const point & center, const point & size) {
        float juicy_grow_amount = onion_all_button->get_juice_value();
        draw_button(
            center,
            size + juicy_grow_amount,
            "", game.fonts.standard, COLOR_WHITE,
            onion_all_button->selected
        );
    };
    onion_all_button->on_activate =
    [this] (const point &) {
        add_all_to_onion();
    };
    onion_all_button->can_auto_repeat = true;
    onion_all_button->on_get_tooltip =
    [] () { return "Store one Pikmin of each type inside."; };
    gui.add_item(onion_all_button, "onion_all");
    
    //Onion amounts.
    for(size_t t = 0; t < ONION_MENU::TYPES_PER_PAGE; ++t) {
        gui_item* onion_amount_text = new gui_item(false);
        onion_amount_text->on_draw =
            [this, t, onion_amount_text]
        (const point & center, const point & size) {
            onion_menu_type_struct* t_ptr = this->on_screen_types[t];
            
            size_t real_onion_amount =
                this->n_ptr->get_amount_by_type(t_ptr->pik_type);
                
            draw_filled_rounded_rectangle(
                center, size, game.win_w * 0.01,
                al_map_rgba(188, 230, 230, 128)
            );
            
            ALLEGRO_COLOR color = al_map_rgb(255, 255, 255);
            const auto &red_it = this->red_items.find(onion_amount_text);
            if(red_it != this->red_items.end()) {
                color =
                    interpolate_color(
                        red_it->second,
                        0, ONION_MENU::RED_TEXT_DURATION,
                        color, al_map_rgb(224, 0, 0)
                    );
            }
            
            float juicy_grow_amount = onion_amount_text->get_juice_value();
            draw_compressed_scaled_text(
                game.fonts.area_name,
                color,
                center,
                point(1.0f + juicy_grow_amount, 1.0f + juicy_grow_amount),
                ALLEGRO_ALIGN_CENTER, TEXT_VALIGN_CENTER,
                size, true, i2s(real_onion_amount - t_ptr->delta)
            );
        };
        gui.add_item(onion_amount_text, "onion_" + i2s(t + 1) + "_amount");
        onion_amount_items.push_back(onion_amount_text);
    }
    
    //Group icons and buttons.
    for(size_t t = 0; t < ONION_MENU::TYPES_PER_PAGE; ++t) {
        string id = "group_" + i2s(t + 1) + "_button";
        
        gui_item* group_icon = new gui_item(false);
        group_icon->on_draw =
        [this, t, group_icon] (const point & center, const point & size) {
            onion_menu_type_struct* t_ptr = this->on_screen_types[t];
            float juicy_grow_amount = group_icon->get_juice_value();
            if(t_ptr->pik_type->bmp_icon) {
                draw_bitmap_in_box(
                    t_ptr->pik_type->bmp_icon, center,
                    (size * 0.8f) + juicy_grow_amount, true
                );
            }
        };
        gui.add_item(group_icon, id);
        group_icon_items.push_back(group_icon);
        
        button_gui_item* group_button =
            new button_gui_item("", game.fonts.standard);
        group_button->on_draw =
        [this, group_button] (const point & center, const point & size) {
            float juicy_grow_amount = group_button->get_juice_value();
            draw_button(
                center,
                size + juicy_grow_amount,
                "", game.fonts.standard, COLOR_WHITE,
                group_button->selected
            );
        };
        group_button->on_activate =
        [this, t] (const point &) {
            add_to_group(on_screen_types[t]->type_idx);
        };
        group_button->can_auto_repeat = true;
        group_button->on_get_tooltip =
        [this, t] () {
            onion_menu_type_struct* t_ptr = this->on_screen_types[t];
            return "Call one " + t_ptr->pik_type->name + " to the group.";
        };
        gui.add_item(group_button, id);
        group_button_items.push_back(group_button);
    }
    
    //Group's all button.
    group_all_button =
        new button_gui_item("", game.fonts.standard);
    group_all_button->on_draw =
    [this] (const point & center, const point & size) {
        float juicy_grow_amount = group_all_button->get_juice_value();
        draw_button(
            center,
            size + juicy_grow_amount,
            "", game.fonts.standard, COLOR_WHITE,
            group_all_button->selected
        );
    };
    group_all_button->on_activate =
    [this] (const point &) {
        add_all_to_group();
    };
    group_all_button->can_auto_repeat = true;
    group_all_button->on_get_tooltip =
    [] () { return "Call one Pikmin of each type to the group."; };
    gui.add_item(group_all_button, "group_all");
    
    //Group amounts.
    for(size_t t = 0; t < ONION_MENU::TYPES_PER_PAGE; ++t) {
        gui_item* group_amount_text = new gui_item(false);
        group_amount_text->on_draw =
            [this, t, group_amount_text]
        (const point & center, const point & size) {
            onion_menu_type_struct* t_ptr = this->on_screen_types[t];
            
            size_t real_group_amount =
                this->l_ptr->group->get_amount_by_type(t_ptr->pik_type);
                
            draw_filled_rounded_rectangle(
                center, size, game.win_w * 0.01,
                al_map_rgba(188, 230, 230, 128)
            );
            
            ALLEGRO_COLOR color = al_map_rgb(255, 255, 255);
            const auto &red_it = this->red_items.find(group_amount_text);
            if(red_it != this->red_items.end()) {
                color =
                    interpolate_color(
                        red_it->second,
                        0, ONION_MENU::RED_TEXT_DURATION,
                        color, al_map_rgb(224, 0, 0)
                    );
            }
            
            float juicy_grow_amount = group_amount_text->get_juice_value();
            draw_compressed_scaled_text(
                game.fonts.area_name,
                color,
                center,
                point(1.0f + juicy_grow_amount, 1.0f + juicy_grow_amount),
                ALLEGRO_ALIGN_CENTER, TEXT_VALIGN_CENTER,
                size, true, i2s(real_group_amount + t_ptr->delta)
            );
        };
        gui.add_item(group_amount_text, "group_" + i2s(t + 1) + "_amount");
        group_amount_items.push_back(group_amount_text);
    }
    
    //Onion left "more" indicator.
    onion_more_l_icon = new gui_item(false);
    onion_more_l_icon->on_draw =
    [this] (const point & center, const point & size) {
        draw_bitmap(
            game.sys_assets.bmp_more,
            center,
            point(-size.x, size.y) * 0.8f,
            0, map_gray(128)
        );
    };
    gui.add_item(onion_more_l_icon, "onion_left_more");
    
    //Onion right "more" indicator.
    onion_more_r_icon = new gui_item(false);
    onion_more_r_icon->on_draw =
    [this] (const point & center, const point & size) {
        draw_bitmap(
            game.sys_assets.bmp_more,
            center,
            size * 0.8f,
            0, map_gray(128)
        );
    };
    gui.add_item(onion_more_r_icon, "onion_right_more");
    
    //Group left "more" indicator.
    group_more_l_icon = new gui_item(false);
    group_more_l_icon->on_draw =
    [this] (const point & center, const point & size) {
        draw_bitmap(
            game.sys_assets.bmp_more,
            center,
            point(-size.x, size.y) * 0.8f,
            0, map_gray(128)
        );
    };
    gui.add_item(group_more_l_icon, "group_left_more");
    
    //Group right "more" indicator.
    group_more_r_icon = new gui_item(false);
    group_more_r_icon->on_draw =
    [this] (const point & center, const point & size) {
        draw_bitmap(
            game.sys_assets.bmp_more,
            center,
            size * 0.8f,
            0, map_gray(128)
        );
    };
    gui.add_item(group_more_r_icon, "group_right_more");
    
    //Previous page button.
    prev_page_button = new gui_item(true);
    prev_page_button->on_draw =
    [this] (const point & center, const point & size) {
        draw_bitmap(
            game.sys_assets.bmp_more,
            center, point(-size.x, size.y) * 0.5f
        );
        
        draw_button(
            center, size, "", game.fonts.standard, COLOR_WHITE,
            prev_page_button->selected,
            prev_page_button->get_juice_value()
        );
    };
    prev_page_button->on_activate =
    [this] (const point &) {
        go_to_page(sum_and_wrap((int) page, -1, (int) nr_pages));
    };
    prev_page_button->visible = nr_pages > 1;
    prev_page_button->selectable = nr_pages > 1;
    prev_page_button->on_get_tooltip =
    [] () { return "Go to the previous page of Pikmin types."; };
    gui.add_item(prev_page_button, "prev_page");
    
    //Next page button.
    next_page_button = new gui_item(true);
    next_page_button->on_draw =
    [this] (const point & center, const point & size) {
        draw_bitmap(
            game.sys_assets.bmp_more,
            center, size * 0.5f
        );
        
        draw_button(
            center, size, "", game.fonts.standard, COLOR_WHITE,
            next_page_button->selected,
            next_page_button->get_juice_value()
        );
    };
    next_page_button->on_activate =
    [this] (const point &) {
        go_to_page(sum_and_wrap((int) page, 1, (int) nr_pages));
    };
    next_page_button->visible = nr_pages > 1;
    next_page_button->selectable = nr_pages > 1;
    next_page_button->on_get_tooltip =
    [] () { return "Go to the next page of Pikmin types."; };
    gui.add_item(next_page_button, "next_page");
    
    //Tooltip text.
    tooltip_gui_item* tooltip_text =
        new tooltip_gui_item(&gui);
    gui.add_item(tooltip_text, "tooltip");
    
    //Finishing touches.
    update();
    gui.start_animation(
        GUI_MANAGER_ANIM_UP_TO_CENTER,
        GAMEPLAY::MENU_ENTRY_HUD_MOVE_TIME
    );
}


/* ----------------------------------------------------------------------------
 * Destroys an Onion menu struct.
 */
onion_menu_struct::~onion_menu_struct() {
    gui.destroy();
}


/* ----------------------------------------------------------------------------
 * Adds one Pikmin of each type from Onion to the group, if possible.
 */
void onion_menu_struct::add_all_to_group() {
    for(size_t t = 0; t < types.size(); ++t) {
        add_to_group(t);
    }
}


/* ----------------------------------------------------------------------------
 * Adds one Pikmin of each type from the group to the Onion, if possible.
 */
void onion_menu_struct::add_all_to_onion() {
    for(size_t t = 0; t < types.size(); ++t) {
        add_to_onion(t);
    }
}


/* ----------------------------------------------------------------------------
 * Adds one Pikmin from the Onion to the group, if possible.
 * type_idx:
 *   Index of the Onion's Pikmin type.
 */
void onion_menu_struct::add_to_group(const size_t type_idx) {
    size_t real_onion_amount =
        n_ptr->get_amount_by_type(n_ptr->nest_type->pik_types[type_idx]);
        
    //First, check if there are enough in the Onion to take out.
    if((signed int) (real_onion_amount - types[type_idx].delta) <= 0) {
        size_t screen_idx = types[type_idx].on_screen_idx;
        if(screen_idx != INVALID) {
            make_gui_item_red(onion_amount_items[screen_idx]);
        }
        return;
    }
    
    //Next, check if the addition won't make the field amount hit the limit.
    int total_delta = 0;
    for(size_t t = 0; t < types.size(); ++t) {
        total_delta += types[t].delta;
    }
    if(
        game.states.gameplay->mobs.pikmin_list.size() + total_delta >=
        game.config.max_pikmin_in_field
    ) {
        make_gui_item_red(field_amount_text);
        return;
    }
    
    types[type_idx].delta++;
    
    size_t on_screen_idx = types[type_idx].on_screen_idx;
    if(on_screen_idx != INVALID) {
        onion_amount_items[on_screen_idx]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_HIGH
        );
        group_amount_items[on_screen_idx]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_HIGH
        );
    }
    field_amount_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_MEDIUM
    );
    
}


/* ----------------------------------------------------------------------------
 * Adds one Pikmin from the group to the Onion, if possible.
 * type_idx:
 *   Index of the Onion's Pikmin type.
 */
void onion_menu_struct::add_to_onion(const size_t type_idx) {
    size_t real_group_amount =
        l_ptr->group->get_amount_by_type(n_ptr->nest_type->pik_types[type_idx]);
        
    if((signed int) (real_group_amount + types[type_idx].delta) <= 0) {
        size_t screen_idx = types[type_idx].on_screen_idx;
        if(screen_idx != INVALID) {
            make_gui_item_red(group_amount_items[screen_idx]);
        }
        return;
    }
    
    types[type_idx].delta--;
    
    size_t on_screen_idx = types[type_idx].on_screen_idx;
    if(on_screen_idx != INVALID) {
        onion_amount_items[on_screen_idx]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_HIGH
        );
        group_amount_items[on_screen_idx]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_TEXT_HIGH
        );
    }
    field_amount_text->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_TEXT_MEDIUM
    );
}


/* ----------------------------------------------------------------------------
 * Confirms the player's changes, and sets up the Pikmin to climb up the
 * Onion, if any, and sets up the Onion to spit out Pikmin, if any.
 */
void onion_menu_struct::confirm() {
    for(size_t t = 0; t < types.size(); ++t) {
        if(types[t].delta > 0) {
            n_ptr->request_pikmin(t, types[t].delta, l_ptr);
        } else if(types[t].delta < 0) {
            l_ptr->order_pikmin_to_onion(
                types[t].pik_type, n_ptr, -types[t].delta
            );
        }
    }
}


/* ----------------------------------------------------------------------------
 * Flips to the specified page of Pikmin types.
 * page:
 *   Index of the new page.
 */
void onion_menu_struct::go_to_page(const size_t page) {
    this->page = page;
    grow_buttons();
    update();
}


/* ----------------------------------------------------------------------------
 * Makes the Onion and group buttons juicy grow.
 */
void onion_menu_struct::grow_buttons() {
    for(size_t t = 0; t < ONION_MENU::TYPES_PER_PAGE; ++t) {
        onion_icon_items[t]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_ICON
        );
        onion_button_items[t]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_ICON
        );
        group_icon_items[t]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_ICON
        );
        group_button_items[t]->start_juice_animation(
            gui_item::JUICE_TYPE_GROW_ICON
        );
    }
    onion_all_button->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_ICON
    );
    group_all_button->start_juice_animation(
        gui_item::JUICE_TYPE_GROW_ICON
    );
}


/* ----------------------------------------------------------------------------
 * Handles an Allegro event.
 * ev:
 *   Event to handle.
 */
void onion_menu_struct::handle_event(const ALLEGRO_EVENT &ev) {
    if(!closing) gui.handle_event(ev);
}


/* ----------------------------------------------------------------------------
 * Handles a player action.
 * action:
 *   Data about the player action.
 */
void onion_menu_struct::handle_player_action(const player_action &action) {
    gui.handle_player_action(action);
}


/* ----------------------------------------------------------------------------
 * Makes a given GUI item turn red.
 * item:
 *   The item.
 */
void onion_menu_struct::make_gui_item_red(gui_item* item) {
    red_items[item] = ONION_MENU::RED_TEXT_DURATION;
}


/* ----------------------------------------------------------------------------
 * Starts the closing process.
 */
void onion_menu_struct::start_closing() {
    closing = true;
    closing_timer = GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME;
    gui.start_animation(
        GUI_MANAGER_ANIM_CENTER_TO_UP, GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
    );
    game.states.gameplay->hud->gui.start_animation(
        GUI_MANAGER_ANIM_OUT_TO_IN,
        GAMEPLAY::MENU_EXIT_HUD_MOVE_TIME
    );
}


/* ----------------------------------------------------------------------------
 * Ticks time by one frame of logic.
 * delta_t:
 *   How long the frame's tick is, in seconds.
 */
void onion_menu_struct::tick(const float delta_t) {

    //Correct the amount of wanted group members, if they are invalid.
    int total_delta = 0;
    
    for(size_t t = 0; t < n_ptr->nest_type->pik_types.size(); ++t) {
        //Get how many the player really has with them.
        int real_group_amount =
            (int) l_ptr->group->get_amount_by_type(
                n_ptr->nest_type->pik_types[t]
            );
            
        //Make sure the player can't request to store more than what they have.
        types[t].delta = std::max(-real_group_amount, (int) types[t].delta);
        
        //Get how many are really in the Onion.
        int real_onion_amount =
            (int) n_ptr->get_amount_by_type(n_ptr->nest_type->pik_types[t]);
            
        //Make sure the player can't request to call more than the Onion has.
        types[t].delta = std::min(real_onion_amount, (int) types[t].delta);
        
        //Calculate the total delta.
        total_delta += types[t].delta;
    }
    
    //Make sure the player can't request to have more than the field limit.
    int delta_over_limit =
        (int) game.states.gameplay->mobs.pikmin_list.size() +
        total_delta -
        (int) game.config.max_pikmin_in_field;
        
    while(delta_over_limit > 0) {
        vector<size_t> candidate_types;
        
        for(size_t t = 0; t < n_ptr->nest_type->pik_types.size(); ++t) {
            int real_group_amount =
                (int) l_ptr->group->get_amount_by_type(
                    n_ptr->nest_type->pik_types[t]
                );
                
            if((-types[t].delta) < real_group_amount) {
                //It's possible to take away from this type's delta request.
                candidate_types.push_back(t);
            }
        }
        
        //Figure out the type with the largest delta.
        size_t best_type = 0;
        int best_type_delta = types[candidate_types[0]].delta;
        for(size_t t = 1; t < candidate_types.size(); ++t) {
            if(types[candidate_types[t]].delta > best_type_delta) {
                best_type = candidate_types[t];
                best_type_delta = types[candidate_types[t]].delta;
            }
        }
        
        //Finally, remove one request from this type.
        types[candidate_types[best_type]].delta--;
        delta_over_limit--;
    }
    
    //Animate red text, if any.
    for(auto w = red_items.begin(); w != red_items.end();) {
        w->second -= delta_t;
        if(w->second <= 0.0f) {
            w = red_items.erase(w);
        } else {
            ++w;
        }
    }
    
    //Tick the GUI.
    gui.tick(delta_t);
    
    //Tick the background.
    const float bg_alpha_mult_speed =
        1.0f / GAMEPLAY::MENU_ENTRY_HUD_MOVE_TIME;
    const float diff =
        closing ? -bg_alpha_mult_speed : bg_alpha_mult_speed;
    bg_alpha_mult = clamp(bg_alpha_mult + diff * delta_t, 0.0f, 1.0f);
    
    //Tick the menu closing.
    if(closing) {
        closing_timer -= delta_t;
        if(closing_timer <= 0.0f) {
            to_delete = true;
        }
    }
}


/* ----------------------------------------------------------------------------
 * Toggles the "select all" mode.
 */
void onion_menu_struct::toggle_select_all() {
    select_all = !select_all;
    
    update();
}


/* ----------------------------------------------------------------------------
 * Updates some things about the Onion's state, especially caches.
 */
void onion_menu_struct::update() {
    //Reset the on-screen types.
    on_screen_types.clear();
    
    for(size_t t = 0; t < types.size(); ++t) {
        types[t].on_screen_idx = INVALID;
    }
    
    //Reset the button and amount states.
    for(size_t t = 0; t < ONION_MENU::TYPES_PER_PAGE; ++t) {
        onion_icon_items[t]->visible = false;
        onion_button_items[t]->visible = false;
        onion_button_items[t]->selectable = false;
        onion_amount_items[t]->visible = false;
        group_icon_items[t]->visible = false;
        group_button_items[t]->visible = false;
        group_button_items[t]->selectable = false;
        group_amount_items[t]->visible = false;
    }
    
    //Assign the on-screen types.
    for(
        size_t t = page * ONION_MENU::TYPES_PER_PAGE;
        t < (page + 1) * ONION_MENU::TYPES_PER_PAGE &&
        t < n_ptr->nest_type->pik_types.size();
        ++t
    ) {
        types[t].on_screen_idx = on_screen_types.size();
        on_screen_types.push_back(&types[t]);
    }
    
    //Assign the coordinates of the on-screen-type-related GUI items.
    float splits = on_screen_types.size() + 1;
    float leftmost = 0.50f;
    float rightmost = 0.50f;
    
    for(size_t t = 0; t < on_screen_types.size(); ++t) {
        float x = 1.0f / splits * (t + 1);
        onion_icon_items[t]->center.x = x;
        onion_button_items[t]->center.x = x;
        onion_amount_items[t]->center.x = x;
        group_icon_items[t]->center.x = x;
        group_button_items[t]->center.x = x;
        group_amount_items[t]->center.x = x;
        
        leftmost =
            std::min(
                leftmost,
                x - onion_button_items[t]->size.x / 2.0f
            );
        rightmost =
            std::max(
                rightmost,
                x + onion_button_items[t]->size.x / 2.0f
            );
    }
    
    //Make all relevant GUI items active.
    for(size_t t = 0; t < on_screen_types.size(); ++t) {
        onion_icon_items[t]->visible = true;
        onion_amount_items[t]->visible = true;
        group_icon_items[t]->visible = true;
        group_amount_items[t]->visible = true;
        if(!select_all) {
            onion_button_items[t]->visible = true;
            onion_button_items[t]->selectable = true;
            group_button_items[t]->visible = true;
            group_button_items[t]->selectable = true;
        }
    }
    
    if(nr_pages > 1) {
        leftmost =
            std::min(
                leftmost,
                onion_more_l_icon->center.x - onion_more_l_icon->size.x / 2.0f
            );
        rightmost =
            std::max(
                rightmost,
                onion_more_r_icon->center.x + onion_more_r_icon->size.x / 2.0f
            );
    }
    
    onion_more_l_icon->visible = nr_pages > 1 && select_all;
    onion_more_r_icon->visible = nr_pages > 1 && select_all;
    group_more_l_icon->visible = nr_pages > 1 && select_all;
    group_more_r_icon->visible = nr_pages > 1 && select_all;
    
    onion_all_button->size.x = rightmost - leftmost;
    group_all_button->size.x = rightmost - leftmost;
    
    onion_all_button->visible = select_all;
    onion_all_button->selectable = select_all;
    group_all_button->visible = select_all;
    group_all_button->selectable = select_all;
}


/* ----------------------------------------------------------------------------
 * Creates an Onion menu Pikmin type struct.
 * idx:
 *   Index of the Pikmin type in the nest object.
 * pik_type:
 *   The Pikmin type.
 */
onion_menu_type_struct::onion_menu_type_struct(
    const size_t idx, pikmin_type* pik_type
) :
    delta(0),
    type_idx(idx),
    on_screen_idx(INVALID),
    pik_type(pik_type) {
    
}
