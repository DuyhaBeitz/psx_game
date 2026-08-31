cmake -S . -B build -DPLATFORM=Desktop -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

ln -sf "$(pwd)/assets" build/