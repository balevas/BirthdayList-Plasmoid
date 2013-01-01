/**
 * @file    birthdaylist_source_kabc.cpp
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


#include "birthdaylist_source_kabc.h"
#include <KABC/Addressee>
#include <KABC/StdAddressBook>


BirthdayListSource_KABC::BirthdayListSource_KABC()
{
    updateContacts();
    connect(KABC::StdAddressBook::self(), SIGNAL(addressBookChanged(AddressBook*)), this, SLOT(updateContacts()));
}

BirthdayListSource_KABC::~BirthdayListSource_KABC()
{
    disconnect(KABC::StdAddressBook::self(), SIGNAL(addressBookChanged(AddressBook*)), this, SLOT(updateContacts()));
}

const QList<BirthdayListAddresseeInfo>& BirthdayListSource_KABC::getAllContacts()
{
    return m_contacts;
}

void BirthdayListSource_KABC::updateContacts() 
{
    m_contacts.clear();
    
    // scan all addressbook entries
    KABC::AddressBook *kabcAddressBook = KABC::StdAddressBook::self();
    int readEntries = 0;

    for (KABC::AddressBook::Iterator it = kabcAddressBook->begin(); it != kabcAddressBook->end(); ++it) {
        KABC::Addressee kabcAddressee = *it;
        BirthdayListAddresseeInfo addresseeInfo;
        
        fillBirthdayListAddresseeInfo(addresseeInfo, kabcAddressee);
        
        m_contacts.append(addresseeInfo);
        ++readEntries;
    }

    kDebug() << "Read" << readEntries << "entries from KDE Address Book";
  
    emit contactsUpdated();
}
