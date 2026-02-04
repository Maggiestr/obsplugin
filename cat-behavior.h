#pragma once

#include <obs.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Cat behavior states
typedef enum {
    CAT_STATE_WANDERING,
    CAT_STATE_EXAMINING,
    CAT_STATE_RETREATING
} cat_state_t;

// Cat physics and behavior parameters
typedef struct {
    // Position and velocity
    float x, y;
    float vx, vy;
    float screen_width, screen_height;
    
    // Behavior state
    cat_state_t state;
    time_t state_start_time;
    float target_x, target_y;  // For examining behavior
    
    // Configuration parameters
    float base_speed;
    float retreat_speed_multiplier;
    float mouse_avoidance_radius;
    float examine_duration_min;
    float examine_duration_max;
    float wander_change_interval;
    
    // Mouse tracking
    float mouse_x, mouse_y;
    bool mouse_nearby;
    
    // Texture
    gs_texture_t *texture;
    uint32_t texture_width;
    uint32_t texture_height;
} cat_behavior_t;

// Initialize cat behavior system
void cat_behavior_init(cat_behavior_t *cat, uint32_t screen_width, uint32_t screen_height);

// Update cat position and state (call every frame)
void cat_behavior_tick(cat_behavior_t *cat, float seconds);

// Set current mouse position (in screen coordinates)
void cat_behavior_set_mouse_position(cat_behavior_t *cat, float x, float y);

// Load cat texture from file path
bool cat_behavior_load_texture(cat_behavior_t *cat, const char *file_path);

// Release resources
void cat_behavior_destroy(cat_behavior_t *cat);

// Get current cat position
void cat_behavior_get_position(const cat_behavior_t *cat, float *x, float *y);
