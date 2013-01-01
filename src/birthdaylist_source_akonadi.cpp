/**
 * @file    birthdaylist_source_akonadi.cpp
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


#include "birthdaylist_source_akonadi.h"
#include "birthdaylist_source_collections.h"
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Session>
#include <KABC/Addressee>


BirthdayListSource_Akonadi::BirthdayListSource_Akonadi(const BirthdayListSource_Collections &sourceCollections)
: m_sourceCollections(sourceCollections),
m_session(new Akonadi::Session("BirthdayListSource_Akonadi", this)),
m_currentCollectionId(-1),
m_monitorAddressBook(0),
m_contactsModel(0)
{
    // register for notifications when there are changes in the list of Akonadi collections
    connect(&m_sourceCollections, SIGNAL(collectionsUpdated()), this, SLOT(collectionsUpdated()));
}

BirthdayListSource_Akonadi::~BirthdayListSource_Akonadi()
{
    disconnect(&m_sourceCollections, SIGNAL(collectionsUpdated()), this, SLOT(collectionsUpdated()));
    unregisterFromCurrentCollection();
    delete m_session;
}

void BirthdayListSource_Akonadi::setCurrentCollection(Akonadi::Collection::Id collectionId) 
{
    // make sure that we're not currently registering in the 'old' collection
    // (this could be done on startup when the collections are being loaded in the background)
    m_collectionRegistrationMutex.lock();

    if (collectionId == m_currentCollectionId) {
        return;
    }

    unregisterFromCurrentCollection();
    m_currentCollectionId = collectionId;
    
    tryRegisteringInCurrentCollection();

    m_collectionRegistrationMutex.unlock();
}


const QList<BirthdayListAddresseeInfo>& BirthdayListSource_Akonadi::getAllContacts()
{
    return m_contacts;
}

void BirthdayListSource_Akonadi::tryRegisteringInCurrentCollection() 
{
    const Akonadi::Collection newCollection = m_sourceCollections.getAkonadiCollection(m_currentCollectionId);
    if (newCollection.isValid()) {
        kDebug() << "Connecting to collection" << newCollection.id() << newCollection.resource() << newCollection.name();
        registerInCollection(newCollection);
    }
    else {
        kDebug() << "Can't connect to collection" << m_currentCollectionId << ", the collection model is not valid yet";
    }
}

void BirthdayListSource_Akonadi::registerInCollection(const Akonadi::Collection &akonadiCollection) 
{
    m_monitorAddressBook = new Akonadi::ChangeRecorder(this);
    m_monitorAddressBook->setSession(m_session);
    m_monitorAddressBook->setCollectionMonitored(akonadiCollection);
    m_monitorAddressBook->setMimeTypeMonitored(KABC::Addressee::mimeType());
    Akonadi::ItemFetchScope scopeAddressBook;
    scopeAddressBook.fetchFullPayload(true);
    scopeAddressBook.fetchAllAttributes(true);
    m_monitorAddressBook->setItemFetchScope(scopeAddressBook);

    m_contactsModel = new Akonadi::EntityTreeModel(m_monitorAddressBook, this);
    connect(m_contactsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(updateContacts()) );
    connect(m_contactsModel, SIGNAL(rowsInserted (QModelIndex, int, int )), this, SLOT(updateContacts()) );
    connect(m_contactsModel, SIGNAL(rowsRemoved (QModelIndex, int, int )), this, SLOT(updateContacts()) );
}

void BirthdayListSource_Akonadi::unregisterFromCurrentCollection() 
{
    if (m_contactsModel != 0) {
        kDebug() << "Disconnecting from collection" << m_currentCollectionId;
        
        disconnect(m_contactsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(updateContacts()) );
        disconnect(m_contactsModel, SIGNAL(rowsInserted (QModelIndex, int, int )), this, SLOT(updateContacts()) );
        disconnect(m_contactsModel, SIGNAL(rowsRemoved (QModelIndex, int, int )), this, SLOT(updateContacts()) );

        delete m_contactsModel;
        delete m_monitorAddressBook;
        m_contactsModel = 0;
        m_monitorAddressBook = 0;
    }
}

void BirthdayListSource_Akonadi::collectionsUpdated()
{
    // synchronize this call with collection update (at startup or when changing the collection using config UI)
    m_collectionRegistrationMutex.lock();
    
    kDebug() << "Update of the collections model triggered";
    tryRegisteringInCurrentCollection();
    
    m_collectionRegistrationMutex.unlock();
}

void BirthdayListSource_Akonadi::updateContacts() 
{
    kDebug() << "Update of the contact model triggered";
    
    m_contacts.clear();
    dumpContactChildren(0, QModelIndex());
    
    kDebug() << "Read" << m_contacts.size() << "entries from the current Akonadi collection";

    emit contactsUpdated();
}

void BirthdayListSource_Akonadi::dumpContactChildren(int level, const QModelIndex &parent) 
{
    for (int i=0; i<m_contactsModel->rowCount(parent); ++i) {
        
        QModelIndex index = m_contactsModel->index(i,0,parent);
        Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        
        if (item.hasPayload<KABC::Addressee>()) {
            KABC::Addressee kabcAddressee = item.payload<KABC::Addressee>();
            //kDebug() << "Name" << kabcAddressee.name() << ", Birthday " << kabcAddressee.birthday() << ", parent col" << item.parentCollection().id();
            BirthdayListAddresseeInfo addresseeInfo;

            fillBirthdayListAddresseeInfo(addresseeInfo, kabcAddressee);
            
            m_contacts.append(addresseeInfo);
        }
        
        if (m_contactsModel->hasChildren(index)) dumpContactChildren(level+1, index);
    }
}
