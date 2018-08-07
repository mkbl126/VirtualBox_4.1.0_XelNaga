

#--------------- 
#!/bin/sh 

if [ $# != 2 ] 
then 
    echo "  !!Usage: vbox_build.sh abs_vbox_path debug|release"
    exit 
else 
    echo   "Your input is: "   $1   $2 
fi 
#-------------- 

# run the script in the VitualBox folder please.

# configure and build
echo -e "\n !! configure the kmakefiles !! \n"
cd $1
./configure --disable-hardening --disable-docs  --disable-pulse --disable-opengl --disable-alsa --disable-java --disable-python --disable-sdl-ttf

#  --disable-xpcom --disable-qt4    

source ./env.sh


if [ $2 == "debug" ]
then
    echo -e "\n !! BUILD DEBUG VERSION !! \n"
    kmk BUILD_TYPE=debug
else
    echo -e "\n !! BUILD RELEASE VERSION !! \n"
    kmk
fi

# create some s-links

echo -e "\n\n !! create some s-links !! \n\n"
cd $1
if [ $2 == "debug" ]
then
    cd out/linux.x86/debug/bin/components
else
    cd out/linux.x86/release/bin/components
fi

ln -s ../VBoxDDU.so
ln -s ../VBoxREM.so
ln -s ../VBoxRT.so
ln -s ../VBoxVMM.so
ln -s ../VBoxXPCOM.so


# To be more LSB-compliant, change the default pathes which are used by the
# VirtualBox binaries to find their components. Add the following build 
# variables to LocalConfig.kmk: 

#VBOX_PATH_APP_PRIVATE_ARCH := /usr/lib/virtualbox
#VBOX_PATH_SHARED_LIBS := $(VBOX_PATH_APP_PRIVATE_ARCH)
#VBOX_WITH_ORIGIN :=
#VBOX_WITH_RUNPATH := $(VBOX_PATH_APP_PRIVATE_ARCH)
#VBOX_PATH_APP_PRIVATE := /usr/share/virtualbox
#VBOX_PATH_APP_DOCS := /usr/share/doc/virtualbox
#VBOX_WITH_TESTCASES :=
#VBOX_WITH_TESTSUITE :=

# build and install the VirtualBox kernel module 

echo -e "\n\n !! build and install the VirtualBox kernel module !! \n\n"
cd $1
if [ $2 == "debug" ]
then
    cd out/linux.x86/debug/bin/src
else
    cd out/linux.x86/release/bin/src
fi
make
sudo make install
modprobe vboxdrv
cd .. 



