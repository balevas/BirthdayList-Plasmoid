#!/bin/bash

cd dataengine-akonadi/build
( sudo make uninstall ) || { echo "Uninstallation of the Akonadi support for BirthdayList failed"; }

echo "Akonadi support for BirthdayList successfully uninstalled.";
exit 0;