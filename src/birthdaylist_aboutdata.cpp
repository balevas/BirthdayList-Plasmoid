/**
 * @file    birthdaylist_aboutdata.cpp
 * @author  Karol Slanina
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 */


#include "birthdaylist_aboutdata.h"
#include <QByteArray>

    
const QByteArray BirthdayList::AboutData::version("1.0");

BirthdayList::AboutData::AboutData()
: KAboutData("birthdaylist_applet", 
             QByteArray(), 
             ki18n("Birthday List"), 
             version, 
             ki18n("Birthday List"), 
             KAboutData::License_GPL,
             ki18n("Copyright (C) 2010-2013 Karol Slanina"),
             ki18n("Shows the list of upcoming birthdays, anniversaries and name days"),
             "http://kde-look.org/content/show.php?content=121134",
             "http://kde-look.org/content/show.php?content=121134")
{
    setProgramIconName("bl_cookie");
    
    addAuthor(ki18n("Karol Slanina"), ki18n("Plasmoid, data engines, Slovak translation"), "karol.slanina@gmail.com");
    
    addCredit(ki18n("Ondřej Kuda"), ki18n("Czech translation"), "ondrej.kuda@gmail.com");
    addCredit(ki18n("Andreas Goldbohm"), ki18n("German translation"), "vongoldi@o2online.de");
    addCredit(ki18n("Richard Bos"), ki18n("Dutch translation"));
    addCredit(ki18n("André Marcelo Alvarenga"), ki18n("Brazilian Portuguese translation"));
    addCredit(ki18n("David Vignoni"), ki18n("Nuvola icons"), "http://www.icon-king.com/projects/nuvola");
    addCredit(ki18n("All people who report bugs, send feedback and new feature requests"));
    
    setTranslator(ki18nc("NAME OF THE TRANSLATORS", "Your names"), ki18nc("EMAIL OF THE TRANSLATORS", "Your emails"));
}

BirthdayList::AboutData::~AboutData()
{
}
