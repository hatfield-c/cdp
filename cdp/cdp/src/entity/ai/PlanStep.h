#pragma once

struct PlanStep {
	float wall_direction;
	const char* start_proximity_path = "\0";
	int target_center;
	bool is_transit = true;

	float* start_proximity_hash = new float[16];
	Vector3* mass_centers = new Vector3[16];
	Vector3 mass_target{ 0, 16, -1 };

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
			int result = (int)fread(this->start_proximity_hash, sizeof(float), (unsigned int)proximity_size, in_file);
			fclose(in_file);
		}

		this->ExtractMassCenters();
		//exit(0);
	}

	void ExtractMassCenters() {
		for (int i = 0; i < 16; i++) {
			this->mass_centers[i] = Vector3{ 0, 16, -1 };
		}

		if (!this->IsStartValid()) {
			return;
		}

		float mass_start = 0;
		float mass_end = 16;
		float center_depth = 16;

		float mass_threshold = 1;

		for (int i = 1; i < 16; i++) {
			float depth0 = this->start_proximity_hash[i - 1];
			float depth1 = this->start_proximity_hash[i];

			if (depth0 < center_depth) {
				center_depth = depth0;
			}

			float delta = depth1 - depth0;

			if (abs(delta) > mass_threshold || (depth0 != 16 && depth1 == 16) || (depth0 == 16 && depth1 != 16) || i == 15) {
				mass_end = (float)i - 1.0f;

				int mass_center = (int)round((mass_end + mass_start) / 2);

				if (center_depth < 16 && mass_center != 0) {
					this->mass_centers[mass_center].x = mass_end - mass_start;
					this->mass_centers[mass_center].y = center_depth;
					this->mass_centers[mass_center].z = (float)mass_center;

					if (mass_center >= 6 && mass_center <= 9) {
						this->mass_target = this->mass_centers[mass_center];
						//this->mass_target.Print("**");
					}

					//this->mass_centers[mass_center].Print();
				}
				
				mass_start = (float)i;
				mass_end = 16.0f;
				center_depth = 16.0f;
			}
		}
	}

	bool IsStartValid() {
		if (this->start_proximity_path[0] == '\0') {
			return false;
		}

		return true;
	}
};