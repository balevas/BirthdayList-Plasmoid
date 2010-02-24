#include "bn_dataengine.h"

/* ************************************************
 *  @name: bn_dataengine.cpp 
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


// kde headers
#include <KDebug>
#include <KStandardDirs>

#include <QFile>

// make KabEntry available for 
Q_DECLARE_METATYPE(KabEntry)

// definition of the two sources, the engine provides
static const char *BIRTHDAY_SOURCE    = "Birthdays";
static const char *ANNIVERSARY_SOURCE = "Anniversaries";
static QString source_namedayLists("NamedayLists");
static QString source_namedayDefPrefix("Namedays_");

KabcEngine::KabcEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args),
    m_pBirthdayList(0),
    m_pAnniversaryList(0)
{
    Q_UNUSED(args)

    // get instance of the standard K address book
    m_pAddressbook =  KABC::StdAddressBook::self();
    // and connect the signal indicating changes with a slot of this
    connect( m_pAddressbook, SIGNAL( addressBookChanged(AddressBook*) ),
            this, SLOT( slotAddressbookChanged(AddressBook*) ) );
}

KabcEngine::~KabcEngine()
{
    delete m_pBirthdayList;
    delete m_pAnniversaryList;
}


QStringList KabcEngine::sources() const
{
    QStringList sources;
    sources << BIRTHDAY_SOURCE;
    sources << ANNIVERSARY_SOURCE;
    sources << source_namedayLists;

    return sources;
}

bool KabcEngine::sourceRequestEvent(const QString &name)
{
    if(QString::compare(name, BIRTHDAY_SOURCE) == 0 ||
        QString::compare(name, ANNIVERSARY_SOURCE) == 0){
        updateEventList(name);
        return updateSourceEvent(name);
    }
    else if (name == source_namedayLists || name.startsWith("Namedays_")) {
        return updateSourceEvent(name);
    }
    return false;
}

bool KabcEngine::updateSourceEvent(const QString &name)
{
    if(QString::compare(name, BIRTHDAY_SOURCE) == 0){
        setData(I18N_NOOP(name), I18N_NOOP("Birthdays"), *m_pBirthdayList);
    }else if(QString::compare(name, ANNIVERSARY_SOURCE) == 0){
        setData(I18N_NOOP(name), I18N_NOOP("Anniversaries"), *m_pAnniversaryList);
    } else if (name == source_namedayLists) {
      return updateNamedayLists();
    } else if (name.startsWith(source_namedayDefPrefix)) {
      return updateNamedayDef(name);
    }
    
    return true;
}


bool KabcEngine::updateNamedayLists()
{
    QStringList fileNames = KGlobal::dirs()->findAllResources("data", "plasma_engine_birthdays_namedays/namedaydefs/namedays_*.txt");
    if (fileNames.isEmpty() ) {
	kDebug() << "Couldn't find any nameday definition files";
	return false;
    }

    foreach (QString fileName, fileNames) {
	kDebug() << "Processing nameday definition file " << fileName;
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

        setData(source_namedayLists, namedayDefinitionKey, namedayDefinitionInfo);
    }

    return true;
}

bool KabcEngine::updateNamedayDef(QString sourceName)
{
  QString langCode = sourceName.mid(9);
  kDebug() << "Updating nameday strings for " << langCode;
  QVariant namedayFileRecord = query(source_namedayLists)[langCode].toHash()["File"];
  if (!namedayFileRecord.isValid()) {
    kDebug() << "Cannot find nameday strings for " << langCode;
    return false;
  }

  QString namedayFileName = namedayFileRecord.toString();
  QFile namedayFile(namedayFileName);
  if (!namedayFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    kDebug() << "Cannot read nameday strings from " << namedayFileName;
    return false;
  }

  QTextStream stream(&namedayFile);
  // skip language string
  stream.readLine();
  while (!stream.atEnd()) {
    QString namedayEntry = stream.readLine();
    int dateIndex = namedayEntry.indexOf(QRegExp("[0-9][0-9]-[0-9][0-9]"), 0);
    if (dateIndex == -1) {
      //kDebug() << "Skipping line " << namedayEntry;
      continue;
    }
    setData(sourceName, namedayEntry.mid(dateIndex, 5), namedayEntry.mid(dateIndex + 5).trimmed());
  }

  namedayFile.close();
  return true;
}

//
// private slots
//

void KabcEngine::slotAddressbookChanged(AddressBook*)
{
    updateEventList(BIRTHDAY_SOURCE);
    updateSourceEvent(BIRTHDAY_SOURCE);
    updateEventList(ANNIVERSARY_SOURCE);
    updateSourceEvent(ANNIVERSARY_SOURCE);
}

//
// private
//
void KabcEngine::updateEventList(const QString &name)
{
    KabEventList* list;
    bool birthday = false;
    if(QString::compare(name, BIRTHDAY_SOURCE) == 0){
        delete m_pBirthdayList;
        m_pBirthdayList = new KabEventList;
        list = m_pBirthdayList;
        birthday = true;
    }else if(QString::compare(name, ANNIVERSARY_SOURCE) == 0){
        delete m_pAnniversaryList;
        m_pAnniversaryList = new KabEventList;
        list = m_pAnniversaryList;
    }else{
        return;
    }
    KABC::Addressee entry;
    KABC::AddressBook::Iterator it;

    // scan all addressbook entries
    for( it = m_pAddressbook->begin(); it != m_pAddressbook->end(); ++it )
    {
        entry = *it;

        QDate eventDate;
        if(birthday){
            eventDate = entry.birthday().date();
        }else{
            eventDate = getAnniversary(entry);
        }

        if(eventDate.isValid()){
            QString name;
            if( ! entry.nickName().isEmpty() ){
                name = entry.nickName();
            }else if( ! entry.formattedName().isEmpty() ){
                name = entry.formattedName();
            }else{
                // test if lastname is set
                if( ! entry.familyName().isEmpty() ){
                    // test if firstname is set
                    if( ! entry.name().isEmpty() )
                        // set lastname, firstname as text
                        name = QString( "  " + entry.familyName() + ", " + entry.name() );
                    else
                        // set lastnam as text
                        name = QString( "  " + entry.familyName() );
                }else{
                    // test if firstname is set
                    if( ! entry.name().isEmpty() )
                        // set firstname as text
                        name = QString( "  " + entry.name() );
                    else
                        // set dummy as text
                        name = i18n("  no name available");
                }
            }
            KabEntry e = KabEntry(name, eventDate);
            QVariant entry;
            entry.setValue(e);
            list->append(entry);
        }
    }
}


QDate KabcEngine::getAnniversary( const KABC::Addressee &addressee )
{
    QString anniversary = addressee.custom( "KADDRESSBOOK", "X-Anniversary" );

    if( anniversary.isEmpty() )
        return QDate();
    else
        return QDate::fromString( anniversary, Qt::ISODate );
}

#include "bn_dataengine.moc"
