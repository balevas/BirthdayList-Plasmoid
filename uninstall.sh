#!/bin/bash

cd build
( sudo make uninstall ) || { echo "BirthdayList uninstallation failed"; exit 1; }

echo "BirthdayList successfully uninstalled.";
exit 0;