#ifndef BIRTHDAYLIST_SOURCE_COLLECTIONS_H
#define BIRTHDAYLIST_SOURCE_COLLECTIONS_H

/**
 * @file    birthdaylist_source_collections.h
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


#include <Akonadi/Collection>
#include <QHash>

namespace Akonadi {
    class EntityTreeModel;
    class Session;
};
class QModelIndex;


class BirthdayListSource_Collections : public QObject
{
    Q_OBJECT
public:
    BirthdayListSource_Collections();
    ~BirthdayListSource_Collections();
    
    QHash<QString, int> getAkonadiCollections() const;
    const Akonadi::Collection getAkonadiCollection(Akonadi::Collection::Id collectionId) const;

private:
    void dumpCollectionChildren(int level, const QModelIndex &parent);

    Akonadi::Session *m_session;
    Akonadi::EntityTreeModel *m_collectionsModel;

    QHash<QString, int> m_collectionIds;
    QHash<Akonadi::Collection::Id, Akonadi::Collection> m_collections;
    
private slots:
    void updateCollectionsMap();
    
signals:
    void collectionsUpdated();
};


#endif //BIRTHDAYLIST_SOURCE_COLLECTIONS_H