#ifndef VW_SETTINGS_GLOBAL_H
#define VW_SETTINGS_GLOBAL_H
#include "glm/glm.hpp"
namespace GameSettings {
  inline int SCR_WIDTH = 800;
  inline int SCR_HEIGHT = 800;
  //int SCR_WIDTH = 1920;
  //int SCR_HEIGHT = 1080;

  inline int ray_debug_draw_lifetime_milliseconds = 2000;

  inline float physics_step_multiplier = 0.001f;
  //inline float physics_step_multiplier = 0.01f;

  inline float cam_speed = 1.0f;
  inline bool quit = false;

  inline float MAX_DRAW_DISTANCE = 1000.0;

  inline const glm::vec3 world_right = { 1,0,0 };
  inline const glm::vec3 world_up = { 0,1,0 };
  inline const glm::vec3 world_forward = { 0,0,-1 };
};
namespace CameraSettings {
	inline float sensivity = 0.1f;
	inline float movement_speed = 0.1f;
	inline float scroll_speed = 1.0f;
	inline float mouse_motion_screen_border_velocity = 0.1f;
};
#endif
