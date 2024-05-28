#ifndef VW_SYSTEMS_MANAGER
#define VW_SYSTEMS_MANAGER

#include "settings_global.h"
#include "Settings.h"
#include "Settings.cpp"
#include "Renderer.h"
#include "RendererContextManager.h"
#include "Camera.h"
#include "ImguiGui.h"
#include "Controller.h"
#include "TextureManager.h"
#include "FontManager.h"
#include "physics.h"
#include "AsyncTaskManager.h"
#include "Animation.h"
#include "StateManager.h"
#include "MouseKeyboardEventProcessor.h"
#include "Logging.h"
#include "FileWatcher.h"
#include "BufferManager.h"
#include "ScreenOverlayManager.h"
#include "ShaderManager.h"
#include "TerrainManager.h"
#include "DrawCallManager.h"
#include "MeshDataUtils.h"
#include "SpatialManager.h"

class SystemsManager
{
  friend class SausageTestBase;
public:
  MeshManager* mesh_manager = nullptr;
  Camera* camera = nullptr;
  Controller* controller = nullptr;
  Renderer* renderer = nullptr;
  RendererContextManager* renderer_context_manager = nullptr;
  Samplers* samplers = nullptr;
  TextureManager* texture_manager = nullptr;
  FontManager* font_manager = nullptr;
  FileWatcher* file_watcher = nullptr;
  AsyncTaskManager* async_manager = nullptr;
  AnimationManager* anim_manager = nullptr;
  PhysicsManager* physics_manager = nullptr;
  StateManager* state_manager = nullptr;
  BulletDebugDrawer* bullet_debug_drawer = nullptr;
  ControllerEventProcessorEditor* controller_event_processor = nullptr;
  ScreenOverlayManager* screen_overlay_manager = nullptr;
  ShaderManager* shader_manager = nullptr;
  BufferManager* buffer_manager = nullptr;
  TerrainManager* terrain_manager = nullptr;
  DrawCallManager* draw_call_manager = nullptr;
  MeshDataUtils* mesh_data_utils = nullptr;
  SpatialManager* spatial_manager = nullptr;
  CommandBuffersManager* command_buffers_manager = nullptr;

  static SystemsManager* GetInstance() {
    static SystemsManager* instance = new SystemsManager();
    return instance;
  };

  ~SystemsManager() {};

  void Render();
  void InitSystems();
  void ChangeStateUpdate();
  void _ChangePhysicsDebugDrawer();
  void Reset();

  void PreUpdate();
  void Update();
  void Clear();
  void CreateDebugDrawer();

private:
  SystemsManager() { };
  void _SubmitAsyncTasks();
  void AddDraws();
};

#endif
