#ifndef BIRTHDAYLIST_SOURCE_KABC_H
#define BIRTHDAYLIST_SOURCE_KABC_H

/**
 * @file    birthdaylist_source_kabc.h
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


class BirthdayListSource_KABC : public BirthdayListSource_Contacts
{
    Q_OBJECT
public:
    BirthdayListSource_KABC();
    ~BirthdayListSource_KABC();
    
    virtual const QList<BirthdayListAddresseeInfo>& getAllContacts();

private:
    QList<BirthdayListAddresseeInfo> m_contacts;
    
private slots:
    void updateContacts();
};


#endif //BIRTHDAYLIST_SOURCE_KABC_H