#include "CudaHitPoly.cuh"

__global__ void CudaHitPoly::GenerateTrainingData_Kernel(TrainingGenerator generator) {
    generator.GenerateData();
}
void CudaHitPoly::GenerateTrainingData() {
    Vector3 position_lower{ -1, 1, -8 };
    Vector3 position_upper{ 1, 8, 0 };
    Vector3 position_steps = (position_upper + 1) - position_lower;
    Vector3 velocity_lower{ -15, 0, -15 };
    Vector3 velocity_upper{ 15, 0, 15 };
    Vector3 velocity_steps = (velocity_upper + 1) - velocity_lower;

    TrainingGenerator generator{};
    generator.Init(position_lower, position_steps, velocity_lower, velocity_steps);

    dim3 threads_per_block(32, 1, 1);
    unsigned long long block_count = (unsigned long long)ceil((double)generator.state_count / (double)threads_per_block.x);
    dim3 blocks_per_grid((unsigned int)block_count, 1, 1);

    CudaHitPoly::GenerateTrainingData_Kernel<<<blocks_per_grid, threads_per_block>>>(generator);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    float* state_cpu = new float[generator.float_count];
    float* value_cpu = new float[generator.state_count];
    
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMemcpy(state_cpu, generator.state_data, generator.float_count * sizeof(float), cudaMemcpyDeviceToHost), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaMemcpy(value_cpu, generator.value_data, generator.state_count * sizeof(float), cudaMemcpyDeviceToHost), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);

    unsigned long long positive_count = 0;
    for (unsigned long long i = 0; i < generator.state_count; i++) {

        if (value_cpu[i] > 0.0) {
            positive_count++;
        }
    }

    float* state_positives = new float[positive_count * generator.dim];
    unsigned long long positive_index = 0;
    for (unsigned long long i = 0; i < generator.state_count; i++) {
        if (value_cpu[i] > 0.0) {
            unsigned long long state_base_index = Indexer::FlatIndex2(0, i, generator.dim);
            unsigned long long positive_base_index = Indexer::FlatIndex2(0, positive_index, generator.dim);

            state_positives[positive_base_index + 0] = state_cpu[state_base_index + 0];
            state_positives[positive_base_index + 1] = state_cpu[state_base_index + 1];
            state_positives[positive_base_index + 2] = state_cpu[state_base_index + 2];
            state_positives[positive_base_index + 3] = state_cpu[state_base_index + 3];
            state_positives[positive_base_index + 4] = state_cpu[state_base_index + 4];
            state_positives[positive_base_index + 5] = state_cpu[state_base_index + 5];

            //printf("%lld - %lld - [%.2f %.2f %.2f][%.2f %.2f %.2f]\n", i, positive_index, state_cpu[state_base_index + 0], state_cpu[state_base_index + 1], state_cpu[state_base_index + 2], state_cpu[state_base_index + 3], state_cpu[state_base_index + 4], state_cpu[state_base_index + 5]);

            positive_index++;
        }
    }

    std::string state_path = "data/hitpoly/training/state_data.float";
    std::string value_path = "data/hitpoly/training/value_data.float";
    std::string positive_path = "data/hitpoly/training/state_positive.float";

    FILE* out_file;
    fopen_s(&out_file, state_path.c_str(), "wb+");
    if (out_file == NULL) {
        printf("\n\nWarning: File did not open when saving state data:\n    %s!\n", state_path.c_str());
        exit(1);
    }
    int result = (int)fwrite(state_cpu, sizeof(float), generator.float_count, out_file);
    fclose(out_file);

    fopen_s(&out_file, value_path.c_str(), "wb+");
    if (out_file == NULL) {
        printf("\n\nWarning: File did not open when saving value data:\n    %s!\n", value_path.c_str());
        exit(1);
    }
    result = (int)fwrite(value_cpu, sizeof(float), generator.state_count, out_file);
    fclose(out_file);

    fopen_s(&out_file, positive_path.c_str(), "wb+");
    if (out_file == NULL) {
        printf("\n\nWarning: File did not open when saving positive data:\n    %s!\n", positive_path.c_str());
        exit(1);
    }
    result = (int)fwrite(state_positives, sizeof(float), positive_count * generator.dim, out_file);
    fclose(out_file);

    printf("Generation Complete:\n");
    printf("    Positive: %lld\n", positive_count);
    printf("    Total   : %lld\n", generator.state_count);
    printf("    Ratio   : %.4f\n\n", ((float)positive_count) / ((float)generator.state_count));

    generator.Free();
    CudaError::CheckError((cudaError_enum)cudaPeekAtLastError(), __FILE__, __LINE__);
    CudaError::CheckError((cudaError_enum)cudaDeviceSynchronize(), __FILE__, __LINE__);
}

