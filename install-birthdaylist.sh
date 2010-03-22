#!/bin/bash

fail() {
    echo "BirthdayList installation failed$1";
    exit 1;
}


( mkdir -p applet/build ) || fail ", cannot create the applet build directory.";
cd applet/build
( cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` .. ) ||
    fail ".";
( make ) || fail ".";
( sudo make install ) || fail ".";

cd ../..

( mkdir -p dataengine-default/build ) || fail ", cannot create the dataengine build directory.";
cd dataengine-default/build
( cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` .. ) ||
    fail ".";
( make ) || fail ".";
( sudo make install ) || fail ".";


echo "Rebuilding the system configuration cache.";
kbuildsycoca4 &>/dev/null

echo "BirthdayList successfully installed.";
exit 0;
