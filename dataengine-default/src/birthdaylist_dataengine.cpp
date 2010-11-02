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

#include "birthdaylist_dataengine.h"

#include <QFile>

#include <KDebug>
#include <KStandardDirs>

#include <kabc/stdaddressbook.h>


K_EXPORT_PLASMA_DATAENGINE(birthdaylist, BirthdayListDataEngine);


const QString BirthdayListDataEngine::source_namedayLists("NamedayLists");
const QString BirthdayListDataEngine::source_namedayListPrefix("NamedayList_");
const QString BirthdayListDataEngine::source_contactInfo("ContactInfo");

BirthdayListDataEngine::BirthdayListDataEngine(QObject* parent, const QVariantList& args)
: Plasma::DataEngine(parent, args) {
    // connect the signal indicating changes in the KDE Address Book with a slot of this
    connect(KABC::StdAddressBook::self(), SIGNAL(addressBookChanged(AddressBook*)), this, SLOT(slotAddressbookChanged()));
}

BirthdayListDataEngine::~BirthdayListDataEngine() {
}

QStringList BirthdayListDataEngine::sources() const {
    QStringList sources;
    sources << source_namedayLists;
    sources << source_contactInfo;

    return sources;
}

bool BirthdayListDataEngine::sourceRequestEvent(const QString &name) {
    if (name == source_contactInfo ||
        name == source_namedayLists ||
        name.startsWith(source_namedayListPrefix)) {
        return updateSourceEvent(name);
    } else return false;
}

bool BirthdayListDataEngine::updateSourceEvent(const QString &name) {
    if (name == source_contactInfo) {
        return updateContactInfo();
    } else if (name == source_namedayLists) {
        return updateNamedayLists();
    } else if (name.startsWith(source_namedayListPrefix)) {
        return updateNamedayList(name);
    }

    return false;
}

void BirthdayListDataEngine::slotAddressbookChanged() {
    updateContactInfo();
}

bool BirthdayListDataEngine::updateNamedayLists() {
    QStringList fileNames = KGlobal::dirs()->findAllResources("data", "plasma_engine_birthdaylist/namedaydefs/namedays_*.txt");
    if (fileNames.isEmpty()) {
        kDebug() << "Couldn't find any nameday list files";
        return false;
    }

    foreach(QString fileName, fileNames) {
        int langPos = fileName.lastIndexOf("/namedays_") + 10;
        QString namedayDefinitionKey = fileName.mid(langPos);
        namedayDefinitionKey.chop(4);

        QHash<QString, QVariant> namedayDefinitionInfo;
        namedayDefinitionInfo.insert("File", fileName);

        QFile namedayFile(fileName);
        if (namedayFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&namedayFile);
            namedayDefinitionInfo.insert("Language", stream.readLine());
            namedayFile.close();
        } else {
            kDebug() << "Cannot read language string from " << fileName;
            continue;
        }

        kDebug() << "Registering nameday file" << namedayDefinitionInfo["File"].toString()
                << "for langcode" << namedayDefinitionKey << "language" << namedayDefinitionInfo["Language"].toString();
        setData(source_namedayLists, namedayDefinitionKey, namedayDefinitionInfo);
    }

    return true;
}

bool BirthdayListDataEngine::updateNamedayList(QString sourceName) {
    QString langCode = sourceName.mid(source_namedayListPrefix.size());
    QVariant namedayFileRecord = query(source_namedayLists)[langCode].toHash()["File"];
    if (!namedayFileRecord.isValid()) {
        kDebug() << "No registered nameday file for" << langCode;
        return false;
    }

    QString namedayFileName = namedayFileRecord.toString();
    QFile namedayFile(namedayFileName);
    if (!namedayFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        kDebug() << "Cannot open nameday file" << namedayFileName;
        return false;
    }

    QTextStream stream(&namedayFile);
    int readEntries = 0, skippedEntries = 0;
    // skip language string
    stream.readLine();
    while (!stream.atEnd()) {
        QString namedayEntry = stream.readLine();
        int dateIndex = namedayEntry.indexOf(QRegExp("[0-9][0-9]-[0-9][0-9]"), 0);
        if (dateIndex == -1) {
            //kDebug() << "Skipping line " << namedayEntry;
            ++skippedEntries;
            continue;
        }
        setData(sourceName, namedayEntry.mid(dateIndex, 5), namedayEntry.mid(dateIndex + 5).trimmed());
        ++readEntries;
    }

    namedayFile.close();

    kDebug() << "Read" << readEntries << "and skipped" << skippedEntries << "nameday entries for langcode" << langCode;
    return true;
}

bool BirthdayListDataEngine::updateContactInfo() {
    removeAllData(source_contactInfo);
    
    // scan all addressbook entries
    KABC::AddressBook *kabcAddressBook = KABC::StdAddressBook::self();
    int readEntries = 0;

    for (KABC::AddressBook::Iterator it = kabcAddressBook->begin(); it != kabcAddressBook->end(); ++it) {
        KABC::Addressee kabcAddressee = *it;
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

        setData(source_contactInfo, kabcAddressee.uid(), addresseeInfo);
        ++readEntries;
    }

    qDebug() << "Read" << readEntries << "entries from KDE Address Book";
    return true;
}

#include "birthdaylist_dataengine.moc"
