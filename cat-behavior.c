#include "cat-behavior.h"
#include <stdlib.h>
#include <stdio.h>

// Cross-platform mouse position helper
static void get_global_mouse_position(float *x, float *y) {
#ifdef _WIN32
    POINT pt;
    if (GetCursorPos(&pt)) {
        *x = (float)pt.x;
        *y = (float)pt.y;
    } else {
        *x = *y = -1.0f; // Invalid position
    }
#elif __APPLE__
    // macOS implementation would use CGEventSource
    *x = *y = -1.0f;
#else // Linux/X11
    // Linux implementation would use XQueryPointer
    *x = *y = -1.0f;
#endif
}

void cat_behavior_init(cat_behavior_t *cat, uint32_t screen_width, uint32_t screen_height) {
    memset(cat, 0, sizeof(cat_behavior_t));
    
    cat->screen_width = (float)screen_width;
    cat->screen_height = (float)screen_height;
    
    // Start in middle of screen
    cat->x = cat->screen_width * 0.5f;
    cat->y = cat->screen_height * 0.5f;
    
    // Default behavior parameters
    cat->base_speed = 30.0f;          // pixels per second
    cat->retreat_speed_multiplier = 3.0f;
    cat->mouse_avoidance_radius = 150.0f;
    cat->examine_duration_min = 1.5f;
    cat->examine_duration_max = 4.0f;
    cat->wander_change_interval = 3.0f;
    
    cat->state = CAT_STATE_WANDERING;
    cat->state_start_time = time(NULL);
    
    // Initial random direction
    float angle = (float)rand() / RAND_MAX * 2.0f * (float)M_PI;
    cat->vx = cosf(angle) * cat->base_speed;
    cat->vy = sinf(angle) * cat->base_speed;
}

void cat_behavior_set_mouse_position(cat_behavior_t *cat, float x, float y) {
    cat->mouse_x = x;
    cat->mouse_y = y;
    
    // Check if mouse is nearby (within avoidance radius)
    float dx = cat->x - cat->mouse_x;
    float dy = cat->y - cat->mouse_y;
    float distance = sqrtf(dx*dx + dy*dy);
    cat->mouse_nearby = (distance < cat->mouse_avoidance_radius && cat->mouse_x >= 0);
}

static float randf(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

static void start_examining(cat_behavior_t *cat) {
    cat->state = CAT_STATE_EXAMINING;
    cat->state_start_time = time(NULL);
    
    // Pick a random point on screen to "examine"
    cat->target_x = randf(0, cat->screen_width);
    cat->target_y = randf(0, cat->screen_height);
    
    // Stop movement
    cat->vx = cat->vy = 0;
}

static void start_wandering(cat_behavior_t *cat) {
    cat->state = CAT_STATE_WANDERING;
    cat->state_start_time = time(NULL);
    
    // New random direction
    float angle = randf(0, 2.0f * (float)M_PI);
    cat->vx = cosf(angle) * cat->base_speed;
    cat->vy = sinf(angle) * cat->base_speed;
}

static void start_retreat(cat_behavior_t *cat) {
    cat->state = CAT_STATE_RETREATING;
    cat->state_start_time = time(NULL);
    
    // Calculate direction away from mouse
    float dx = cat->x - cat->mouse_x;
    float dy = cat->y - cat->mouse_y;
    float distance = sqrtf(dx*dx + dy*dy);
    
    if (distance > 1.0f) {
        // Normalize and apply retreat speed
        float speed = cat->base_speed * cat->retreat_speed_multiplier;
        cat->vx = (dx / distance) * speed;
        cat->vy = (dy / distance) * speed;
    } else {
        // Default retreat direction if too close
        cat->vx = cat->base_speed * cat->retreat_speed_multiplier;
        cat->vy = 0;
    }
}

void cat_behavior_tick(cat_behavior_t *cat, float seconds) {
    time_t current_time = time(NULL);
    float elapsed = (float)(current_time - cat->state_start_time);
    
    // State transitions
    switch (cat->state) {
        case CAT_STATE_WANDERING:
            // Occasionally stop to examine something
            if (elapsed > cat->wander_change_interval && randf(0, 1) < 0.3f) {
                start_examining(cat);
                break;
            }
            
            // Check if mouse is nearby
            if (cat->mouse_nearby) {
                start_retreat(cat);
                break;
            }
            break;
            
        case CAT_STATE_EXAMINING:
            // Finish examining after random duration
            float examine_duration = randf(cat->examine_duration_min, cat->examine_duration_max);
            if (elapsed > examine_duration) {
                start_wandering(cat);
                break;
            }
            
            // Interrupt examining if mouse gets too close
            if (cat->mouse_nearby) {
                start_retreat(cat);
                break;
            }
            break;
            
        case CAT_STATE_RETREATING:
            // Stop retreating when mouse is no longer nearby
            if (!cat->mouse_nearby) {
                // 70% chance to go back to wandering, 30% to examine something after retreating
                if (randf(0, 1) < 0.7f) {
                    start_wandering(cat);
                } else {
                    start_examining(cat);
                }
                break;
            }
            break;
    }
    
    // Update position based on velocity
    cat->x += cat->vx * seconds;
    cat->y += cat->vy * seconds;
    
    // Boundary collision - bounce with energy loss
    bool bounced = false;
    if (cat->x < 0) {
        cat->x = 0;
        cat->vx = -cat->vx * 0.7f;
        bounced = true;
    } else if (cat->x > cat->screen_width - cat->texture_width) {
        cat->x = cat->screen_width - cat->texture_width;
        cat->vx = -cat->vx * 0.7f;
        bounced = true;
    }
    
    if (cat->y < 0) {
        cat->y = 0;
        cat->vy = -cat->vy * 0.7f;
        bounced = true;
    } else if (cat->y > cat->screen_height - cat->texture_height) {
        cat->y = cat->screen_height - cat->texture_height;
        cat->vy = -cat->vy * 0.7f;
        bounced = true;
    }
    
    // After bouncing, sometimes change direction naturally
    if (bounced && cat->state == CAT_STATE_WANDERING && randf(0, 1) < 0.4f) {
        float angle = randf(0, 2.0f * (float)M_PI);
        cat->vx = cosf(angle) * cat->base_speed;
        cat->vy = sinf(angle) * cat->base_speed;
    }
}

bool cat_behavior_load_texture(cat_behavior_t *cat, const char *file_path) {
    if (cat->texture) {
        gs_texture_destroy(cat->texture);
        cat->texture = NULL;
    }
    
    if (!file_path || strlen(file_path) == 0) {
        return false;
    }
    
    cat->texture = gs_texture_create_from_file(file_path);
    if (!cat->texture) {
        return false;
    }
    
    cat->texture_width = gs_texture_get_width(cat->texture);
    cat->texture_height = gs_texture_get_height(cat->texture);
    return true;
}

void cat_behavior_destroy(cat_behavior_t *cat) {
    if (cat->texture) {
        gs_texture_destroy(cat->texture);
        cat->texture = NULL;
    }
}

void cat_behavior_get_position(const cat_behavior_t *cat, float *x, float *y) {
    if (x) *x = cat->x;
    if (y) *y = cat->y;
}
