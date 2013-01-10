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


BirthdayList::Source_KABC::Source_KABC()
{
    updateContacts();
    connect(KABC::StdAddressBook::self(), SIGNAL(addressBookChanged(AddressBook*)), this, SLOT(updateContacts()));
}

BirthdayList::Source_KABC::~Source_KABC()
{
    disconnect(KABC::StdAddressBook::self(), SIGNAL(addressBookChanged(AddressBook*)), this, SLOT(updateContacts()));
}

const QHash<QString, BirthdayList::AddresseeInfo>& BirthdayList::Source_KABC::getAllContacts()
{
    return m_contacts;
}

void BirthdayList::Source_KABC::updateContacts() 
{
    m_contacts.clear();
    
    // scan all addressbook entries
    KABC::AddressBook *kabcAddressBook = KABC::StdAddressBook::self();
    int readEntries = 0;

    for (KABC::AddressBook::Iterator it = kabcAddressBook->begin(); it != kabcAddressBook->end(); ++it) {
        KABC::Addressee kabcAddressee = *it;
        AddresseeInfo addresseeInfo;
        
        fillAddresseeInfo(addresseeInfo, kabcAddressee);
        
        m_contacts.insert(kabcAddressee.uid(), addresseeInfo);
        ++readEntries;
    }

    kDebug() << "Read" << readEntries << "entries from KDE Address Book";
  
    emit contactsUpdated();
}
