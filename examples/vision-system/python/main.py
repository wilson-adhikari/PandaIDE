#!/usr/bin/env python3
"""Vision System - Main entry point"""

import sys
import os

# Add C++ module path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

def main():
    print("Vision System Starting...")
    
    # Initialize camera via C++ extension
    from camera import camera_init, camera_capture_frame, camera_get_frame
    
    camera_init()
    
    width, height = 640, 480
    frame = camera_get_frame(&width, &height)
    
    print(f"Camera resolved: {width}x{height}")
    print("Vision system ready")
    
    # Main processing loop
    frame_count = 0
    while True:
        result = camera_capture_frame()
        if result == 0:
            frame_count += 1
            if frame_count % 30 == 0:
                print(f"Processing frame {frame_count}...")
        
        # Break condition for demo
        if frame_count >= 100:
            break
    
    print("Vision system complete")

if __name__ == "__main__":
    main()