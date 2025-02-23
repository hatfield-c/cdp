#pragma once

struct PlanStep {
	float wall_direction;
	const char* start_proximity_path = "\0";
	int target_center;
	bool is_transit = true;

	float* start_proximity_hash = new float[16];

	int proximity_size = 16;

	void Init() {
		int proximity_memory_size = proximity_size * sizeof(float);
		
		if (this->IsStartValid()) {
			memset(this->start_proximity_hash, 0, proximity_memory_size);

			FILE* in_file;
			fopen_s(&in_file, this->start_proximity_path, "rb");
			if (in_file == NULL) {
				printf("\n\n[Warning] .phash file did not open when loading:\n    %s!\n", start_proximity_path);
				exit(1);
			}
			int result = fread(this->start_proximity_hash, sizeof(float), proximity_size, in_file);
			fclose(in_file);
		}
	}

	bool IsStartValid() {
		if (this->start_proximity_path[0] == '\0') {
			return false;
		}

		return true;
	}
};