class UI {
public:

    // constructor
    UI();

    // destructor
    ~UI();

    // Initialize everything and return true if it went all right
    void Initialize(unsigned int width, unsigned int height, const char* title);

    // Uninitialize everything that was initialized
    void Terminate();

    // Draw a frame and handle events
    void MainLoop();

    // Return true as long as the main loop should keep on running
    void IsRunning();
private:
    GLFWwindow* window = nullptr;

    wgpu::Instance  instance;
    wgpu::Adapter   adapter;
    wgpu::Device    device;
    wgpu::Queue     queue;
    wgpu::Surface   surface;


    void InitializeWebGPU();
    void InitializeGLFW(unsigned int width, unsigned int height, const char* title);
};
