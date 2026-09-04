cmake -S . -B build -DPLATFORM=Desktop -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

ln -sf "$(pwd)/lil_engine_sdk/imgui.ini" imgui.ini
ln -sf "$(pwd)/assets" build/