#!/bin/bash

rm -rf ../build/*

EXTRACTRC=extractrc
PREPARETIPS=preparetips
XGETTEXT=xgettext

$EXTRACTRC `find .. -name \*.rc -o -name *.ui -o -name *.kcfg` >> rc.cpp
# $PREPARETIPS > tips.cpp
$XGETTEXT `find .. -name \*.cc -o -name \*.cpp -o -name \*.h` -o $podir/plasma_applet_kbirthdayapplet.pot
