#pragma once

#include "Camera.h"

using namespace std;
using namespace glm;

void Camera::_UpdateMatrices() {
  this->view_matrix = lookAt(this->pos, this->pos + this->direction, this->up);
  this->projection_view = projection_matrix * view_matrix;
  this->projection_view_inverse =
      inverse(view_matrix) * inverse(projection_matrix);
}
void Camera::_UpdateFrustum() {
  SausageDebug::GetScaledCameraFrustum(this, this->frustum, 1.0);
}
void Camera::UpdateCamera() {
  vec3 target = vec3(sin(radians(yaw_angle)) * cos(radians(pitch_angle)),
                     sin(radians(pitch_angle)),
                     -cos(radians(yaw_angle)) * cos(radians(pitch_angle)));
  direction = normalize(target);
  right = normalize(cross(target, world_up));
  up = normalize(cross(right, target));
  _UpdateMatrices();
  _UpdateFrustum();
}
void Camera::MouseWheelCallback(SDL_MouseWheelEvent &mw_event) {
  switch (camera_mode) {
  case RTS:
    MouseWheelCallbackRTS(mw_event);
    break;
  case FREE:
    MouseWheelCallbackFreeCam(mw_event);
    break;
  default:
    break;
  }
  this->pos.y -= (mw_event.y * CameraSettings::scroll_speed);

  SetUpdateMatrices();
}
void Camera::SetUpdateMatrices() { is_update_need = true; }
void Camera::SetPosition(vec3 &pos) {
  this->pos = pos;
  SetUpdateMatrices();
};
void Camera::MouseWheelCallbackRTS(SDL_MouseWheelEvent &mw_event) {
  this->pos.y -= (mw_event.y * CameraSettings::scroll_speed);

  SetUpdateMatrices();
};
void Camera::MouseWheelCallbackFreeCam(SDL_MouseWheelEvent &mw_event) {
  this->pos += direction;

  SetUpdateMatrices();
};
void Camera::PreUpdate(float delta_time) {
  this->velocity = delta_time * CameraSettings::movement_speed;
  this->delta_time = delta_time;
}
void Camera::Update() {
  if (is_update_need) {
    is_update_need = false;
    UpdateCamera();
  }
}
void Camera::KeyCallback(int scan_code) {
  if (scan_code == KeyboardLayout::ChangeCamera) {
    if (camera_mode == CameraMode::RTS) {
      camera_mode = CameraMode::FREE;
    } else {
      camera_mode = CameraMode::RTS;
    }
  } else {
    switch (camera_mode) {
    case RTS:
      KeyCallbackRTS(scan_code);
      break;
    case FREE:
      KeyCallbackFreeCam(scan_code);
      break;
    default:
      KeyCallbackRTS(scan_code);
      break;
    }
  }
}
void Camera::KeyCallbackRTS(int scan_code) {
  bool is_rts = true;
  _CameraMove(scan_code == KeyboardLayout::Left,
              scan_code == KeyboardLayout::Right,
              scan_code == KeyboardLayout::Up,
              scan_code == KeyboardLayout::Down, is_rts);
};
void Camera::KeyCallbackFreeCam(int scan_code) {
  bool is_rts = false;
  _CameraMove(scan_code == KeyboardLayout::Left,
              scan_code == KeyboardLayout::Right,
              scan_code == KeyboardLayout::Up,
              scan_code == KeyboardLayout::Down, is_rts);
};
void Camera::ResizeCallback(int new_width, int new_height) {
  this->width = new_width;
  this->height = new_height;
  this->projection_matrix =
      perspective(this->FOV_rad, (float)this->width / (float)this->height,
                  this->near_plane, this->far_plane);
  this->projection_matrix_ortho =
      ortho(0.0f, (float)this->width, 0.0f, (float)this->height);

  SetUpdateMatrices();
}
void Camera::MouseMotionCallback(float screen_x, float screen_y) {
  switch (camera_mode) {
  case RTS:
    MouseMotionCallbackRTS(screen_x, screen_y);
    break;
  case FREE:
    MouseMotionCallbackFreeCam(screen_x, screen_y);
    break;
  default:
    break;
  }
}
void Camera::MouseMotionCallbackRTS(float screen_x, float screen_y) {
  bool is_left = screen_x - screen_border <= 0;
  bool is_right = screen_x + 1 + screen_border >= GameSettings::SCR_WIDTH;
  bool is_up = screen_y + 1 + screen_border >= GameSettings::SCR_HEIGHT;
  bool is_down = screen_y - screen_border <= 0;
  bool is_rts = true;

  _CameraMove(is_left, is_right, is_up, is_down, is_rts,
              CameraSettings::mouse_motion_screen_border_velocity);
}
void Camera::MouseMotionCallbackFreeCam(float motion_x, float motion_y) {
  this->yaw_angle += (motion_x * CameraSettings::sensivity);
  this->pitch_angle -= (motion_y * CameraSettings::sensivity);
  this->pitch_angle = std::clamp(this->pitch_angle, -90.0f, 90.0f);
  if (yaw_angle > 360.0f || yaw_angle < -360.0f) {
    yaw_angle = 0.0f;
  }
  if (pitch_angle > 90.0f) {
    pitch_angle = 90.0f;
  }
  if (pitch_angle < -90.0f) {
    pitch_angle = -90.0f;
  }
  SetUpdateMatrices();
}

bool Camera::IsCursorOnWindowBorder(float screen_x, float screen_y) {
  return (screen_x + 1 + screen_border >= GameSettings::SCR_WIDTH) ||
         (screen_x - screen_border <= 0) ||
         (screen_y + 1 + screen_border >= GameSettings::SCR_HEIGHT) ||
         (screen_y - screen_border <= 0);
}
void Camera::_CameraMove(bool is_left, bool is_right, bool is_up, bool is_down,
                         bool is_rts, float velocity_delta) {
  if (is_right) {
    auto delta = right * velocity * velocity_delta;
    pos += delta;
  } else if (is_left) {
    auto delta = right * velocity * velocity_delta;
    pos -= delta;
  }
  if (is_up) {
    auto delta = direction * velocity * velocity_delta;
    if (is_rts) {
      delta.y = 0;
    }
    pos += delta;
  } else if (is_down) {
    auto delta = direction * velocity * velocity_delta;
    if (is_rts) {
      delta.y = 0;
    }
    pos -= delta;
  }
  SetUpdateMatrices();
}
