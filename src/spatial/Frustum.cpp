#include "Frustum.h"
#include "Camera.h"
#include "SystemsManager.h"

using namespace SausageDebug;

void SausageDebug::GetScaledCameraFrustum(Camera *c, Frustum *out_frustum,
                                          float scale) {
  const float half_far_width = c->far_plane * scale * tanf(c->FOV_rad * .5f);
  const float half_far_height = half_far_width * c->aspect;
  const glm::vec3 far_center = c->pos + c->far_plane * scale * c->direction;

  const float half_near_width = c->near_plane * scale * tanf(c->FOV_rad * .5f);
  const float half_near_height = half_near_width * c->aspect;
  const glm::vec3 near_center = c->pos + c->near_plane * scale * c->direction;

  auto cam_dist_from_origin = glm::length(c->pos);

  // WRITE FRUSTUM
  out_frustum->near = {glm::length(near_center), c->direction};
  out_frustum->far = {glm::length(far_center), -c->direction};

  {
    vec3 pl1 = near_center, pl2 = near_center, pl3 = far_center;
    pl1 += pl1 - c->right * half_near_width;
    pl2 += pl1 + c->up * half_near_height;
    pl3 += pl3 - c->right * half_far_width;
    vec3 v1 = pl1 - pl2, v2 = pl1 - pl3;
    out_frustum->left = {cam_dist_from_origin, normalize(cross(v1, v2))};
  }

  {
    vec3 pl1 = near_center, pl2 = near_center, pl3 = far_center;
    pl1 += pl1 + c->right * half_near_width;
    pl2 += pl1 + c->up * half_near_height;
    pl3 += pl3 + c->right * half_far_width;
    vec3 v1 = pl1 - pl2, v2 = pl1 - pl3;
    out_frustum->right = {cam_dist_from_origin, normalize(cross(v1, v2))};
  }

  {
    vec3 pl1 = near_center, pl2 = near_center, pl3 = far_center;
    pl1 += pl1 + c->up * half_near_height;
    pl2 += pl1 + c->right * half_near_width;
    pl3 += pl3 + c->up * half_far_height;
    vec3 v1 = pl1 - pl2, v2 = pl1 - pl3;
    out_frustum->up = {cam_dist_from_origin, normalize(cross(v2, v1))};
  }

  {
    vec3 pl1 = near_center, pl2 = near_center, pl3 = far_center;
    pl1 += pl1 - c->up * half_near_height;
    pl2 += pl1 + c->right * half_near_width;
    pl3 += pl3 - c->up * half_far_height;
    vec3 v1 = pl1 - pl2, v2 = pl1 - pl3;
    out_frustum->down = {cam_dist_from_origin, normalize(cross(v1, v2))};
  }
}

FrustumVertices SausageDebug::GetScaledCameraFrustumVertices(float scale) {
  FrustumVertices out_verts;

  auto c = SystemsManager::GetInstance()->camera;
  const float half_far_width = c->far_plane * scale * tanf(c->FOV_rad * .5f);
  const float half_far_height = half_far_width * c->aspect;
  const glm::vec3 far_center = c->pos + c->far_plane * scale * c->direction;

  const float half_near_width = c->near_plane * scale * tanf(c->FOV_rad * .5f);
  const float half_near_height = half_near_width * c->aspect;
  const glm::vec3 near_center = c->pos + c->near_plane * scale * c->direction;

  auto cam_dist_from_origin = glm::length(c->pos);

  // WRITE DEBUG VERTICES
  out_verts.vertices = {
      near_center - c->right * half_near_width + c->up * half_near_height,
      near_center + c->right * half_near_width + c->up * half_near_height,
      near_center - c->right * half_near_width - c->up * half_near_height,
      near_center + c->right * half_near_width - c->up * half_near_height,

      far_center - c->right * half_far_width + c->up * half_far_height,
      far_center + c->right * half_far_width + c->up * half_far_height,
      far_center - c->right * half_far_width - c->up * half_far_height,
      far_center + c->right * half_far_width - c->up * half_far_height,
  };

  //// GL_LINES: Vertices 0 and 1 are considered a line. Vertices 2 and 3 are
  /// considered a line. And so on. If the user specifies a non-even number of
  /// vertices, then the extra vertex is ignored.
  out_verts.indices = {0, 1, 1, 3, 3, 2, 2, 0,

                       0, 4, 1, 5, 2, 6, 3, 7,

                       4, 5, 5, 7, 7, 6, 6, 4};

  return out_verts;
}
