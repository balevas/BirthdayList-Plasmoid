/* ************************************************
 *  @name: birthdaylist_dataengine.cpp 
 *  @author: Karol Slanina
 * ********************************************** */

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 or (at your option)    *
 *   any later version.                                                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   General Public Licencse for more details.                             *
 *                                                                         *
 *   A copy of the license should be part of the package. If not you can   *
 *   find it here: http://www.gnu.org/licenses/gpl.html                    *
 *                                                                         *
 ***************************************************************************/

#include "birthdaylist_dataengine.h"

#include <KDebug>
#include <KStandardDirs>
#include <kabc/stdaddressbook.h>
#include <kabc/field.h>

#include <QFile>


K_EXPORT_PLASMA_DATAENGINE(birthdaylist, BirthdayListDataEngine);


const QString BirthdayListDataEngine::source_kabcContactInfo("KabcContactInfo");
const QString BirthdayListDataEngine::source_kabcNamedayStringPrefix("KabcNamedayString_");
const QString BirthdayListDataEngine::source_kabcAnniversaryStringPrefix("KabcAnniversaryString_");
const QString BirthdayListDataEngine::source_namedayLists("NamedayLists");
const QString BirthdayListDataEngine::source_namedayListPrefix("NamedayList_");

BirthdayListDataEngine::BirthdayListDataEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args)
{
    Q_UNUSED(args)
    
    kabcAnniversaryField = "X-Anniversary";
    kabcNamedayField = "X-Nameday";

    // connect the signal indicating changes in the KDE Address Book with a slot of this
    connect(KABC::StdAddressBook::self(), SIGNAL(addressBookChanged(AddressBook*)), this, SLOT(slotAddressbookChanged()));
}

BirthdayListDataEngine::~BirthdayListDataEngine()
{
}

QStringList BirthdayListDataEngine::sources() const
{
    QStringList sources;
    sources << source_namedayLists;
    sources << source_kabcContactInfo;

    return sources;
}

bool BirthdayListDataEngine::sourceRequestEvent(const QString &name)
{
    if (name == source_kabcContactInfo ||
        name == source_namedayLists ||
        name.startsWith(source_namedayListPrefix) ||
        name.startsWith(source_kabcNamedayStringPrefix) ||
        name.startsWith(source_kabcAnniversaryStringPrefix)) {
        return updateSourceEvent(name);
    }
    else return false;
}

bool BirthdayListDataEngine::updateSourceEvent(const QString &name)
{
    if (name == source_kabcContactInfo) {
        return updateKabcContactInfo();
    } else if (name == source_namedayLists) {
        return updateNamedayLists();
    } else if (name.startsWith(source_namedayListPrefix)) {
        return updateNamedayList(name);
    } else if (name.startsWith(source_kabcNamedayStringPrefix)) {
        setKabcNamedayField(name.mid(source_kabcNamedayStringPrefix.size()));
    } else if (name.startsWith(source_kabcAnniversaryStringPrefix)) {
        setKabcAnniversaryField(name.mid(source_kabcAnniversaryStringPrefix.size()));
    }

    return false;
}

//
// private slots
//
void BirthdayListDataEngine::slotAddressbookChanged()
{
    updateKabcContactInfo();
}

//
// private
//
bool BirthdayListDataEngine::updateNamedayLists()
{
    QStringList fileNames = KGlobal::dirs()->findAllResources("data", "plasma_engine_birthdaylist/namedaydefs/namedays_*.txt");
    if (fileNames.isEmpty() ) {
        kDebug() << "Couldn't find any nameday list files";
        return false;
    }

    foreach (QString fileName, fileNames) {
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
        }
        else {
            kDebug() << "Cannot read language string from " << fileName;
            continue;
        }

        kDebug() << "Registering nameday file" << namedayDefinitionInfo["File"].toString() 
                 << "for langcode" << namedayDefinitionKey << "language" << namedayDefinitionInfo["Language"].toString();
        setData(source_namedayLists, namedayDefinitionKey, namedayDefinitionInfo);
    }

    return true;
}

