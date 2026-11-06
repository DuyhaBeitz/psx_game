cd /code/lil_engine/
./rebuild.sh
./web_build.sh
./linux_package_sdk_basic_backend.sh
cd /code/psx_game/
rm -rf /code/psx_game/lil_engine_sdk
cp -r /code/lil_engine/lil_engine_sdk /code/psx_game/