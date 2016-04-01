cmake_minimum_required (VERSION 2.6)

link_libraries(apriltags)

add_executable(apriltag_image_detector apriltag_image_detector.cpp Serial.cpp)
pods_install_executables(apriltag_image_detector)