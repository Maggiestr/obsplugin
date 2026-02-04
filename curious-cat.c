#include <obs-module.h>
#include <util/platform.h>
#include "cat-behavior.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("curious-cat", "en-US")

// Source data structure
struct curious_cat_source {
    obs_source_t *context;
    cat_behavior_t cat;
    char *image_path;
    
    // Settings
    float base_speed;
    float avoidance_radius;
    float examine_min;
    float examine_max;
};

// Get global mouse position relative to current display
static void update_mouse_position(struct curious_cat_source *src) {
    float mx, my;
    get_global_mouse_position(&mx, &my);
    
    // Convert to current output coordinates if needed
    // For simplicity, we assume single display setup
    cat_behavior_set_mouse_position(&src->cat, mx, my);
}

static const char *curious_cat_get_name(void *unused) {
    UNUSED_PARAMETER(unused);
    return obs_module_text("CuriousCat");
}

static void *curious_cat_create(obs_data_t *settings, obs_source_t *source) {
    struct curious_cat_source *src = bzalloc(sizeof(struct curious_cat_source));
    src->context = source;
    
    // Initialize with default screen size (will update on first render)
    uint32_t width = 1920;
    uint32_t height = 1080;
    cat_behavior_init(&src->cat, width, height);
    
    // Load settings
    src->image_path = bstrdup(obs_data_get_string(settings, "image_path"));
    src->base_speed = (float)obs_data_get_double(settings, "base_speed");
    src->avoidance_radius = (float)obs_data_get_double(settings, "avoidance_radius");
    src->examine_min = (float)obs_data_get_double(settings, "examine_min");
    src->examine_max = (float)obs_data_get_double(settings, "examine_max");
    
    // Apply settings to cat behavior
    src->cat.base_speed = src->base_speed;
    src->cat.mouse_avoidance_radius = src->avoidance_radius;
    src->cat.examine_duration_min = src->examine_min;
    src->cat.examine_duration_max = src->examine_max;
    
    // Load texture
    if (!src->image_path || strlen(src->image_path) == 0) {
        // Use default image from plugin data path
        char *data_path = obs_module_get_data_path(obs_current_module());
        char default_path[512];
        snprintf(default_path, sizeof(default_path), "%s%sdefault-cat.png", 
                 data_path, strlen(data_path) && data_path[strlen(data_path)-1] == '/' ? "" : "/");
        bfree(data_path);
        
        src->image_path = bstrdup(default_path);
    }
    
    cat_behavior_load_texture(&src->cat, src->image_path);
    
    return src;
}

static void curious_cat_destroy(void *data) {
    struct curious_cat_source *src = data;
    
    cat_behavior_destroy(&src->cat);
    
    if (src->image_path) {
        bfree(src->image_path);
    }
    
    bfree(src);
}

static void curious_cat_update(void *data, obs_data_t *settings) {
    struct curious_cat_source *src = data;
    
    // Update settings
    const char *new_path = obs_data_get_string(settings, "image_path");
    if (!src->image_path || strcmp(src->image_path, new_path) != 0) {
        if (src->image_path) {
            bfree(src->image_path);
        }
        src->image_path = bstrdup(new_path);
        cat_behavior_load_texture(&src->cat, src->image_path);
    }
    
    src->base_speed = (float)obs_data_get_double(settings, "base_speed");
    src->avoidance_radius = (float)obs_data_get_double(settings, "avoidance_radius");
    src->examine_min = (float)obs_data_get_double(settings, "examine_min");
    src->examine_max = (float)obs_data_get_double(settings, "examine_max");
    
    // Apply to behavior system
    src->cat.base_speed = src->base_speed;
    src->cat.mouse_avoidance_radius = src->avoidance_radius;
    src->cat.examine_duration_min = src->examine_min;
    src->cat.examine_duration_max = src->examine_max;
}

static void curious_cat_tick(void *data, float seconds) {
    struct curious_cat_source *src = data;
    
    // Update mouse position (global tracking)
    update_mouse_position(src);
    
    // Update cat physics and behavior
    cat_behavior_tick(&src->cat, seconds);
}

static void curious_cat_render(void *data, gs_effect_t *effect) {
    struct curious_cat_source *src = data;
    
    // Update screen dimensions based on current output
    uint32_t width = obs_source_get_base_width(obs_get_output_source(0));
    uint32_t height = obs_source_get_base_height(obs_get_output_source(0));
    
    if (width > 0 && height > 0) {
        src->cat.screen_width = (float)width;
        src->cat.screen_height = (float)height;
    }
    
    if (!src->cat.texture) {
        return;
    }
    
    // Get current cat position
    float x, y;
    cat_behavior_get_position(&src->cat, &x, &y);
    
    // Set up the effect
    while (gs_effect_loop(effect, "Draw")) {
        gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), src->cat.texture);
        gs_effect_set_float(gs_effect_get_param_by_name(effect, "opacity"), 1.0f);
        
        // Draw the texture at cat position
        gs_matrix_push();
        gs_matrix_translate3f(x, y, 0.0f);
        gs_draw_sprite(src->cat.texture, 0, 0, 0);
        gs_matrix_pop();
    }
}

static void curious_cat_get_defaults(obs_data_t *settings) {
    // Default image path (will be replaced with actual path at runtime)
    obs_data_set_default_string(settings, "image_path", "");
    
    // Default behavior parameters
    obs_data_set_default_double(settings, "base_speed", 40.0);
    obs_data_set_default_double(settings, "avoidance_radius", 180.0);
    obs_data_set_default_double(settings, "examine_min", 1.5);
    obs_data_set_default_double(settings, "examine_max", 4.0);
}

static obs_properties_t *curious_cat_get_properties(void *data) {
    UNUSED_PARAMETER(data);
    
    obs_properties_t *props = obs_properties_create();
    
    // Image selection
    obs_properties_add_path(props, "image_path", obs_module_text("CatImage"),
                          OBS_PATH_FILE, "PNG Files (*.png)", NULL);
    
    // Behavior controls
    obs_properties_add_float_slider(props, "base_speed", obs_module_text("BaseSpeed"),
                                  10.0, 100.0, 1.0);
    obs_properties_add_float_slider(props, "avoidance_radius", obs_module_text("AvoidanceRadius"),
                                  50.0, 400.0, 5.0);
    obs_properties_add_float_slider(props, "examine_min", obs_module_text("ExamineMinDuration"),
                                  0.5, 5.0, 0.1);
    obs_properties_add_float_slider(props, "examine_max", obs_module_text("ExamineMaxDuration"),
                                  1.0, 10.0, 0.1);
    
    return props;
}

static struct obs_source_info curious_cat_info = {
    .id             = "curious_cat_source",
    .type           = OBS_SOURCE_TYPE_INPUT,
    .output_flags   = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
    .get_name       = curious_cat_get_name,
    .create         = curious_cat_create,
    .destroy        = curious_cat_destroy,
    .update         = curious_cat_update,
    .video_tick     = curious_cat_tick,
    .video_render   = curious_cat_render,
    .get_defaults   = curious_cat_get_defaults,
    .get_properties = curious_cat_get_properties,
    .icon_type      = OBS_ICON_TYPE_IMAGE,
};

bool obs_module_load(void) {
    srand((unsigned int)time(NULL));
    obs_register_source(&curious_cat_info);
    blog(LOG_INFO, "Curious Cat plugin loaded successfully");
    return true;
}

void obs_module_unload(void) {
    blog(LOG_INFO, "Curious Cat plugin unloaded");
}
