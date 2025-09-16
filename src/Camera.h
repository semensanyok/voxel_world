#pragma once

#include "sausage.h"
#include "Logging.h"
#include "Settings.h"
#include "Structures.h"
#include "Interfaces.h"
#include "Frustum.h"
#include <SDL.h>

using namespace std;
using namespace glm;

enum CameraMode {
  RTS,
  FREE
};

class Camera : public SausageSystem
{
private:
  friend class Gui;
  friend class Renderer;
  friend class Controller;
  friend class SystemsManager;
  friend class ShaderManager;

  mat4 view_matrix;
  mat4 projection_matrix;
  mat4 projection_matrix_ortho;
  mat4 projection_view;
  mat4 projection_view_inverse;
  vec3 world_up;
  float yaw_angle;
  float pitch_angle;

  int screen_border = 20;

  // per frame variables
  float velocity;
  float delta_time;

public:
  float aspect;
  float near_plane;
  float far_plane;
  float FOV_rad;
  float width;
  float height;

  Frustum* frustum;
  vec3 pos;
  vec3 direction;
  vec3 right;
  vec3 up;

  bool is_update_need = false;
  CameraMode camera_mode = CameraMode::RTS;

  Camera(float FOV_deg,
    float width,
    float height,
    float near_plane,
    float far_plane,
    vec3 pos,
    float yaw_angle = 0.0f,
    float pitch_angle = 0.0f,
    vec3 up = vec3(0.0f, 1.0f, 0.0f),
    vec3 world_up = vec3(0.0f, 1.0f, 0.0f)
  ) {
    this->pos = pos;
    this->FOV_rad = radians(FOV_deg);
    this->width = width;
    this->height = height;
    this->near_plane = near_plane;
    this->far_plane = far_plane;
    this->yaw_angle = yaw_angle;
    this->pitch_angle = pitch_angle;
    this->world_up = world_up;
    this->up = up;
    this->aspect = (float)this->width / (float)this->height;
    this->projection_matrix = perspective(this->FOV_rad, this->aspect, this->near_plane, this->far_plane);
    this->projection_matrix_ortho = ortho(0.0f, (float)this->width, 0.0f, (float)this->height);
    this->frustum = new Frustum();
    UpdateCamera();
  };
  ~Camera() {};
  void UpdateCamera();
  void MouseWheelCallback(SDL_MouseWheelEvent& mw_event);
  void SetUpdateMatrices();
  void SetPosition(vec3& pos);
  void MouseWheelCallbackRTS(SDL_MouseWheelEvent& mw_event);
  void MouseWheelCallbackFreeCam(SDL_MouseWheelEvent& mw_event);
  void PreUpdate(float delta_time);
  void Update();
  void KeyCallbackRTS(int scan_code);
  void KeyCallbackFreeCam(int scan_code);
  void ResizeCallback(int new_width, int new_height);
  void MouseMotionCallback(float screen_x, float screen_y);
  void MouseMotionCallbackRTS(float screen_x, float screen_y);
  void MouseMotionCallbackFreeCam(float motion_x, float motion_y);
  void KeyCallback(int scan_code);
  bool IsCursorOnWindowBorder(float screen_x, float screen_y);
private:
  void _UpdateMatrices();
  void _UpdateFrustum();
  void _CameraMove(bool is_left, bool is_right, bool is_up, bool is_down, bool is_rts, float velocity_delta = 1.0f);
};
