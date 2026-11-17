ADB=~/Android/Sdk/platform-tools/adb
PACKAGE=com.yourname.game

# ---- install + launch ----
$ADB install -r android-build/app/outputs/apk/debug/app-debug.apk
$ADB shell am force-stop $PACKAGE
$ADB shell am start -n $PACKAGE/android.app.NativeActivity


sleep 30   # or longer: get the game into the slow scene and wait until you see the 20 fps
# $ADB shell simpleperf record --app $PACKAGE -g --duration 10 -o /data/local/tmp/perf.data
# $ADB shell simpleperf report -i /data/local/tmp/perf.data --sort dso | head -15
# $ADB shell simpleperf report -i /data/local/tmp/perf.data --sort symbol | head -30


# $ADB logcat -d | grep -i "hardware instancing"

# ---- B) System trace (perfetto). Only if you need it again, in a separate run. ----
# TRACE=/data/misc/perfetto-traces/trace.perfetto-trace
# $ADB shell perfetto -o $TRACE -t 10s sched freq idle gfx view am wm input
# $ADB pull $TRACE trace.perfetto-trace

# ---- logs ----
$ADB logcat --pid=$($ADB shell pidof -s $PACKAGE)