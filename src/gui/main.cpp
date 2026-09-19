// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <zmq.hpp>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == 1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

// --- Live video frame streaming over ZeroMQ ---
// model.py runs a PUB socket that continuously publishes JPEG-encoded webcam
// frames. We connect a SUB socket to it here and pull frames on a dedicated
// background thread, so a slow or momentarily stalled network read never
// blocks the ImGui render loop (which needs to stay responsive at all times).
struct LatestFrame
{
    std::mutex mutex;
    std::vector<unsigned char> jpeg_bytes;
    bool has_new_frame = false;
};

static LatestFrame g_latest_frame;
static std::atomic<bool> g_zmq_running{true};

void zmq_frame_receiver_thread()
{
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, zmq::socket_type::sub);

    // Only ever keep the single most recent frame queued. If the GUI can't
    // keep up with the incoming frame rate, we want to drop old frames and
    // show the newest one rather than fall behind and display stale video.
    subscriber.set(zmq::sockopt::conflate, 1);
    subscriber.set(zmq::sockopt::rcvtimeo, 100); // ms - lets us re-check g_zmq_running periodically
    subscriber.set(zmq::sockopt::subscribe, "");
    subscriber.connect("tcp://localhost:5556");

    while (g_zmq_running.load())
    {
        zmq::message_t message;
        auto result = subscriber.recv(message, zmq::recv_flags::none);
        if (!result)
            continue; // recv timed out; loop back and check g_zmq_running

        std::lock_guard<std::mutex> lock(g_latest_frame.mutex);
        const unsigned char* data = static_cast<const unsigned char*>(message.data());
        g_latest_frame.jpeg_bytes.assign(data, data + message.size());
        g_latest_frame.has_new_frame = true;
    }
}

void detection_window_content() {

}

void button_panel_content(bool &show_add_gesture, bool &show_edit_gesture) {

    // TO-DO: 
    // - Add Scroll feature if window too small for buttons to be visible.
    // - Add secondary window when "Add New gesture" is pressed
    // - Add secondary window when "Edit gesture" is pressed

    int button_x_size = ImGui::GetWindowSize().x / 1.2;
    int button_y_size = 70;

    ImGui::SetCursorPos(ImVec2(button_x_size/9, 250));
    if (ImGui::Button("Add New Gesture", ImVec2(button_x_size, button_y_size))) {
        show_add_gesture = true;
    }

    ImGui::SetCursorPos(ImVec2(button_x_size/9, 450));
    if (ImGui::Button("Edit Gestures", ImVec2(button_x_size, button_y_size))) {
        show_edit_gesture = true;
    }

}

// Variables


// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Select GL version + let the backend select a GLSL version
    const char* glsl_version = nullptr;
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + generally GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + generally GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Gesture Commands", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 2.0f; // rounded edges of the buttons/ sliders in the windows
    style.FontSizeBase = 30.0f; 

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    // style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)



    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Custom Font
    std::string fontPath = std::string(PROJECT_ROOT) + "/gui/fonts/PerfectPenmanship.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.c_str());

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool show_add_gesture = false;
    bool show_edit_gesture = false;

    int my_image_width = 640;
    int my_image_height = 480;
    GLuint my_image_texture = 1;
    bool ret = LoadTextureFromFile("../src/cat-test-image.jpg", &my_image_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    // The image above is only a placeholder shown until the first real frame
    // arrives from model.py - it gets replaced as soon as the SUB thread
    // below receives something.

    std::thread zmq_thread(zmq_frame_receiver_thread);

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Pull in the latest frame published by model.py, if a new one has arrived.
        {
            std::vector<unsigned char> jpeg_copy;
            bool got_new_frame = false;
            {
                std::lock_guard<std::mutex> lock(g_latest_frame.mutex);
                if (g_latest_frame.has_new_frame)
                {
                    jpeg_copy = g_latest_frame.jpeg_bytes;
                    g_latest_frame.has_new_frame = false;
                    got_new_frame = true;
                }
            }

            if (got_new_frame)
            {
                GLuint new_texture;
                int new_width, new_height;
                if (LoadTextureFromMemory(jpeg_copy.data(), jpeg_copy.size(), &new_texture, &new_width, &new_height))
                {
                    glDeleteTextures(1, &my_image_texture); // free the old GPU texture before swapping it out
                    my_image_texture = new_texture;
                    my_image_width = new_width;
                    my_image_height = new_height;
                }
            }
        }

        // HAND DETECTION VIDEO DISPLAY WINDOW
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.80f, viewport->Size.y));
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::Begin("Hand Detection Window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);


        // ImGui::Text("pointer = %x", my_image_texture);
        // ImGui::Text("size = %d x %d", my_image_width, my_image_height);
        
        // 640 x 480 image.
        int image_x_size = ImGui::GetWindowSize().x / 5;
        int image_y_size = ImGui::GetWindowSize().y / 5;
        ImGui::SetCursorPos(ImVec2(image_x_size, image_y_size));

        ImGui::Image((ImTextureID)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));

        ImGui::End(); // VIDEO DISPLAY WINDOW END


        // BUTTON WINDOW
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.20f, viewport->Size.y));
        ImGui::SetNextWindowPos(ImVec2(2 % 2 ? 0 : viewport->Size.x - viewport->Size.x * 0.20f, 2 / 2 ? 0 : viewport->Size.y)); // Put window in the top right
        ImGui::Begin("Button Window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

        button_panel_content(show_add_gesture, show_edit_gesture);

        ImGui::End(); // BUTTON WINDOW END

        // ADD GESTURE WINDOW
        if (show_add_gesture)
        {
            ImGui::SetNextWindowSize(ImVec2(600, 500));
            // ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("Add a new Gesture!", &show_add_gesture, ImGuiWindowFlags_NoResize);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            if (ImGui::Button("Close Me"))
                show_add_gesture = false;
            ImGui::End();
        }

        // EDIT GESTURE WINDOW
        if (show_edit_gesture)
        {
            ImGui::SetNextWindowSize(ImVec2(600, 500));
            // ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("Edit Gesture!", &show_edit_gesture, ImGuiWindowFlags_NoResize);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            if (ImGui::Button("Close Me"))
                show_edit_gesture = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }


        glfwSwapBuffers(window);
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    g_zmq_running = false;
    zmq_thread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}