/**
 * @file    birthdaylist_source_contacts.cpp
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


#include "birthdaylist_source_contacts.h"
#include <KABC/Addressee>


BirthdayListSource_Contacts::BirthdayListSource_Contacts()
{
}

BirthdayListSource_Contacts::~BirthdayListSource_Contacts()
{
}

void BirthdayListSource_Contacts::fillBirthdayListAddresseeInfo(BirthdayListAddresseeInfo &addresseeInfo, const KABC::Addressee &kabcAddressee)
{
    addresseeInfo.name = kabcAddressee.formattedName();
    if (addresseeInfo.name.isEmpty()) addresseeInfo.name = kabcAddressee.assembledName();
    if (addresseeInfo.name.isEmpty()) addresseeInfo.name = kabcAddressee.name();

    addresseeInfo.nickName = kabcAddressee.nickName();
    addresseeInfo.givenName =  kabcAddressee.givenName();
    addresseeInfo.email = kabcAddressee.preferredEmail();
    addresseeInfo.homepage = (kabcAddressee.url().isEmpty() ? "" : kabcAddressee.url().url());

    QDate birthdayDate = kabcAddressee.birthday().date();
    if (birthdayDate.isValid()) {
        addresseeInfo.birthday = birthdayDate;
    }

    addresseeInfo.categories = kabcAddressee.categories();

    for (int i=0; i<kabcAddressee.customs().size(); ++i) {
        int separatorPos = kabcAddressee.customs()[i].indexOf(":");
        QString fieldName = kabcAddressee.customs()[i].left(separatorPos);
        QString fieldValue = kabcAddressee.customs()[i].mid(separatorPos + 1);

        addresseeInfo.customFields.insert(QString("Custom_%1").arg(fieldName), fieldValue);
    }
}
