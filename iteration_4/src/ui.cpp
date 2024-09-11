#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <glfw3webgpu.h> // glfwGetWGPUSurface
#include "debug.h"

#define WEBGPU_DAWN

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif // EMSCRIPTEN


// Callback function for requesting adapter
void AdapterRequestCallback(WGPURequestAdapterStatus status, wgpu::Adapter receivedAdapter, const char* message, void* userdata) {
    auto adapter = static_cast<wgpu::Adapter*>(userdata);
    if (status == WGPURequestAdapterStatus_Success) {
        *adapter = receivedAdapter;
    } else {
        std::cerr << "Failed to get an adapter: " << message << std::endl;
    }
}
// Callback function for requesting device
void DeviceRequestCallback(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
    auto wgpuDevice = static_cast<wgpu::Device*>(userdata);
    if (status == WGPURequestDeviceStatus_Success) {
        *wgpuDevice = wgpu::Device::Acquire(device);
    } else {
        std::cerr << "Failed to request WebGPU device: " << message << std::endl;
        exit(-1);
    }
}
// initialize webgpu
void UI::InitializeWebGPU() {
    // Create WebGPU instance
    {
        #ifdef EMSCRIPTEN
            instance = wgpu::CreateInstance(nullptr);
        #else
            wgpu::InstanceDescriptor instanceDesc{};
            instance = wgpu::CreateInstance(&instanceDesc);
        #endif
        if (!instance) {
            std::cerr << "Instance creation failed!" << std::endl;
            exit(-1);
        }
    }

    // Request adapter and add surface
    {
        surface = glfwGetWGPUSurface(instance, window);
        wgpu::RequestAdapterOptions adapterOpts{};
        adapterOpts.compatibleSurface = surface;
        instance.RequestAdapter(&adapterOpts, AdapterRequestCallback, &adapter);
        if (!adapter) {
            std::cerr << "RequestAdapter failed!" << std::endl;
            exit(-1);
        }
    }

    // Request device
    {
        wgpu::DeviceDescriptor deviceDesc{};
        adapter.RequestDevice(&deviceDesc, DeviceRequestCallback, &device);
        if (!device) {
            std::cerr << "Failed to create device." << std::endl;
            exit(-1);
        }
    }

    //
    {
        queue = device.GetQueue();
    }


    // Print adapter properties
    {
        wgpu::DawnAdapterPropertiesPowerPreference powerProps{};
        wgpu::AdapterProperties properties;
        adapter.GetProperties(&properties);

        std::cout << "VendorID: " << std::hex << properties.vendorID << std::dec << "\n";
        std::cout << "Vendor: " << properties.vendorName << "\n";
        std::cout << "DeviceID: " << std::hex << properties.deviceID << std::dec << "\n";
        std::cout << "Device: " << properties.deviceName << "\n";
        std::cout << "Driver description: " << properties.driverDescription << "\n";

        #ifndef EMSCRIPTEN
        WGPUSupportedLimits supportedLimits = {};
        supportedLimits.nextInChain = nullptr;

        #ifdef WEBGPU_DAWN
        bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
        #else
        bool success = wgpuAdapterGetLimits(adapter, &supportedLimits);
        #endif
        if (success) {
            std::cout << "Device limits:" << std::endl;
            std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
            std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
            std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
            std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
            std::cout << " - maxBindGroups: " << limits.limits.maxBindGroups << std::endl;
            std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
            std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
            std::cout << " - maxSampledTexturesPerShaderStage: " << limits.limits.maxSampledTexturesPerShaderStage << std::endl;
            std::cout << " - maxSamplersPerShaderStage: " << limits.limits.maxSamplersPerShaderStage << std::endl;
            std::cout << " - maxStorageBuffersPerShaderStage: " << limits.limits.maxStorageBuffersPerShaderStage << std::endl;
            std::cout << " - maxStorageTexturesPerShaderStage: " << limits.limits.maxStorageTexturesPerShaderStage << std::endl;
            std::cout << " - maxUniformBuffersPerShaderStage: " << limits.limits.maxUniformBuffersPerShaderStage << std::endl;
            std::cout << " - maxUniformBufferBindingSize: " << limits.limits.maxUniformBufferBindingSize << std::endl;
            std::cout << " - maxStorageBufferBindingSize: " << limits.limits.maxStorageBufferBindingSize << std::endl;
            std::cout << " - minUniformBufferOffsetAlignment: " << limits.limits.minUniformBufferOffsetAlignment << std::endl;
            std::cout << " - minStorageBufferOffsetAlignment: " << limits.limits.minStorageBufferOffsetAlignment << std::endl;
            std::cout << " - maxVertexBuffers: " << limits.limits.maxVertexBuffers << std::endl;
            std::cout << " - maxVertexAttributes: " << limits.limits.maxVertexAttributes << std::endl;
            std::cout << " - maxVertexBufferArrayStride: " << limits.limits.maxVertexBufferArrayStride << std::endl;
            std::cout << " - maxInterStageShaderComponents: " << limits.limits.maxInterStageShaderComponents << std::endl;
            std::cout << " - maxComputeWorkgroupStorageSize: " << limits.limits.maxComputeWorkgroupStorageSize << std::endl;
            std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
            std::cout << " - maxComputeWorkgroupSizeX: " << limits.limits.maxComputeWorkgroupSizeX << std::endl;
            std::cout << " - maxComputeWorkgroupSizeY: " << limits.limits.maxComputeWorkgroupSizeY << std::endl;
            std::cout << " - maxComputeWorkgroupSizeZ: " << limits.limits.maxComputeWorkgroupSizeZ << std::endl;
            std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.limits.maxComputeWorkgroupsPerDimension << std::endl;
        }
        #endif // NOT EMSCRIPTEN

        std::vector<WGPUFeatureName> features;
        size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
        features.resize(featureCount);
        wgpuDeviceEnumerateFeatures(device, features.data());

        std::cout << "Device features:" << std::endl;
        std::cout << std::hex;
        for (auto f : features) {
            std::cout << " - 0x" << f << std::endl;
        }
        std::cout << std::dec;

        wgpu::AdapterProperties properties = {};
        properties.nextInChain = nullptr;
        adapter.GetProperties(&properties);
        std::cout << "Adapter properties:" << std::endl;
        std::cout << " - vendorID: " << properties.vendorID << std::endl;
        if (properties.vendorName) {std::cout << " - vendorName: " << properties.vendorName << std::endl;}
        if (properties.architecture) {std::cout << " - architecture: " << properties.architecture << std::endl;}
        std::cout << " - deviceID: " << properties.deviceID << std::endl;
        if (properties.name) {std::cout << " - name: " << properties.name << std::endl;}
        if (properties.driverDescription) {std::cout << " - driverDescription: " << properties.driverDescription << std::endl;}
        std::cout << std::hex;
        std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
        std::cout << " - backendType: 0x" << properties.backendType << std::endl;
        std::cout << std::dec; // Restore decimal numbers

        switch (properties.backendType) {
            case wgpu::BackendType::D3D12:
                std::cout << " - Backend: DirectX 12\n";
                break;
            case wgpu::BackendType::Metal:
                std::cout << " - Backend: Metal\n";
                break;
            case wgpu::BackendType::Vulkan:
                std::cout << " - Backend: Vulkan\n";
                break;
            case wgpu::BackendType::OpenGL:
                std::cout << " - Backend: OpenGL\n";
                break;
            case wgpu::BackendType::OpenGLES:
                std::cout << " - Backend: OpenGL ES\n";
                break;
            default:
                std::cout << " - Backend: Unknown\n";
                break;
        }
    }
}

