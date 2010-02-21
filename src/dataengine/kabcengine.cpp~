#include "kabcengine.h"

/* ************************************************
 *  @name: kabcengine.cpp 
 *  @author: Meinhard Ritscher and Jan Hambrecht
 *
 *  $Id:  $
 *
 *  See header file for description and history
 *
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

// make KabEntry available for 
Q_DECLARE_METATYPE(KabEntry)

// definition of the two sources, the engine provides
static const char *BIRTHDAY_SOURCE    = "Birthdays";
static const char *ANNIVERSARY_SOURCE = "Anniversaries";

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

    return sources;
}

bool KabcEngine::sourceRequestEvent(const QString &name)
{
    if(QString::compare(name, BIRTHDAY_SOURCE) == 0 ||
        QString::compare(name, ANNIVERSARY_SOURCE) == 0){
        updateEventList(name);
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
    }
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
            if( ! entry.formattedName().isEmpty() ){
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

#include "kabcengine.moc"
