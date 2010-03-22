#!/bin/bash

cd applet/build
( sudo make uninstall ) || { echo "BirthdayList applet uninstallation failed"; }

cd ../../dataengine-default/build
( sudo make uninstall ) || { echo "BirthdayList dataengine uninstallation failed"; exit 1; }

echo "BirthdayList successfully uninstalled.";
exit 0;