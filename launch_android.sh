ADB=~/Android/Sdk/platform-tools/adb
PACKAGE=com.yourname.game

# $ADB logcat -c
$ADB install -r android-build/app/outputs/apk/debug/app-debug.apk
$ADB shell am start -n $PACKAGE/android.app.NativeActivity
# sleep 1
# $ADB logcat --pid=$($ADB shell pidof -s $PACKAGE)