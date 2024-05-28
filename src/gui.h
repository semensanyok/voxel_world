#ifndef VW_GUI_H
#define VW_GUI_H

#include <SDL.h>
#include <functional>
#include <vector>
#include "settings_global.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "Camera.h"
#include "StateManager.h"

using namespace std;

struct ButtonGui {
	const char* label;
	function<void()> callback;
};

class Gui {
	static inline vector<ButtonGui> buttons_callbacks;
public:
	static inline bool enable = true;

	static void InitGuiContext(SDL_Window* window, SDL_GLContext& context) {
		if (!enable) {
			return;
		}
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();
		ImGui_ImplSDL2_InitForOpenGL(window, context);
		const char* glsl_version = "#version 460";
		ImGui_ImplOpenGL3_Init(glsl_version);
	}
	static void AddButton(ButtonGui button) {
		if (!enable) {
			return;
		}
		buttons_callbacks.push_back(button);
	}
	static void RenderGui(SDL_Window* window, Camera* camera) {
		if (!enable) {
			return;
		}
		static bool show_demo_window = true;
		static bool show_another_window = true;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::Begin("Camera");
            ImGui::InputFloat3("POSITION", &camera->pos[0], "%.0f", ImGuiInputTextFlags_None);
			ImGui::Text("Yaw: %.1f Pitch: %.1f", camera->yaw_angle, camera->pitch_angle);
			ImGui::Text("Right: %.1f %.1f %.1f", camera->right.x, camera->right.y, camera->right.z);
			ImGui::Text("Up: %.1f %.1f %.1f", camera->up.x, camera->up.y, camera->up.z);
			ImGui::Text("direction: %.1f %.1f %.1f", camera->direction.x, camera->direction.y, camera->direction.z);
			ImGui::End();
		}
		{
			ImGui::SetNextWindowPos(ImVec2(0, 20));
			ImGui::Begin("Physics");
			ImGui::InputFloat("physics step multiplier", &GameSettings::physics_step_multiplier, 0.001f, 1.0f, "%.6f");
			ImGui::InputInt("ray_debug_draw_lifetime_milliseconds", &GameSettings::ray_debug_draw_lifetime_milliseconds, 500);

			ImGui::End();
		}
#ifdef SAUSAGE_PROFILE_ENABLE
		{
			static bool is_update_time = false;
			using time_unit = std::chrono::milliseconds;
			static time_unit t1 = time_unit();
			static time_unit t2 = time_unit();
			static time_unit t3 = time_unit();
			static time_unit t4 = time_unit();
			static time_unit t5 = time_unit();
			static time_unit t6 = time_unit();
			static time_unit t7 = time_unit();
			static time_unit t8 = time_unit();
			static time_unit t9 = time_unit();
			static time_unit t10 = time_unit();
			static time_unit t11 = time_unit();
			if (is_update_time) {
				t1 = std::chrono::duration_cast<time_unit>(ProfTime::prepare_draws_ns);
				t2 = std::chrono::duration_cast<time_unit>(ProfTime::render_commands_ns);
				t3 = std::chrono::duration_cast<time_unit>(ProfTime::render_commands_ns);
				t4 = std::chrono::duration_cast<time_unit>(ProfTime::render_total_ns);
				t5 = std::chrono::duration_cast<time_unit>(ProfTime::render_gui_ns);
				t6 = std::chrono::duration_cast<time_unit>(ProfTime::render_swap_window_ns);
				t7 = std::chrono::duration_cast<time_unit>(ProfTime::total_frame_ns);
				t8 = std::chrono::duration_cast<time_unit>(ProfTime::physics_sym_ns);
				t9 = std::chrono::duration_cast<time_unit>(ProfTime::physics_buf_trans_ns);
				t10 = std::chrono::duration_cast<time_unit>(ProfTime::physics_debug_draw_world_ns);
				t11 = std::chrono::duration_cast<time_unit>(ProfTime::anim_update_ns);
			}
			if (ImGui::TreeNode("Profile"))
			{

				if (ImGui::TreeNode("Render thread:")) {
					ImGui::Text("prepare_draws_ns: %i", t1);
					ImGui::Text("Renderer class:");
					ImGui::Text("render_commands_ns: %i", t2);
					ImGui::Text("render_draw_ns: %i", t3);
					ImGui::Text("render_total_ns: %i", t4);
					ImGui::Text("render_gui_ns: %i", t5);
					ImGui::Text("render_swap_window_ns: %i", t6);
					ImGui::Text("total ms: %i", t7);
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Physics thread:")) {
					ImGui::Text("simulate physics ms: %i", t8);
					ImGui::Text("update physics transforms ms: %i", t9);
					ImGui::Text("physics debug draw ms: %i", t10);
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Anim thread:")) {
					ImGui::Text("anim update: %i", t11);
					ImGui::TreePop();
				}
				ImGui::Checkbox("Update time", &is_update_time);
				ImGui::Checkbox("Debug Draw Physics", &StateManager::GetInstance()->phys_debug_draw);
				ImGui::TreePop();
			}
      if (ImGui::TreeNode("Cam Controls"))
      {
        ImGui::InputFloat("physics step multiplier", &GameSettings::physics_step_multiplier, 0.001f, 1.0f, "%.6f");
        ImGui::TreePop();
      }
			for (auto& button : buttons_callbacks) {
				if (ImGui::Button(button.label)) {
					button.callback();
				}
			}
		}
#endif

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	static void CleanupGui() {
		if (!enable) {
			return;
		}
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
}

#endif
