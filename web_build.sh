emcmake cmake -S . -B web-build -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release
cp -r web/shell.html web-build/
cp -r imgui.ini web-build/
cp -r assets/ web-build/
cmake --build web-build -j$(nproc)