void InitializeWGPU() {






    //RenderPipelineDescriptor pipelineDesc;
    // Describe render pipeline
    //RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);


}
void UI::InitializeGLFW(unsigned int width, unsigned int height, const char* title) {

    // Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        exit(-1);
    }


    // Set GLFW window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        exit(-1);
    }

    // Make the OpenGL context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        exit(-1);
    }
}

void UI::Initialize() {

    UI::InitializeWebGPU();
    UI::InitializeGLFW(800, 600, "WebGPU UI");

    #ifdef EMSCRIPTEN
    auto callback = [](void* arg) {
        UI* pUI = reinterpret_cast<UI*>(arg);
        pUI->MainLoop(); // Call the main loop function of the UI object
    };
    emscripten_set_main_loop_arg(callback, this, 0, true);
    #endif
}
void UI::Terminate() {
    instance.release();
    adapter.release();
    queue.release();
    wgpuSurfaceRelease(surface);

    glfwDestroyWindow(window);
    glfwTerminate();
}
void UI::MainLoop() {
    #ifdef EMSCRIPTEN
    // ????
    #else // EMSCRIPTEN
    if (glfwGetKey(window.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window.window, true);
    }

    glfwPollEvents();
    #endif // EMSCRIPTEN

}
bool UI::IsRunning() {
    return !glfwWindowShouldClose(window.window);
}

