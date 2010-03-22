#!/bin/bash

fail() {
    echo "Installation of the Akonadi support for BirthdayList failed$1";
    exit 1;
}


( mkdir -p dataengine-akonadi/build ) || fail ", cannot create the dataengine build directory.";
cd dataengine-akonadi/build
( cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` .. ) ||
    fail ".";
( make ) || fail ".";
( sudo make install ) || fail ".";


echo "Rebuilding the system configuration cache.";
kbuildsycoca4 &>/dev/null

echo "Akonadi support for BirthdayList successfully installed.";
echo "Please relogin or restart Plasma to get Akonadi support in the running applet."

exit 0;
