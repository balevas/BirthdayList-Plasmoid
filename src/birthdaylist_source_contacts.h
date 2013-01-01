#ifndef BIRTHDAYLIST_SOURCE_CONTACTS_H
#define BIRTHDAYLIST_SOURCE_CONTACTS_H

/**
 * @file    birthdaylist_source_contacts.h
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


#include <QDate>
#include <QHash>
#include <QStringList>
#include <QVariant>

namespace KABC {
    class Addressee;
}


struct BirthdayListAddresseeInfo 
{
    QString name;
    QString nickName;
    QString givenName;
    QString email;
    QString homepage;
    QDate birthday;
    QStringList categories;
    QHash<QString, QVariant> customFields;
};


class BirthdayListSource_Contacts : public QObject
{
    Q_OBJECT
public:
    BirthdayListSource_Contacts();
    virtual ~BirthdayListSource_Contacts();

    virtual const QList<BirthdayListAddresseeInfo>& getAllContacts() = 0;
    
protected:
    void fillBirthdayListAddresseeInfo(BirthdayListAddresseeInfo &addresseeInfo, const KABC::Addressee &kabcAddressee);
    
signals:
    void contactsUpdated();
};


#endif //BIRTHDAYLIST_SOURCE_CONTACTS_H