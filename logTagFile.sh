clear
adb logcat -b all -c 
touch libMyHook.log
rm libMyHook.log
adb logcat libMyHook:V *:S > libMyHook.log