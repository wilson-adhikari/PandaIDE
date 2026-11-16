// Camera C++ module for vision system
#include <cstdio>

extern "C" {
    // Camera initialization
    void camera_init() {
        printf("Camera initialized\\n");
    }
    
    // Frame capture
    int camera_capture_frame() {
        return 0; // Success
    }
    
    // Get frame data
    unsigned char* camera_get_frame(int* width, int* height) {
        static unsigned char dummy[640 * 480 * 3] = {0};
        *width = 640;
        *height = 480;
        return dummy;
    }
}