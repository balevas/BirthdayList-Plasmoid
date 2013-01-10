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

namespace BirthdayList {
    class Source_Collections;
};
namespace Akonadi {
    class ChangeRecorder;
    class EntityTreeModel;
    class Session;
};
class QModelIndex;


namespace BirthdayList 
{
    class Source_Akonadi : public Source_Contacts
    {
        Q_OBJECT
    public:
        Source_Akonadi(const Source_Collections &sourceCollections);
        ~Source_Akonadi();
        
        void setCurrentCollection(Akonadi::Collection::Id collectionId);
        
        virtual const QList<AddresseeInfo>& getAllContacts();

    private:
        void tryRegisteringInCurrentCollection();
        void registerInCollection(const Akonadi::Collection &akonadiCollection);
        void unregisterFromCurrentCollection();

        void dumpContactChildren(int level, const QModelIndex &parent);

        const Source_Collections &m_sourceCollections;
        Akonadi::Session *m_session;
        QMutex m_collectionRegistrationMutex;
        
        Akonadi::Collection::Id m_currentCollectionId;
        Akonadi::Collection::Id m_registeredCollectionId;
        Akonadi::ChangeRecorder *m_monitorAddressBook;
        Akonadi::EntityTreeModel *m_contactsModel;

        QList<AddresseeInfo> m_contacts;
        
    private slots:
        void collectionsUpdated();
        void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
        void rowsInserted(const QModelIndex& parent, int start, int end);
        void rowsRemoved(const QModelIndex& parent, int start, int end);
        void updateContacts();
    };
};


#endif //BIRTHDAYLIST_SOURCE_AKONADI_H