bool BirthdayListDataEngine::updateNamedayList(QString sourceName)
{
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

void BirthdayListDataEngine::setKabcAnniversaryField(QString kabcAnniversaryField) 
{ 
    this->kabcAnniversaryField = QString("X-%1").arg(kabcAnniversaryField);
    kDebug() << "Setting anniversary string to" << this->kabcAnniversaryField;
    updateKabcContactInfo();
}

void BirthdayListDataEngine::setKabcNamedayField(QString kabcNamedayField) 
{ 
    this->kabcNamedayField = QString("X-%1").arg(kabcNamedayField); 
    kDebug() << "Setting nameday string to" << this->kabcNamedayField;
    updateKabcContactInfo();
}

bool BirthdayListDataEngine::updateKabcContactInfo()
{
    /*
    // Unfortunately KABC does not show all fields available in Address Book. Anniversary field is not provided by the loop below.
    QList<KABC::Field*> fields = KABC::Field::allFields();
    kDebug() << "KABC fields: ";
    for (int i=0; i<fields.size(); ++i) {
        kDebug() << "Field #" << i << ": " << fields[i]->label() << ", " << KABC::Field::categoryLabel(fields[i]->category());
    }*/

    // scan all addressbook entries
    KABC::AddressBook *kabcAddressBook = KABC::StdAddressBook::self();
    int readEntries = 0, skippedEntries = 0;
    
    for (KABC::AddressBook::Iterator it = kabcAddressBook->begin(); it != kabcAddressBook->end(); ++it) {
        KABC::Addressee kabcAddressee = *it;
        QHash<QString, QVariant> kabcContactInfo;
        bool entryValid = false;

        QString kabcAddresseeName = getAddresseeName(kabcAddressee);
        kabcContactInfo.insert("Name", kabcAddresseeName);
        
        QDate birthdayDate = kabcAddressee.birthday().date();
        if (birthdayDate.isValid()) {
            kabcContactInfo.insert("Birthday", birthdayDate);
            entryValid = true;
        }
        QDate namedayDate = QDate::fromString(kabcAddressee.custom("KADDRESSBOOK", kabcNamedayField), Qt::ISODate);
        if (namedayDate.isValid()) {
            kabcContactInfo.insert("Nameday", namedayDate);
            entryValid = true;
        }
        QDate anniversaryDate = QDate::fromString(kabcAddressee.custom("KADDRESSBOOK", kabcAnniversaryField), Qt::ISODate);
        if (anniversaryDate.isValid()) {
            kabcContactInfo.insert("Anniversary", anniversaryDate);
            entryValid = true;
        }

        if (entryValid) {
            setData(source_kabcContactInfo, kabcAddressee.uid(), kabcContactInfo);
            ++readEntries;
        } else {
            ++skippedEntries;
        }
    }

    kDebug() << "Read" << readEntries << "entries from KADDRESSBOOK," << skippedEntries << "were skipped due to missing dates";
    return true;
}


QString BirthdayListDataEngine::getAddresseeName(KABC::Addressee &kabcAddressee)
{
    if (!kabcAddressee.nickName().isEmpty()) {
        return kabcAddressee.nickName();
    } else if (!kabcAddressee.formattedName().isEmpty()) {
        return kabcAddressee.formattedName();
    } else if (!kabcAddressee.familyName().isEmpty()) {
        if (!kabcAddressee.name().isEmpty()) {
            return QString("%1, %2").arg(kabcAddressee.familyName()).arg(kabcAddressee.name());
        } else {
            return kabcAddressee.familyName();
        }
    } else if (!kabcAddressee.name().isEmpty()) {
        return kabcAddressee.name();
    } else {
        return i18n("no name available");
    }
}


#include "birthdaylist_dataengine.moc"
