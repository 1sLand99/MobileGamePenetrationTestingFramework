clear
adb logcat -b all -c 
touch libMyHook.log
rm libMyHook.log
adb logcat > libMyHook.log