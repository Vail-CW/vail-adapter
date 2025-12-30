/*
 * VAIL SUMMIT - LVGL Screen Manager
 * Handles screen transitions, input groups, and global ESC navigation
 */

#ifndef LV_SCREEN_MANAGER_H
#define LV_SCREEN_MANAGER_H

#include <lvgl.h>
#include "lv_init.h"
#include "lv_theme_manager.h"

// ============================================
// Screen Stack for Back Navigation
// ============================================

#define MAX_SCREEN_STACK 10

// Screen stack for back navigation
static lv_obj_t* screen_stack[MAX_SCREEN_STACK];
static int screen_stack_index = -1;

// Current active screen
static lv_obj_t* current_screen = NULL;

// Callback type for back button action
typedef void (*BackActionCallback)(void);
static BackActionCallback global_back_callback = NULL;

// ============================================
// Screen Transition Animations
// ============================================

typedef enum {
    SCREEN_ANIM_NONE = 0,
    SCREEN_ANIM_FADE,
    SCREEN_ANIM_SLIDE_LEFT,
    SCREEN_ANIM_SLIDE_RIGHT,
    SCREEN_ANIM_SLIDE_UP,
    SCREEN_ANIM_SLIDE_DOWN
} ScreenAnimType;

// Default transition duration in ms
#define DEFAULT_TRANSITION_MS 150

// ============================================
// Global ESC Handler
// ============================================

/*
 * Global ESC key event handler
 * Attached to all navigable widgets
 */
static void global_esc_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_KEY) {
        // Check if event was already handled by a screen-specific handler
        // In LVGL v8.3, check the stop_bubbling flag directly
        if (e->stop_bubbling) {
            return;
        }

        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) {
            Serial.println("[ScreenManager] ESC pressed");

            // Call the global back callback if set
            if (global_back_callback != NULL) {
                global_back_callback();
            }
        }
    }
}

/*
 * Set the global back navigation callback
 */
void setBackCallback(BackActionCallback callback) {
    global_back_callback = callback;
}

// ============================================
// Input Group Management
// ============================================

/*
 * Add a widget to the navigation group with ESC handler
 * This is the primary way to make widgets navigable
 */
void addNavigableWidget(lv_obj_t* widget) {
    lv_group_t* group = getLVGLInputGroup();
    if (group == NULL) {
        Serial.println("[ScreenManager] WARNING: No input group available!");
        return;
    }

    // Add to input group for keyboard navigation
    lv_group_add_obj(group, widget);

    // Add ESC handler for back navigation
    lv_obj_add_event_cb(widget, global_esc_handler, LV_EVENT_KEY, NULL);

    Serial.printf("[ScreenManager] Added widget to nav group, now has %d objects\n",
                  lv_group_get_obj_count(group));
}

/*
 * Remove a widget from the navigation group
 */
void removeNavigableWidget(lv_obj_t* widget) {
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_remove_obj(widget);
    }
}

/*
 * Clear all widgets from the navigation group
 */
void clearNavigationGroup() {
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_remove_all_objs(group);
    }
}

/*
 * Focus a specific widget
 */
void focusWidget(lv_obj_t* widget) {
    lv_group_t* group = getLVGLInputGroup();
    if (group != NULL) {
        lv_group_focus_obj(widget);
    }
}

// ============================================
// Screen Management
// ============================================

/*
 * Create a new screen with default styling
 */
