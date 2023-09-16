#pragma once
#include <iostream>
#include <vector>
class CCameraSettingsOption {
public:
	std::string name;
	std::vector<int64_t> addresses;
	float value = 0.f;
	double value_d = 0.f;
	float min_value, max_value;
	bool m_Bas_double = false;
public:
	CCameraSettingsOption(bool as_double, std::string name, std::vector<int64_t> _addresses, float min ,float max) {
		this->name = name;
		this->addresses = _addresses;
		this->m_Bas_double = as_double;
		this->min_value = min;
		this->max_value = max;
		if (this->addresses.size() > 0) {
			if (m_Bas_double) {//00085
				
				value = *(double*)this->addresses.at(0) * 1000000;
			}
			else {
				value = *(float*)this->addresses.at(0);
			}
		}
	}
	void Update() {
		for (auto& addr : addresses) {
			if (m_Bas_double) {
				*(double*)addr = value * 0.000001;

				printf("update to value %lf \n", *(double*)addr);
			}
			else
				*(float*)addr = value;
		}
 	}
	void Read() {
		return;
		float average = 0.f; int i = 0;
		for (auto& addr : addresses) {
			average += *(float*)addr; i++;
		}
		if (i > 0)
			value = average / i;
		

		std::cout << "Value for " << name << " is " << value << std::endl;
	}
};
class CCameraSettings {
public:
	std::vector<CCameraSettingsOption> options;
	
	void Add(bool as_double, std::string name, std::initializer_list<int64_t> addresses, float min = 0.f, float max = 100.f) {
		options.push_back(CCameraSettingsOption(as_double, name, std::vector<int64_t>{ addresses }, min, max));
	}

	void Read() {
		for (auto& o : options) {
			o.Read();
		}
	}
};
class CCameraFov
{
public:

	void Init();
	void OnFrame();
	void RenderGui();
	int battleCamOffset = 0;
	int worldCamOffset = 0;
	uint64_t maxDistOffAddr = 0;
	uint8_t* objBase = nullptr;
	int camMaxAbsOffset = 0;
	int camMaxOffset = 0;
	int camTiltOffset = 0;
	int combatZoomOutAddr = 0;
	bool bEnableCombatCameraZoom = false;
	int64_t obj = 0;

	CCameraSettings settings;
	void CombatCameraZoom();
};

