/**
 * @file    birthdaylist_dataengine.cpp
 * @author  Karol Slanina
 * @version 0.6.0
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
const QString BirthdayListAkonadiDataEngine::source_contactInfo("ContactInfo");
const QString BirthdayListAkonadiDataEngine::source_setCurrentCollectionPrefix("SetCurrentCollection_");
const QString BirthdayListAkonadiDataEngine::source_setNamedayFieldPrefix("SetNamedayField_");
const QString BirthdayListAkonadiDataEngine::source_setAnniversaryFieldPrefix("SetAnniversaryField_");

BirthdayListAkonadiDataEngine::BirthdayListAkonadiDataEngine(QObject* parent, const QVariantList& args)
: Plasma::DataEngine(parent, args) {
    m_akonadiMonitor = 0;
    m_currentCollectionResource = "";
    m_anniversaryField = "Anniversary";
    m_namedayField = "Nameday";

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
    sources << source_contactInfo;

    return sources;
}

void BirthdayListAkonadiDataEngine::initAkonadi()
{
    kDebug() << "Starting Akonadi";
    Akonadi::Control::start();

    if (Akonadi::ServerManager::isRunning()) {
        kDebug() << "Akonadi already running";
        m_akonadiMonitor = new Akonadi::Monitor();
        connect(m_akonadiMonitor, SIGNAL(itemChanged (const Akonadi::Item &, const QSet< QByteArray > &)), this, SLOT(slotCollectionChanged()));
        connect(m_akonadiMonitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)), this, SLOT(slotCollectionChanged()));
        connect(m_akonadiMonitor, SIGNAL(itemRemoved(const Akonadi::Item &)), this, SLOT(slotCollectionChanged()));

        connect(Akonadi::ServerManager::self(), SIGNAL(started()), this, SLOT(slotAkonadiStarted()));
        connect(Akonadi::ServerManager::self(), SIGNAL(stopped()), this, SLOT(slotAkonadiStopped()));

        // now that Akonadi is up, see what collections are available
        updateCollectionList();
        // if the visualisation has requested certain collection already, its name has been remembered
        // now complete the setting of this collection
        setCurrentCollection(m_currentCollectionResource);
        // read the contacts from the required collection, this will also update the visualisation
        updateContactInfo();
    }
    else {
        kDebug() << "Akonadi not running, going to check again in 5s";
        QTimer::singleShot(5000, this, SLOT(initAkonadi()));
    }
}

bool BirthdayListAkonadiDataEngine::sourceRequestEvent(const QString &name) {
    if (name == source_contactInfo ||
            name == source_akonadiCollections ||
            name.startsWith(source_setCurrentCollectionPrefix) ||
            name.startsWith(source_setNamedayFieldPrefix) ||
            name.startsWith(source_setAnniversaryFieldPrefix)) {
        return updateSourceEvent(name);
    } else return false;
}

bool BirthdayListAkonadiDataEngine::updateSourceEvent(const QString &name) {
    if (name == source_contactInfo) {
        return updateContactInfo();
    } else if (name == source_akonadiCollections) {
        return updateCollectionList();
    } else if (name.startsWith(source_setCurrentCollectionPrefix)) {
        setCurrentCollection(name.mid(source_setCurrentCollectionPrefix.size()));
    } else if (name.startsWith(source_setNamedayFieldPrefix)) {
        setNamedayField(name.mid(source_setNamedayFieldPrefix.size()));
    } else if (name.startsWith(source_setAnniversaryFieldPrefix)) {
        setAnniversaryField(name.mid(source_setAnniversaryFieldPrefix.size()));
    }

    return false;
}

void BirthdayListAkonadiDataEngine::slotCollectionChanged() {
    updateContactInfo();
}

void BirthdayListAkonadiDataEngine::slotAkonadiStarted() {
    kDebug() << "BirthdayList Akonadi data engine detected Akonadi start";
    m_akonadiMonitor = new Akonadi::Monitor();
    connect(m_akonadiMonitor, SIGNAL(itemChanged (const Akonadi::Item &, const QSet< QByteArray > &)), this, SLOT(slotCollectionChanged()));
    connect(m_akonadiMonitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)), this, SLOT(slotCollectionChanged()));
    connect(m_akonadiMonitor, SIGNAL(itemRemoved(const Akonadi::Item &)), this, SLOT(slotCollectionChanged()));

    updateCollectionList();
    setCurrentCollection(m_currentCollectionResource);
    updateContactInfo();
}

void BirthdayListAkonadiDataEngine::slotAkonadiStopped() {
    kDebug() << "BirthdayList Akonadi data engine detected Akonadi stop";
    delete m_akonadiMonitor;
    m_akonadiMonitor = 0;
    
    updateCollectionList();
    updateContactInfo();
}

bool BirthdayListAkonadiDataEngine::updateCollectionList() {
    removeAllData(source_akonadiCollections);
    setData(source_akonadiCollections, DataEngine::Data());
    m_collectionMap.clear();
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
            m_collectionMap[collection.resource()] = &collection;
        }
    }

    return true;
}

void BirthdayListAkonadiDataEngine::setCurrentCollection(QString resource) {
    this->m_currentCollectionResource = resource;

    if (m_akonadiMonitor == 0) return;

    updateCollectionList();
    const Akonadi::Collection *collection = m_collectionMap[resource];

    if (collection) {
        m_akonadiMonitor->setAllMonitored(false);
        m_akonadiMonitor->setCollectionMonitored(*collection, true);
    }
}

void BirthdayListAkonadiDataEngine::setAnniversaryField(QString anniversaryField) {
    this->m_anniversaryField = anniversaryField;
    updateContactInfo();
}

void BirthdayListAkonadiDataEngine::setNamedayField(QString namedayField) {
    this->m_namedayField = namedayField;
    updateContactInfo();
}

bool BirthdayListAkonadiDataEngine::updateContactInfo() {
    kDebug() << "Updating contact information";
    removeAllData(source_contactInfo);
    setData(source_contactInfo, DataEngine::Data());
    if (m_akonadiMonitor == 0) return true;

    Akonadi::CollectionFetchJob job(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive);
    Akonadi::CollectionFetchScope jobScope;
    jobScope.setResource(m_currentCollectionResource);
    job.setFetchScope(jobScope);

    QList<QVariant> events;
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
                        addresseeInfo.insert("Name", kabcAddressee.formattedName());
                        addresseeInfo.insert("Nickname", kabcAddressee.nickName());
                        addresseeInfo.insert("Given name", kabcAddressee.givenName());

                        QDate birthdayDate = kabcAddressee.birthday().date();
                        if (birthdayDate.isValid()) {
                            addresseeInfo.insert("Birthday", birthdayDate);
                        }
                        // try to read the custom fields with and without "X-" prefix, since both forms can be used
                        QDate namedayDate = QDate::fromString(kabcAddressee.custom("KADDRESSBOOK", m_namedayField), Qt::ISODate);
                        if (!namedayDate.isValid()) namedayDate = QDate::fromString(kabcAddressee.custom("KADDRESSBOOK", QString("X-%1").arg(m_namedayField)), Qt::ISODate);
                        if (namedayDate.isValid()) {
                            // ignore year (set the following year so that it's clear it's nonsense)
                            namedayDate.setDate(QDate::currentDate().year() + 1, namedayDate.month(), namedayDate.day());
                            addresseeInfo.insert("Nameday", namedayDate);
                        }
                        // try to read the custom fields with and without "X-" prefix, since both forms can be used
                        QDate anniversaryDate = QDate::fromString(kabcAddressee.custom("KADDRESSBOOK", m_anniversaryField), Qt::ISODate);
                        if (!anniversaryDate.isValid()) anniversaryDate = QDate::fromString(kabcAddressee.custom("KADDRESSBOOK", QString("X-%1").arg(m_anniversaryField)), Qt::ISODate);
                        if (anniversaryDate.isValid()) {
                            addresseeInfo.insert("Anniversary", anniversaryDate);
                        }

                        setData(source_contactInfo, kabcAddressee.uid(), addresseeInfo);
                        ++readEntries;
                    }
                }
            }
        }
    }

    return true;
}

#include "birthdaylist_akonadi_dataengine.moc"
