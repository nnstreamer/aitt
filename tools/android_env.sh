#!/bin/bash

sudo apt install -y default-jre curl

if [ ! $ANDROID_SDK_ROOT ]; then
	if [ -d "${HOME}/Android/Sdk" ]; then
		echo "export ANDROID_SDK_ROOT=\$HOME/Android/Sdk" >> ~/.profile
	else
		echo "Fail : Please change SDK location in this script"
		echo "Did you install android studio?(https://developer.android.com/studio)"
		exit
	fi

fi
if [ ! $ANDROID_NDK_ROOT ]; then
	if [ -d "${HOME}/Android/Sdk/ndk/21.3.6528147" ]; then
		echo "export ANDROID_NDK_ROOT=\$HOME/Android/Sdk/ndk/21.3.6528147" >> ~/.profile
	else
		echo "Fail : Please change NDK location in this script"
		echo "Android Studio >> Tools >> SDK Manager >> SDK Tools >> check [Show Package Details] >> NDK(21.3.6528147) "
		echo "https://developer.android.com/studio/projects/install-ndk"
		exit
	fi
fi

if [ ! -e "$HOME/.sdkman/bin/sdkman-init.sh" ]; then
	curl -s "https://get.sdkman.io" | bash
fi

source "$HOME/.sdkman/bin/sdkman-init.sh"
sdk install gradle

echo "It is needed to reboot for applying this system-wide"
#sudo reboot
