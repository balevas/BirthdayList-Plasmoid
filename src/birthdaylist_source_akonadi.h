#ifndef BIRTHDAYLIST_SOURCE_AKONADI_H
#define BIRTHDAYLIST_SOURCE_AKONADI_H

/**
 * @file    birthdaylist_source_akonadi.h
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
#include <Akonadi/Collection>
#include <QMutex>

class BirthdayListSource_Collections;
namespace Akonadi {
    class ChangeRecorder;
    class EntityTreeModel;
    class Session;
};
class QModelIndex;


class BirthdayListSource_Akonadi : public BirthdayListSource_Contacts
{
    Q_OBJECT
public:
    BirthdayListSource_Akonadi(const BirthdayListSource_Collections &sourceCollections);
    ~BirthdayListSource_Akonadi();
    
    void setCurrentCollection(Akonadi::Collection::Id collectionId);
    
    virtual const QList<BirthdayListAddresseeInfo>& getAllContacts();

private:
    void tryRegisteringInCurrentCollection();
    void registerInCollection(const Akonadi::Collection &akonadiCollection);
    void unregisterFromCurrentCollection();

    void dumpContactChildren(int level, const QModelIndex &parent);

    const BirthdayListSource_Collections &m_sourceCollections;
    Akonadi::Session *m_session;
    QMutex m_collectionRegistrationMutex;
    
    Akonadi::Collection::Id m_currentCollectionId;
    Akonadi::ChangeRecorder *m_monitorAddressBook;
    Akonadi::EntityTreeModel *m_contactsModel;

    QList<BirthdayListAddresseeInfo> m_contacts;
    
private slots:
    void collectionsUpdated();
    void updateContacts();
};


#endif //BIRTHDAYLIST_SOURCE_AKONADI_H