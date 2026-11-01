cd /code/lil_engine/
./rebuild.sh
./linux_package_sdk.sh
rm -rf /code/psx_game/lil_engine_sdk
cp -r /code/lil_engine/lil_engine_sdk /code/psx_game/lil_engine_sdk
cd /code/psx_game
./rebuild.sh