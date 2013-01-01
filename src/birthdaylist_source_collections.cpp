/**
 * @file    birthdaylist_source_collections.cpp
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


#include "birthdaylist_source_collections.h"
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionModel>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/Session>
#include <KABC/Addressee>
#include <QModelIndex>


BirthdayList::Source_Collections::Source_Collections()
{
    m_session = new Akonadi::Session("BirthdayListSource_Collections", this);

    Akonadi::ChangeRecorder* monitorCollections = new Akonadi::ChangeRecorder(this);
    monitorCollections->setSession(m_session);
    monitorCollections->setCollectionMonitored(Akonadi::Collection::root());
    monitorCollections->setMimeTypeMonitored(KABC::Addressee::mimeType());

    m_collectionsModel = new Akonadi::EntityTreeModel(monitorCollections, this);
    m_collectionsModel->setCollectionFetchStrategy(Akonadi::EntityTreeModel::FetchCollectionsRecursive);
    m_collectionsModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::NoItemPopulation);
    
    connect(m_collectionsModel, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(updateCollectionsMap()));
    connect(m_collectionsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(updateCollectionsMap()));
}

BirthdayList::Source_Collections::~Source_Collections()
{
}

QHash<QString, int> BirthdayList::Source_Collections::getAkonadiCollections() const
{
    return m_collectionIds;
}

const Akonadi::Collection BirthdayList::Source_Collections::getAkonadiCollection(Akonadi::Collection::Id collectionId) const
{
    if (m_collections.contains(collectionId)) return m_collections[collectionId];
    else return Akonadi::Collection();
}

void BirthdayList::Source_Collections::updateCollectionsMap()
{
    kDebug() << "Akonadi collections list updated";
    m_collectionIds.clear();
    m_collections.clear();
    dumpCollectionChildren(0, QModelIndex());

    emit collectionsUpdated();
}

void BirthdayList::Source_Collections::dumpCollectionChildren(int level, const QModelIndex &parent)
{
    for (int i=0; i<m_collectionsModel->rowCount(parent); ++i) {
        QModelIndex index = m_collectionsModel->index(i,0,parent);
        Akonadi::Collection collection = index.data(Akonadi::CollectionModel::CollectionRole).value<Akonadi::Collection>();
        if (collection.isValid()) {
            QString collectionName = index.data().toString();
            if (index.parent().isValid()) {
                collectionName.prepend(" - ");
                collectionName.prepend(index.parent().data().toString());
            }
            
            kDebug() << "Collection" << index.data().toString() << "(id:" << collection.id() << ", name:" << collection.name() << ", combobox entry:" << collectionName << ")";
            m_collectionIds.insert(collectionName, collection.id());
            m_collections.insert(collection.id(), collection);

            if (m_collectionsModel->hasChildren(index)) dumpCollectionChildren(level+1, index);
        }
    }
}
