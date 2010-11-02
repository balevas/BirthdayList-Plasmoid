/**
 * @file    birthdaylist_dataengine.cpp
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

#include "birthdaylist_akonadi_dataengine.h"

#include <QFile>
#include <QTimer>

#include <KDebug>
#include <KStandardDirs>

#include <kabc/stdaddressbook.h>
#include <kabc/field.h>

// Akonadi
#include <akonadi/servermanager.h>
#include <akonadi/control.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>

K_EXPORT_PLASMA_DATAENGINE(birthdaylist_akonadi, BirthdayListAkonadiDataEngine);


const QString BirthdayListAkonadiDataEngine::source_akonadiCollections("Collections");
const QString BirthdayListAkonadiDataEngine::source_contactListPrefix("Contacts_");

BirthdayListAkonadiDataEngine::BirthdayListAkonadiDataEngine(QObject* parent, const QVariantList& args)
: Plasma::DataEngine(parent, args) {
    m_akonadiMonitor = 0;

    Akonadi::ServerManager::start();
    QTimer::singleShot(100, this, SLOT(initAkonadi()));
}

BirthdayListAkonadiDataEngine::~BirthdayListAkonadiDataEngine() {
    delete m_akonadiMonitor;
}

QStringList BirthdayListAkonadiDataEngine::sources() const {
    QStringList sources;
    sources << source_akonadiCollections;

    return sources;
}

void BirthdayListAkonadiDataEngine::initAkonadi()
{
    kDebug() << "Starting Akonadi";
    Akonadi::Control::start();

    if (Akonadi::ServerManager::isRunning()) {
        kDebug() << "Akonadi already running";
        m_akonadiMonitor = new Akonadi::Monitor();
        //m_akonadiMonitor->setAllMonitored(true);
        connect(m_akonadiMonitor, SIGNAL(itemChanged (const Akonadi::Item &, const QSet< QByteArray > &)), this, SLOT(slotCollectionChanged()));
        connect(m_akonadiMonitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)), this, SLOT(slotCollectionChanged()));
        connect(m_akonadiMonitor, SIGNAL(itemRemoved(const Akonadi::Item &)), this, SLOT(slotCollectionChanged()));
        
        connect(Akonadi::ServerManager::self(), SIGNAL(started()), this, SLOT(slotAkonadiStarted()));
        connect(Akonadi::ServerManager::self(), SIGNAL(stopped()), this, SLOT(slotAkonadiStopped()));

        // now that Akonadi is up, see what collections are available
        updateCollectionList();
        // read the contacts from the required collection, this will also update the visualisation
        updateAllContactLists();
    }
    else {
        kDebug() << "Akonadi not running, going to check again in 5s";
        QTimer::singleShot(5000, this, SLOT(initAkonadi()));
    }
}

bool BirthdayListAkonadiDataEngine::sourceRequestEvent(const QString &name) {
    if (name == source_akonadiCollections || name.startsWith(source_contactListPrefix))
        return updateSourceEvent(name);
    else return false;
}

bool BirthdayListAkonadiDataEngine::updateSourceEvent(const QString &name) {
    if (name == source_akonadiCollections) {
        return updateCollectionList();
    } else if (name.startsWith(source_contactListPrefix)) {
        return updateContactList(name.mid(source_contactListPrefix.size()));
    }

    return false;
}

void BirthdayListAkonadiDataEngine::slotCollectionChanged() {
    updateCollectionList();
    updateAllContactLists();
}

void BirthdayListAkonadiDataEngine::slotAkonadiStarted() {
    kDebug() << "BirthdayList Akonadi data engine detected Akonadi start";

    m_akonadiMonitor = new Akonadi::Monitor();
    //m_akonadiMonitor->setAllMonitored(true);
    connect(m_akonadiMonitor, SIGNAL(itemChanged (const Akonadi::Item &, const QSet< QByteArray > &)), this, SLOT(slotCollectionChanged()));
    connect(m_akonadiMonitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)), this, SLOT(slotCollectionChanged()));
    connect(m_akonadiMonitor, SIGNAL(itemRemoved(const Akonadi::Item &)), this, SLOT(slotCollectionChanged()));

    updateCollectionList();
    updateAllContactLists();
}

void BirthdayListAkonadiDataEngine::slotAkonadiStopped() {
    kDebug() << "BirthdayList Akonadi data engine detected Akonadi stop";
    delete m_akonadiMonitor;
    m_akonadiMonitor = 0;
    
    updateCollectionList();
    updateAllContactLists();
}

bool BirthdayListAkonadiDataEngine::updateCollectionList() {
    removeAllData(source_akonadiCollections);
    setData(source_akonadiCollections, DataEngine::Data());
    m_collectionResourceMap.clear();
    if (m_akonadiMonitor == 0) return true;

    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
            Akonadi::CollectionFetchJob::Recursive);
    if (job->exec()) {
        Akonadi::Collection::List collections = job->collections();

        foreach(const Akonadi::Collection &collection, collections) {
            if (!collection.contentMimeTypes().contains("text/directory")) {
                kDebug() << "Skipping Akonadi collection" << collection.name();
                continue;
            }
            kDebug() << "Akonadi collection: " << collection.name() << ", id" << collection.id() << ", resource" << collection.resource() << ", attributes" << collection.attributes();
            QHash<QString, QVariant> akonadiCollectionInfo;
            akonadiCollectionInfo.insert("Name", collection.name());
            akonadiCollectionInfo.insert("ID", QString::number(collection.id()));
            akonadiCollectionInfo.insert("Resource", collection.resource());

            setData(source_akonadiCollections, QString::number(collection.id()), akonadiCollectionInfo);
            m_collectionIdMap[QString::number(collection.id())] = collection.resource();
            m_collectionResourceMap[collection.resource()] = &collection;
        }
    }

    return true;
}

bool BirthdayListAkonadiDataEngine::updateAllContactLists() {
    foreach (const QString &collectionName, m_viewedCollections) {
        updateContactList(collectionName);
    }
    
    return true;
}
    
bool BirthdayListAkonadiDataEngine::updateContactList(QString collectionName) {
    kDebug() << "Updating contact information for " << collectionName;
    m_viewedCollections.insert(collectionName);
    QString dataSourceName = source_contactListPrefix + collectionName;

    if (m_akonadiMonitor == 0) {
        removeAllData(dataSourceName);
        setData(dataSourceName, DataEngine::Data());
        return true;
    }

    QString resourceName;
    if (m_collectionResourceMap.contains(collectionName)) resourceName = collectionName;
    else if (m_collectionIdMap.contains(collectionName)) resourceName = m_collectionIdMap[collectionName];
    else {
        removeAllData(dataSourceName);
        setData(dataSourceName, DataEngine::Data());
        return true;
    }

    const Akonadi::Collection *collection = m_collectionResourceMap[resourceName];
    if (collection) m_akonadiMonitor->setCollectionMonitored(*collection, true);

    Akonadi::CollectionFetchJob job(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive);
    Akonadi::CollectionFetchScope jobScope;
    jobScope.setResource(resourceName);
    job.setFetchScope(jobScope);

    DataEngine::Data newData;
    if (job.exec()) {
        Akonadi::Collection::List collections = job.collections();
        int readEntries = 0;

        foreach(const Akonadi::Collection &collection, collections) {
            Akonadi::ItemFetchJob ijob(collection);
            ijob.fetchScope().fetchFullPayload();

            if (ijob.exec()) {
                Akonadi::Item::List items = ijob.items();
                foreach(const Akonadi::Item &item, items) {
                    if (item.hasPayload<KABC::Addressee > ()) {
                        KABC::Addressee kabcAddressee = item.payload<KABC::Addressee > ();
                        
                        QHash<QString, QVariant> addresseeInfo;
                        QString addresseeName = kabcAddressee.formattedName();
                        if (addresseeName.isEmpty()) addresseeName = kabcAddressee.assembledName();
                        if (addresseeName.isEmpty()) addresseeName = kabcAddressee.name();
                        
                        addresseeInfo.insert("Name", addresseeName);
                        addresseeInfo.insert("Nickname", kabcAddressee.nickName());
                        addresseeInfo.insert("Given name", kabcAddressee.givenName());
                        addresseeInfo.insert("Email", kabcAddressee.preferredEmail());
                        addresseeInfo.insert("Homepage", kabcAddressee.url());

                        QDate birthdayDate = kabcAddressee.birthday().date();
                        if (birthdayDate.isValid()) {
                            addresseeInfo.insert("Birthday", birthdayDate);
                        }
                        
                        addresseeInfo.insert("Categories", kabcAddressee.categories());
                        
                        for (int i=0; i<kabcAddressee.customs().size(); ++i) {
                            int separatorPos = kabcAddressee.customs()[i].indexOf(":");
                            QString fieldName = kabcAddressee.customs()[i].left(separatorPos);
                            QString fieldValue = kabcAddressee.customs()[i].mid(separatorPos + 1);

                            addresseeInfo.insert(QString("Custom_%1").arg(fieldName), fieldValue);
                        }

                        newData.insert(kabcAddressee.uid(), addresseeInfo);
                        ++readEntries;
                    }
                }
            }
        }

        kDebug() << "Read " << readEntries << " entries from the Akonadi collection " << collectionName;
    }

    removeAllData(dataSourceName);
    setData(dataSourceName, newData);
    return true;
}

#include "birthdaylist_akonadi_dataengine.moc"