lv_obj_t* createScreen() {
    lv_obj_t* screen = lv_obj_create(NULL);

    // Apply default background color from active theme
    lv_obj_set_style_bg_color(screen, getThemeColors()->bg_deep, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    return screen;
}

/*
 * Push current screen to stack and load new screen
 * NOTE: Do NOT clear the navigation group here - widgets are added during
 * screen creation and would be lost.
 */
void pushScreen(lv_obj_t* new_screen, ScreenAnimType anim = SCREEN_ANIM_FADE) {
    // Push current screen to stack (if any)
    if (current_screen != NULL && screen_stack_index < MAX_SCREEN_STACK - 1) {
        screen_stack_index++;
        screen_stack[screen_stack_index] = current_screen;
    }

    // Load new screen with animation
    lv_scr_load_anim_t lv_anim = LV_SCR_LOAD_ANIM_FADE_ON;
    switch (anim) {
        case SCREEN_ANIM_NONE:
            lv_anim = LV_SCR_LOAD_ANIM_NONE;
            break;
        case SCREEN_ANIM_FADE:
            lv_anim = LV_SCR_LOAD_ANIM_FADE_ON;
            break;
        case SCREEN_ANIM_SLIDE_LEFT:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_LEFT;
            break;
        case SCREEN_ANIM_SLIDE_RIGHT:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
            break;
        case SCREEN_ANIM_SLIDE_UP:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_TOP;
            break;
        case SCREEN_ANIM_SLIDE_DOWN:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_BOTTOM;
            break;
    }

    lv_scr_load_anim(new_screen, lv_anim, DEFAULT_TRANSITION_MS, 0, false);
    current_screen = new_screen;

    Serial.printf("[ScreenManager] Pushed screen, stack depth: %d\n", screen_stack_index + 1);
}

/*
 * Pop from stack and return to previous screen
 */
bool popScreen(ScreenAnimType anim = SCREEN_ANIM_FADE) {
    if (screen_stack_index < 0) {
        Serial.println("[ScreenManager] Cannot pop - stack empty");
        return false;
    }

    // Get previous screen from stack
    lv_obj_t* prev_screen = screen_stack[screen_stack_index];
    screen_stack_index--;

    // Clear navigation group
    clearNavigationGroup();

    // Load previous screen with reverse animation
    lv_scr_load_anim_t lv_anim = LV_SCR_LOAD_ANIM_FADE_ON;
    switch (anim) {
        case SCREEN_ANIM_SLIDE_LEFT:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;  // Reverse
            break;
        case SCREEN_ANIM_SLIDE_RIGHT:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_LEFT;   // Reverse
            break;
        default:
            lv_anim = LV_SCR_LOAD_ANIM_FADE_ON;
            break;
    }

    // Delete current screen after animation, load previous
    lv_scr_load_anim(prev_screen, lv_anim, DEFAULT_TRANSITION_MS, 0, true);
    current_screen = prev_screen;

    Serial.printf("[ScreenManager] Popped screen, stack depth: %d\n", screen_stack_index + 1);
    return true;
}

/*
 * Load a screen directly (replacing current, not using stack)
 * NOTE: Do NOT clear the navigation group here - widgets are added during
 * screen creation and would be lost. The group is managed by screen creators.
 */
void loadScreen(lv_obj_t* new_screen, ScreenAnimType anim = SCREEN_ANIM_FADE) {
    // Determine animation type
    lv_scr_load_anim_t lv_anim = LV_SCR_LOAD_ANIM_FADE_ON;
    switch (anim) {
        case SCREEN_ANIM_NONE:
            lv_anim = LV_SCR_LOAD_ANIM_NONE;
            break;
        case SCREEN_ANIM_FADE:
            lv_anim = LV_SCR_LOAD_ANIM_FADE_ON;
            break;
        case SCREEN_ANIM_SLIDE_LEFT:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_LEFT;
            break;
        case SCREEN_ANIM_SLIDE_RIGHT:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
            break;
        case SCREEN_ANIM_SLIDE_UP:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_TOP;
            break;
        case SCREEN_ANIM_SLIDE_DOWN:
            lv_anim = LV_SCR_LOAD_ANIM_MOVE_BOTTOM;
            break;
    }

    // Delete old screen after transition
    lv_scr_load_anim(new_screen, lv_anim, DEFAULT_TRANSITION_MS, 0, true);
    current_screen = new_screen;

    lv_group_t* group = getLVGLInputGroup();
    Serial.printf("[ScreenManager] Loaded screen (direct), nav group has %d objects\n",
                  group ? lv_group_get_obj_count(group) : -1);
}

/*
 * Get the current active screen
 */
lv_obj_t* getCurrentScreen() {
    return current_screen;
}

/*
 * Clear the screen stack
 */
void clearScreenStack() {
    // Delete all screens in stack
    for (int i = screen_stack_index; i >= 0; i--) {
        if (screen_stack[i] != NULL) {
            lv_obj_del(screen_stack[i]);
            screen_stack[i] = NULL;
        }
    }
    screen_stack_index = -1;
}

/*
 * Get screen stack depth
 */
int getScreenStackDepth() {
    return screen_stack_index + 1;
}

// ============================================
// Modal Dialogs
// ============================================

/*
 * Show a modal message box
 */
lv_obj_t* showMessageBox(const char* title, const char* message, const char* buttons[], int num_buttons) {
    lv_obj_t* mbox = lv_msgbox_create(NULL, title, message, buttons, false);
    lv_obj_center(mbox);

    // Add the buttons to navigation group
    lv_obj_t* btns = lv_msgbox_get_btns(mbox);
    if (btns != NULL) {
        addNavigableWidget(btns);
    }

    return mbox;
}

/*
 * Close a message box
 */
void closeMessageBox(lv_obj_t* mbox) {
    lv_msgbox_close(mbox);
}

// ============================================
// Utility Functions
// ============================================

/*
 * Add all children of a container to the navigation group
 */
void addContainerChildrenToNav(lv_obj_t* container) {
    uint32_t count = lv_obj_get_child_cnt(container);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t* child = lv_obj_get_child(container, i);
        // Only add focusable widgets
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) {
            addNavigableWidget(child);
        }
    }
}

/*
 * Refresh screen (force redraw)
 */
void refreshScreen() {
    lv_obj_invalidate(lv_scr_act());
}

#endif // LV_SCREEN_MANAGER_H
