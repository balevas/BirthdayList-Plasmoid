#!/bin/bash

fail() {
    echo "BirthdayList installation failed$1";
    exit 1;
}


( mkdir -p build ) || fail ", cannot create the build directory.";
cd build
( cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` .. ) ||
    fail ".";
( make ) || fail ".";
( sudo make install ) || fail ".";


echo "Rebuilding the system configuration cache.";
kbuildsycoca4 &>/dev/null

echo "BirthdayList successfully installed.";
exit 0;
