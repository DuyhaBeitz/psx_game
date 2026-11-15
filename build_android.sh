rm -rf android/app/src/main/assets
mkdir -p android/app/src/main/assets
cp -a assets android/app/src/main/assets/assets

export ANDROID_HOME=~/Android/Sdk
export JAVA_HOME=/usr/lib64/jvm/java-21-openjdk-21

android/gradlew -p android assembleDebug -PbuildRoot="$(pwd)/android-build"