#!/bin/bash

cd build
( sudo make uninstall ) || { echo "BirthdayList applet uninstallation failed"; }

echo "BirthdayList successfully uninstalled.";
exit 0;