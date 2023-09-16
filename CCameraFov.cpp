#include "CCameraFov.h"
#include "Utils.h"
#include <vector>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_internal.h"
#include <Windows.h>
void CCameraFov::Init()
{
	try {
		auto camFuncAddr = Utils::PatternScan("4C 8B 05 ? ? ? ? 41 80 B8 ? ? ? ? ? 74 09 49 8D 80 ? ? ? ? EB 13");


		auto objBase = (int64_t)camFuncAddr + 3;

		std::cout << "camFuncAddr " << (void*)camFuncAddr << std::endl;

		objBase = *(int*)(camFuncAddr + 3);

		obj = *(int64_t*)(camFuncAddr + 3 + 4 + objBase);


		std::cout << "obj address " << obj << std::endl;

		battleCamOffset = *(int*)(Utils::PatternScan("49 8D 80 ? ? ? ? F6 C1 01", camFuncAddr) + 3);
		worldCamOffset = *(int*)(Utils::PatternScan("F6 C1 01 75 07 49 8D 80", camFuncAddr) + 8);


		std::cout << "battleCamOffset " << battleCamOffset << std::endl;
		std::cout << "worldCamOffset " << worldCamOffset << std::endl;


		uint64_t* maxDistOffAddrTarget = (uint64_t*)(Utils::PatternScan("C3 F3 0F 10 48", camFuncAddr));

		maxDistOffAddr = (uint64_t)((uint64_t)maxDistOffAddrTarget - (uint64_t)camFuncAddr);

		//std::cout << "maxDistOffAddr ptr " << (void*)maxDistOffAddr << std::endl;
		std::cout << "maxDistOffAddr " << maxDistOffAddr << std::endl;

		camMaxOffset = camFuncAddr[maxDistOffAddr + 5];

		std::cout << "camMaxOffset " << camMaxOffset << std::endl;


		auto camMaxAbs = (uint64_t)((uint64_t)Utils::PatternScan("F3 0F 10 80 ? ? ? ? C3", camFuncAddr) - (uint64_t)camFuncAddr);
		camMaxAbsOffset = camFuncAddr[camMaxAbs + 4];


		auto camTilt = Utils::PatternScan("C3 F3 0F 10 80 ? ? ? ? F3 0F 10 88 ? ? ? ? 0F 14 C8 66 48 0F 7E C8 C3");
		camTiltOffset = *(int*)((camTilt + 5));


		settings.Add(false, "Max zoom", { obj + worldCamOffset + camMaxOffset, obj + battleCamOffset + camMaxOffset });
		settings.Add(false, "Min zoom", { obj + worldCamOffset + camMaxOffset + 4, obj + battleCamOffset + camMaxOffset + 4 });

		settings.Add(false, "Tactical zoom", { obj + worldCamOffset + camMaxAbsOffset, obj + battleCamOffset + camMaxAbsOffset });
		settings.Add(false, "Tactical min zoom", { obj + worldCamOffset + camMaxAbsOffset - 4, obj + battleCamOffset + camMaxAbsOffset - 4 });

		settings.Add(false, "Tilt speed", { obj + worldCamOffset + camMaxOffset + 200 });

		settings.Add(false, "FOV", { obj + worldCamOffset + camMaxOffset + 92,
			obj + worldCamOffset + camMaxOffset + 96,
			obj + battleCamOffset + camMaxOffset + 92,
			obj + battleCamOffset + camMaxOffset + 96
			});

		settings.Add(false, "Camera distance", { obj + worldCamOffset + camMaxOffset + 172, obj + battleCamOffset + camMaxOffset + 172 });

		settings.Add(false, "Scroll speed", { obj + worldCamOffset + camMaxOffset + 136, obj + battleCamOffset + camMaxOffset + 136 }, 0.f, 3.f);

		settings.Add(true, "Camera height", { obj + worldCamOffset + camMaxOffset + 60, obj + battleCamOffset + camMaxOffset + 60 }, 0.0f, 900.f);

		settings.Read();
	}
	catch (std::exception ex) {
		MessageBox(0, ex.what(), "Error occured!", MB_OK);
	}
}
void CCameraFov::CombatCameraZoom() {
	std::string pattern = "F3 45 0F 11 4C 24 ?";
	std::string pattern2 = "E8 ? ? ? ? EB 1E F3 0F 10 57 ?";
	static auto foundAddresses = Utils::FindAllPatterns(pattern.c_str());
	static auto foundAddresses2 = Utils::FindAllPatterns(pattern2.c_str());

	static std::vector<std::vector<uint8_t>> original_bytes_first;
	static std::vector<std::vector<uint8_t>> original_bytes_second;
	static uint8_t nopBytes[]{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	static uint8_t nopBytes2[]{ 0x90, 0x90, 0x90, 0x90, 0x90 };


	if (foundAddresses.size() > 0 && foundAddresses2.size() > 0) {
		if (bEnableCombatCameraZoom) {
		
			DWORD oldProtect;

			for (int i = 0; i < foundAddresses.size(); i++)
			{
				int64_t address = foundAddresses[i];
				
				if (VirtualProtect((void*)(address), sizeof(nopBytes), PAGE_EXECUTE_READWRITE, &oldProtect))
				{
					std::vector<uint8_t> original; original.resize(8);
					
					memcpy((void*)original.data(), (void*)(address), sizeof(nopBytes));

					original_bytes_first.push_back(original);

					memcpy((void*)address, (void*)(&nopBytes), sizeof(nopBytes));
					VirtualProtect((void*)(address), sizeof(nopBytes), oldProtect, &oldProtect);
				}
			}
			for (size_t i = 0; i < foundAddresses2.size(); i++)
			{
				int64_t address2 = *(int64_t*)(foundAddresses2[i] + 1) + foundAddresses2[i] + 5 + 0x2A;
				if (VirtualProtect((void*)(address2), sizeof(nopBytes2), PAGE_EXECUTE_READWRITE, &oldProtect))
				{
					std::vector<uint8_t> original; original.resize(8);

					memcpy((void*)original.data(), (void*)(address2), sizeof(nopBytes2));

					original_bytes_second.push_back(original);

					memcpy((void*)address2, (void*)(&nopBytes2), sizeof(nopBytes2));
					VirtualProtect((void*)(address2), sizeof(nopBytes2), oldProtect, &oldProtect);
				}
			}

		}
		else {
			DWORD oldProtect;

			for (int i = 0; i < foundAddresses.size(); i++)
			{
				int64_t address = foundAddresses[i];
				if (VirtualProtect((void*)(address), sizeof(nopBytes), PAGE_EXECUTE_READWRITE, &oldProtect))
				{
					memcpy((void*)address, (void*)original_bytes_first.at(i).data(), sizeof(nopBytes));
					VirtualProtect((void*)(address), sizeof(nopBytes), oldProtect, &oldProtect);
				}
			}
			for (int i = 0; i < foundAddresses2.size(); i++)
			{
				int64_t address2 = *(int64_t*)(foundAddresses2[i] + 1) + foundAddresses2[i] + 5 + 0x2A;
				if (VirtualProtect((void*)(address2), sizeof(nopBytes2), PAGE_EXECUTE_READWRITE, &oldProtect))
				{
					memcpy((void*)address2, (void*)original_bytes_second.at(i).data(), sizeof(nopBytes));
					VirtualProtect((void*)(address2), sizeof(nopBytes2), oldProtect, &oldProtect);
				}
			}
		}
	}
}
void CCameraFov::OnFrame()
{
	static int prevMouseY;
	static float curTilt;

	int diff = 0;
	static bool Mouse = false;

	if (ImGui::IsKeyPressed(ImGuiKey_MouseMiddle, false)) {
		prevMouseY = ImGui::GetMousePos().y;
	}

	if (ImGui::IsKeyDown(ImGuiKey_MouseMiddle) || ImGui::IsKeyDown(ImGuiKey_R))
	{
		diff = ImGui::GetMousePos().y - prevMouseY;
		Mouse = true;
	}
	
	{
		auto RStick_Up = ImGui::GetKeyData(ImGui::GetCurrentContext(), ImGuiKey_GamepadRStickUp);
		auto RStick_Down = ImGui::GetKeyData(ImGui::GetCurrentContext(), ImGuiKey_GamepadRStickDown);
		auto RStick_Left = ImGui::GetKeyData(ImGui::GetCurrentContext(), ImGuiKey_GamepadRStickLeft);
		auto RStick_Right = ImGui::GetKeyData(ImGui::GetCurrentContext(), ImGuiKey_GamepadRStickRight);

		int maxValue = 20;
		if (RStick_Up->AnalogValue > 0.1)
		{
			diff = ((int)(maxValue * RStick_Up->AnalogValue));
		}
		else if (RStick_Down->AnalogValue > 0.1)
		{ 

			diff = -((int)(maxValue * RStick_Down->AnalogValue));
		}
	}

	if (diff != 0)
	{
		curTilt += diff * 0.05f;
		*(float*)(obj + worldCamOffset + camTiltOffset) = curTilt;
		*(float*)(obj + worldCamOffset + camTiltOffset + 4) = curTilt;
		*(float*)(obj + worldCamOffset + camTiltOffset + 8) = curTilt;
		*(float*)(obj + worldCamOffset + camTiltOffset + 12) = curTilt;
		*(float*)(obj + battleCamOffset + camTiltOffset) = curTilt;
		*(float*)(obj + battleCamOffset + camTiltOffset + 4) = curTilt;
		*(float*)(obj + battleCamOffset + camTiltOffset + 8) = curTilt;
		*(float*)(obj + battleCamOffset + camTiltOffset + 12) = curTilt;
		if (Mouse == false)
		{
			prevMouseY = (int)curTilt;
		}
		else
		{
			prevMouseY = ImGui::GetMousePos().y;
			Mouse = false;
		}
	}
}

void CCameraFov::RenderGui()
{
	ImGui::Begin("Camera settings", nullptr, ImGuiWindowFlags_NoCollapse); {
		for (auto& set : settings.options) {
		
				if (ImGui::SliderFloat(set.name.c_str(), &set.value, set.min_value, set.max_value)) {
					set.Update();
				}
			
		}
		if (ImGui::Checkbox("Combat camera zoom", &bEnableCombatCameraZoom)) {
			CombatCameraZoom();
		}
	}
	ImGui::End();
}
