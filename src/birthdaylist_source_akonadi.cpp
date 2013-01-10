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


BirthdayList::Source_Akonadi::Source_Akonadi(const BirthdayList::Source_Collections &sourceCollections)
: m_sourceCollections(sourceCollections),
m_session(new Akonadi::Session("BirthdayList_Source_Akonadi", this)),
m_currentCollectionId(-1),
m_registeredCollectionId(-1),
m_monitorAddressBook(0),
m_contactsModel(0)
{
    // register for notifications when there are changes in the list of Akonadi collections
    connect(&m_sourceCollections, SIGNAL(collectionsUpdated()), this, SLOT(collectionsUpdated()));
}

BirthdayList::Source_Akonadi::~Source_Akonadi()
{
    disconnect(&m_sourceCollections, SIGNAL(collectionsUpdated()), this, SLOT(collectionsUpdated()));
    unregisterFromCurrentCollection();
    delete m_session;
}

void BirthdayList::Source_Akonadi::setCurrentCollection(Akonadi::Collection::Id collectionId) 
{
    // make sure that we're not currently registering in the 'old' collection
    // (this could be done on startup when the collections are being loaded in the background)
    m_collectionRegistrationMutex.lock();

    if (collectionId != m_currentCollectionId) {
        unregisterFromCurrentCollection();
        m_currentCollectionId = collectionId;
        tryRegisteringInCurrentCollection();
    }

    m_collectionRegistrationMutex.unlock();
}


const QList<BirthdayList::AddresseeInfo>& BirthdayList::Source_Akonadi::getAllContacts()
{
    return m_contacts;
}

void BirthdayList::Source_Akonadi::tryRegisteringInCurrentCollection() 
{
    if (m_currentCollectionId != m_registeredCollectionId) {
        const Akonadi::Collection newCollection = m_sourceCollections.getAkonadiCollection(m_currentCollectionId);
        if (newCollection.isValid()) {
            kDebug() << "Connecting to Akonadi collection" << newCollection.id() << newCollection.resource() << newCollection.name();
            registerInCollection(newCollection);
            m_registeredCollectionId = m_currentCollectionId;
        }
        else {
            kDebug() << "Can't connect to Akonadi collection" << m_currentCollectionId << ", the collection model is not valid yet";
        }
    }
    else {
        kDebug() << "Already connected to Akonadi collection" << m_currentCollectionId;
    }
}

void BirthdayList::Source_Akonadi::registerInCollection(const Akonadi::Collection &akonadiCollection) 
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
    connect(m_contactsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    connect(m_contactsModel, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(rowsInserted(QModelIndex, int, int)));
    connect(m_contactsModel, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(rowsRemoved(QModelIndex, int, int)));
}

void BirthdayList::Source_Akonadi::unregisterFromCurrentCollection() 
{
    if (m_contactsModel != 0) {
        kDebug() << "Disconnecting from Akonadi collection" << m_currentCollectionId;
        m_registeredCollectionId = -1;
        
        disconnect(m_contactsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        disconnect(m_contactsModel, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(rowsInserted(QModelIndex, int, int)));
        disconnect(m_contactsModel, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(rowsRemoved(QModelIndex, int, int)));

        delete m_contactsModel;
        delete m_monitorAddressBook;
        
        m_contactsModel = 0;
        m_monitorAddressBook = 0;
    }
}

void BirthdayList::Source_Akonadi::collectionsUpdated()
{
    // synchronize this call with collection update (at startup or when changing the collection using config UI)
    m_collectionRegistrationMutex.lock();
    
    kDebug() << "Update of the collections model triggered";
    tryRegisteringInCurrentCollection();
    
    m_collectionRegistrationMutex.unlock();
}

void BirthdayList::Source_Akonadi::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    kDebug() << "Akonadi EntityTreeModel data changed between" << topLeft.row() << "and" << bottomRight.row();
    updateContacts();
}

void BirthdayList::Source_Akonadi::rowsInserted(const QModelIndex& parent, int start, int end)
{
    kDebug() << "Akonadi EntityTreeModel rows inserted between" << start << "and" << end << " under parent" << parent.internalId();
    updateContacts();
}

void BirthdayList::Source_Akonadi::rowsRemoved(const QModelIndex& parent, int start, int end)
{
    kDebug() << "Akonadi EntityTreeModel rows removed between" << start << "and" << end << " under parent" << parent.internalId();
    updateContacts();
}

void BirthdayList::Source_Akonadi::updateContacts() 
{
    kDebug() << "Update of the contact model triggered";
    
    m_contacts.clear();
    dumpContactChildren(0, QModelIndex());
    
    kDebug() << "Read" << m_contacts.size() << "entries from the current Akonadi collection";

    emit contactsUpdated();
}

void BirthdayList::Source_Akonadi::dumpContactChildren(int level, const QModelIndex &parent) 
{
    for (int i=0; i<m_contactsModel->rowCount(parent); ++i) {
        
        QModelIndex index = m_contactsModel->index(i,0,parent);
        Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        
        if (item.hasPayload<KABC::Addressee>()) {
            KABC::Addressee kabcAddressee = item.payload<KABC::Addressee>();
            //kDebug() << "Name" << kabcAddressee.name() << ", Birthday " << kabcAddressee.birthday() << ", parent col" << item.parentCollection().id();
            AddresseeInfo addresseeInfo;

            fillAddresseeInfo(addresseeInfo, kabcAddressee);
            
            m_contacts.append(addresseeInfo);
        }
        
        if (m_contactsModel->hasChildren(index)) dumpContactChildren(level+1, index);
    }
}
