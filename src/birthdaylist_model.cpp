/**
 * @file    birthdaylist_model.cpp
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


#include "birthdaylist_model.h"
#include "birthdaylist_modelentry.h"
#include "birthdaylist_source_akonadi.h"
#include "birthdaylist_source_collections.h"
#include "birthdaylist_source_kabc.h"
#include <KDebug>
#include <KLocalizedString>
#include <QDateTime>
#include <QFile>


BirthdayListModelConfiguration::BirthdayListModelConfiguration() :
eventDataSource(EDS_Akonadi),
akonadiCollectionId(-1),
eventThreshold(30),
highlightThreshold(2),
pastThreshold(2),
showNicknames(true),
showNamedays(true),
namedayDisplayMode(NDM_AggregateEvents),
showAnniversaries(true),
namedayIdentificationMode(NIM_Both),
namedayDateFieldString("Nameday"),
anniversaryFieldString("Anniversary"),
filterType(FT_Off),
customFieldName(""),
customFieldPrefix(""),
filterValue(""),
todayColorSettings(false, QColor(255, 255, 255), true, QColor(128, 0, 0), true),
highlightColorSettings(false, QColor(255, 255, 255), true, QColor(128, 0, 0), false),
pastColorSettings(false, QColor(0, 0, 0), true, QColor(160, 0, 0), true)
{
}


BirthdayListModel::BirthdayListModel() 
: QStandardItemModel(0, 5),
m_source_collections(new BirthdayListSource_Collections()),
m_source_contacts(0)
{
    QStringList headerTitles;
    headerTitles << i18n("Name") << i18n("Age") << i18n("Date") << i18n("When") << "" << "";
    setHorizontalHeaderLabels(headerTitles);

    // configure the timer that will update the model at the next midnight (update entries, timing of events, etc.)
    m_midnightTimer.setSingleShot(true);
    QDateTime nextMidnight = QDateTime(QDate::currentDate()).addDays(1);
    int msecToNextMidnight = 1000 * (1 + QDateTime::currentDateTime().secsTo(nextMidnight));
    m_midnightTimer.setInterval(msecToNextMidnight);

    connect(&m_midnightTimer, SIGNAL(timeout()), this, SLOT(midnightUpdate()));
    m_midnightTimer.start();

    // do the initial update (although the data might not have been read yet)
    updateModel();
}

BirthdayListModel::~BirthdayListModel() 
{
    disconnect(&m_midnightTimer, SIGNAL(timeout()), this, SLOT(midnightUpdate()));
    
    delete m_source_contacts;
    delete m_source_collections;
}

void BirthdayListModel::setConfiguration(BirthdayListModelConfiguration newConf) 
{
    kDebug() << "Applying BirthdayList model configuration";
    // remember the current nameday file and data source so that we don't do unnecessary updates if not necessary
    QString oldNamedayFile = m_conf.curNamedayFile;
    BirthdayListModelConfiguration::EventDataSource oldEventDataSource = m_conf.eventDataSource;
    int oldAkonadiCollectionId = m_conf.akonadiCollectionId;
    
    m_conf = newConf;

    kDebug() << "NAMEDAY FILES:" << oldNamedayFile << ", " << newConf.curNamedayFile;
    if (oldNamedayFile != newConf.curNamedayFile) {
        // read the nameday definitions from the currently selected file
        m_curLangNamedayList.clear();
        
        QFile namedayFile(newConf.curNamedayFile);
        if (namedayFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&namedayFile);
            int readEntries = 0, skippedEntries = 0;
            // skip language string
            stream.readLine();
            while (!stream.atEnd()) {
                QString namedayEntry = stream.readLine();
                int dateIndex = namedayEntry.indexOf(QRegExp("[0-9][0-9]-[0-9][0-9]"), 0);
                if (dateIndex >= 0) {
                    m_curLangNamedayList.insert(namedayEntry.mid(dateIndex, 5), namedayEntry.mid(dateIndex + 5).trimmed());
                    ++readEntries;
                }
                else ++skippedEntries;
            }
            namedayFile.close();
            kDebug() << "Read" << readEntries << "and skipped" << skippedEntries << "nameday entries from" << newConf.curNamedayFile;
        }
        else {
            kDebug() << "Cannot open nameday file" << newConf.curNamedayFile;
        }
    }

    // update contact source
    if (newConf.eventDataSource == BirthdayListModelConfiguration::EDS_Akonadi) {
        // don't re-register to the same collection if it has not been changed
        if (oldEventDataSource != newConf.eventDataSource || oldAkonadiCollectionId != newConf.akonadiCollectionId) {
            kDebug() << "Going to read contact event data from Akonadi collection Id" << newConf.akonadiCollectionId;
            
            if (m_source_contacts) {
                disconnect(m_source_contacts, SIGNAL(contactsUpdated()), this, SLOT(contactCollectionUpdated()));
                delete m_source_contacts;
            }

            BirthdayListSource_Akonadi *source_contacts_akonadi = new BirthdayListSource_Akonadi(*m_source_collections);
            source_contacts_akonadi->setCurrentCollection(newConf.akonadiCollectionId);

            m_source_contacts = source_contacts_akonadi;
            connect(m_source_contacts, SIGNAL(contactsUpdated()), this, SLOT(contactCollectionUpdated()));
        }
    }
    else {
        kDebug() << "Going to read contact event data from the standard KDE Address Book";

        if (m_source_contacts) {
            disconnect(m_source_contacts, SIGNAL(contactsUpdated()), this, SLOT(contactCollectionUpdated()));
            delete m_source_contacts;
        }

        m_source_contacts = new BirthdayListSource_KABC;
        connect(m_source_contacts, SIGNAL(contactsUpdated()), this, SLOT(contactCollectionUpdated()));
    }
    
    refreshContactEvents();
}
    
BirthdayListModelConfiguration BirthdayListModel::getConfiguration() const 
{
    return m_conf;
}

QHash<QString, int> BirthdayListModel::getAkonadiCollections()
{
    return m_source_collections->getAkonadiCollections();
}

void BirthdayListModel::refreshContactEvents() 
{
    // since we are going to re-create all entries again, delete currently existing ones
    kDebug() << "Reading contact sources to create a new BirthdayList Model";

    foreach(AbstractAnnualEventEntry *oldListEntry, m_listEntries) {
        delete oldListEntry;
    }
    m_listEntries.clear();

    // store nameday entries separately (so that they can be aggregated)
    QList<NamedayEntry *> namedayEntries;

    // iterate over the contacts from the contacts source and create appropriate list entries
    if (m_source_contacts != 0) {
        const QList<BirthdayListAddresseeInfo> contacts = m_source_contacts->getAllContacts();
        kDebug() << "Source contains" << contacts.size() << "contacts";
        QListIterator<BirthdayListAddresseeInfo> contactIt(contacts);
        while (contactIt.hasNext()) {
            BirthdayListAddresseeInfo contactInfo = contactIt.next();

            QString contactName = contactInfo.name;
            QString contactNickname = contactInfo.nickName;
            if (m_conf.showNicknames && !contactNickname.isEmpty()) contactName = contactNickname;
            QDate contactBirthday = contactInfo.birthday;
            QDate contactNameday;
            if (m_conf.namedayIdentificationMode == BirthdayListModelConfiguration::NIM_DateField) {
                contactNameday = getContactDateField(contactInfo, m_conf.namedayDateFieldString);
            }
            else if (m_conf.namedayIdentificationMode == BirthdayListModelConfiguration::NIM_GivenName) {
                contactNameday = getNamedayByGivenName(contactInfo.givenName);
            }
            else {
                contactNameday = getContactDateField(contactInfo, m_conf.namedayDateFieldString);
                if (!contactNameday.isValid()) contactNameday = getNamedayByGivenName(contactInfo.givenName);
            }

            QDate contactAnniversary = getContactDateField(contactInfo, m_conf.anniversaryFieldString);
            QString contactEmail = contactInfo.email;
            QString contactUrl = contactInfo.homepage;
            
            if (m_conf.filterType == BirthdayListModelConfiguration::FT_Off) {
                // do nothing; this check comes first as it is the most likely selected option
            } else if (m_conf.filterType == BirthdayListModelConfiguration::FT_Category) {
                if (!contactInfo.categories.contains(m_conf.filterValue)) continue;
            } else if (m_conf.filterType == BirthdayListModelConfiguration::FT_CustomField) {
                if (contactInfo.customFields[QString("Custom_%1").arg(m_conf.customFieldName)] != m_conf.filterValue) continue;
            } else if (m_conf.filterType == BirthdayListModelConfiguration::FT_CustomFieldPrefix) {
                bool filterValueFound = false;
                QString customFieldNamePattern = QString("Custom_%1").arg(m_conf.customFieldPrefix);
                
                QStringList contactFields = contactInfo.customFields.keys();
                foreach (QString field, contactFields) {
                    if (field.startsWith(customFieldNamePattern) && contactInfo.customFields[field] == m_conf.filterValue) {
                        filterValueFound = true;
                        break;
                    }
                }
                
                if (!filterValueFound) continue;
            }
            
            if (contactBirthday.isValid()) {
                m_listEntries.append(new BirthdayEntry(contactName, contactBirthday, contactEmail, contactUrl));
            }
            if (m_conf.showNamedays && contactNameday.isValid()) {
                QDate firstNameday = contactNameday;
                if (contactBirthday.isValid()) {
                    firstNameday.setDate(contactBirthday.year(), contactNameday.month(), contactNameday.day());
                    if (firstNameday < contactBirthday) firstNameday = firstNameday.addYears(1);
                }
                namedayEntries.append(new NamedayEntry(contactName, firstNameday, contactEmail, contactUrl));
            }
            if (m_conf.showAnniversaries && contactAnniversary.isValid()) {
                m_listEntries.append(new AnniversaryEntry(contactName, contactAnniversary, contactEmail, contactUrl));
            }
        }

        // if desired, join together nameday entries from the same day
        if (m_conf.namedayDisplayMode == BirthdayListModelConfiguration::NDM_AggregateEvents || 
            m_conf.namedayDisplayMode == BirthdayListModelConfiguration::NDM_AllCalendarNames) {
            qSort(namedayEntries.begin(), namedayEntries.end(), AbstractAnnualEventEntry::lessThan);
            int curYear = QDate::currentDate().year();
            QMap<QDate, AggregatedNamedayEntry*> aggregatedEntries;

            // if all calendar names are to be shown, prepare entries for the visualised period
            if (m_conf.namedayDisplayMode == BirthdayListModelConfiguration::NDM_AllCalendarNames) {
                QDate initialDate = QDate::currentDate().addDays(-m_conf.pastThreshold);
                QDate finalDate = initialDate.addYears(1);

                for (QDate date=initialDate; date<finalDate; date = date.addDays(1)) {
                    aggregatedEntries[date] = new AggregatedNamedayEntry(getNamedayString(date), date);
                }
            }

            foreach(NamedayEntry *namedayEntry, namedayEntries) {
                const QDate namedayDate = namedayEntry->date();
                QDate curYearDate(curYear, namedayDate.month(), namedayDate.day());

                AggregatedNamedayEntry *aggregatedEntry = 0;
                if (aggregatedEntries.contains(curYearDate))
                    aggregatedEntry = aggregatedEntries[curYearDate];
                else {
                    aggregatedEntry = new AggregatedNamedayEntry(getNamedayString(curYearDate), curYearDate);
                    aggregatedEntries[curYearDate] = aggregatedEntry;
                }
                aggregatedEntry->addNamedayEntry(namedayEntry);
            }

            foreach(AggregatedNamedayEntry *entry, aggregatedEntries) {
                m_listEntries.append(entry);
            }
        } else if (m_conf.namedayDisplayMode == BirthdayListModelConfiguration::NDM_IndividualEvents) {

            foreach(NamedayEntry *entry, namedayEntries) {
                m_listEntries.append(entry);
            }
        }
    }

    // sort the entries by date
    qSort(m_listEntries.begin(), m_listEntries.end(), AbstractAnnualEventEntry::lessThan);

    kDebug() << "" << m_listEntries.size() << "event entries read from the contact source";

    updateModel();
}


void BirthdayListModel::updateModel() 
{
    kDebug() << "Creating new BirthdayList model";

    setRowCount(0);
    QStandardItem *parentItem = invisibleRootItem();

    foreach (const AbstractAnnualEventEntry *entry, m_listEntries) {
        int remainingDays = entry->remainingDays();
        bool showEvent = (remainingDays >= 0 && remainingDays <= m_conf.eventThreshold) ||
                (remainingDays <= 0 && remainingDays >= -m_conf.pastThreshold);

        if (showEvent) {
            QList<QStandardItem*> items;
            entry->createModelItems(items, m_conf.dateFormat);
            for (int i=0; i<items.size(); ++i) setModelItemColors(entry, items[i], i);

            parentItem->appendRow(items);
        }
    }

    kDebug() << "New BirthdayList model contains" << rowCount() << "items";
}

QDate BirthdayListModel::getNamedayByGivenName(QString givenName) 
{
    if (givenName.isEmpty()) return QDate();

    QDate initialDate = QDate::currentDate().addDays(-m_conf.pastThreshold);
    QDate finalDate = initialDate.addYears(1);
    for (QDate date=initialDate; date<finalDate; date = date.addDays(1)) {
        QStringList calendarNames = getNamedayString(date).split(QRegExp("\\W+"), QString::SkipEmptyParts);

        // if the name can be found, return the nameday in the future
        // (if the contact has a birthday, the date will be moved to his first nameday
        // so that the correct age can be shown; othewise we'll know that the age is unknown)
        if (calendarNames.contains(givenName)) return date.addYears(1);
    }

    return QDate();
}

QString BirthdayListModel::getNamedayString(QDate date) 
{
    QString namedayStringEntry = m_curLangNamedayList[date.toString("MM-dd")];
    if (!namedayStringEntry.isEmpty()) return namedayStringEntry;
    else return date.toString(m_conf.dateFormat);
}

QDate BirthdayListModel::getContactDateField(const BirthdayListAddresseeInfo &contactInfo, QString fieldName) 
{
    if (contactInfo.customFields.contains(fieldName)) return contactInfo.customFields[fieldName].toDate();

    QString kabcCustomFieldName;
    kabcCustomFieldName = QString("Custom_KADDRESSBOOK-%1").arg(fieldName);
    if (contactInfo.customFields.contains(kabcCustomFieldName)) return contactInfo.customFields[kabcCustomFieldName].toDate();
    
    kabcCustomFieldName = QString("Custom_KADDRESSBOOK-X-%1").arg(fieldName);
    if (contactInfo.customFields.contains(kabcCustomFieldName)) return contactInfo.customFields[kabcCustomFieldName].toDate();
    
    return QDate();
}

void BirthdayListModel::setModelItemColors(const AbstractAnnualEventEntry *entry, QStandardItem *item, int colNum) 
{
    item->setEditable(false);

    Q_UNUSED(colNum)
    //if (colNum>0) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    if (entry->remainingDays() == 0) {
        if (entry->hasEvent() || m_conf.todayColorSettings.highlightNoEvents) {
            if (m_conf.todayColorSettings.isForeground) item->setForeground(m_conf.todayColorSettings.brushForeground);
            if (m_conf.todayColorSettings.isBackground) item->setBackground(m_conf.todayColorSettings.brushBackground);
        }
    } else if (entry->remainingDays() < 0) {
        if (entry->hasEvent() || m_conf.pastColorSettings.highlightNoEvents) {
            if (m_conf.pastColorSettings.isForeground) item->setForeground(m_conf.pastColorSettings.brushForeground);
            if (m_conf.pastColorSettings.isBackground) item->setBackground(m_conf.pastColorSettings.brushBackground);
        }
    } else if (entry->remainingDays() <= m_conf.highlightThreshold) {
        if (entry->hasEvent() || m_conf.highlightColorSettings.highlightNoEvents) {
            if (m_conf.highlightColorSettings.isForeground) item->setForeground(m_conf.highlightColorSettings.brushForeground);
            if (m_conf.highlightColorSettings.isBackground) item->setBackground(m_conf.highlightColorSettings.brushBackground);
        }
    }

    for (int row = 0; row < item->rowCount(); ++row) {
        for (int col = 0; col < item->columnCount(); ++col) {
            QStandardItem *child = item->child(row, col);
            if (child) setModelItemColors(entry, child, col);
        }
    }
}

void BirthdayListModel::contactCollectionUpdated()
{
    kDebug() << "Selected contact collection updated, triggering BirthdayList model refresh";
    refreshContactEvents();
}


void BirthdayListModel::midnightUpdate()
{
    kDebug() << "Performing midnight update";
    refreshContactEvents();

    QDateTime nextMidnight = QDateTime(QDate::currentDate()).addDays(1);
    int msecToNextMidnight = 1000 * (1 + QDateTime::currentDateTime().secsTo(nextMidnight));
    m_midnightTimer.setInterval(msecToNextMidnight);
    m_midnightTimer.start();
}
