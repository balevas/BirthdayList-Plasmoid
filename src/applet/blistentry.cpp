#include "blistentry.h"
#include <KLocalizedString>

/* ************************************************
 *  @name: blistentry.cpp 
 *  @author: Meinhard Ritscher
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

int AbstractAnnualEventEntry::historyMargin = 7;
QIcon BirthdayEntry::m_icon("/home/karolko/src/plasmoid/birthday/KBirthdayPlasma_0_9_73/icons/cookie.png");
QIcon NamedayEntry::m_icon("/home/karolko/src/plasmoid/birthday/KBirthdayPlasma_0_9_73/icons/date.png");
QIcon AggregatedNamedayEntry::m_icon("/home/karolko/src/plasmoid/birthday/KBirthdayPlasma_0_9_73/icons/date.png");


AbstractAnnualEventEntry::AbstractAnnualEventEntry(const QString &name, const QDate &date)
    :m_name(name), m_date(date)
{
    calculateDays();
}

AbstractAnnualEventEntry::~AbstractAnnualEventEntry()
{
}

void AbstractAnnualEventEntry::calculateDays()
{
    QDate today = QDate::currentDate();

    QDate currentAnniversary = QDate(today.year(), m_date.month(), m_date.day());
    int daysToAnniversary = today.daysTo(currentAnniversary);
    if (daysToAnniversary < -historyMargin) {
      currentAnniversary = QDate(today.year() + 1, m_date.month(), m_date.day());
    }
    else if (daysToAnniversary > today.daysInYear() - historyMargin) {
      currentAnniversary = QDate(today.year() - 1, m_date.month(), m_date.day());
    }

    m_remainingDays = today.daysTo(currentAnniversary);
    m_age = currentAnniversary.year() - m_date.year();
}

bool AbstractAnnualEventEntry::lessThan(const AbstractAnnualEventEntry *a, const AbstractAnnualEventEntry *b)
{
    if (a->m_remainingDays != b->m_remainingDays) return a->m_remainingDays < b->m_remainingDays;
    else if (a->m_age != b->m_age) return a->m_age < b->m_age;
    else return a->m_name < b->m_name;
}

QString AbstractAnnualEventEntry::remainingDaysString(const int remainingDays)
{
    QString msg;
    if (remainingDays < -2) {
        msg = i18n("%1 days ago",remainingDays*-1);
    } else if (remainingDays == -2) {
        msg = i18n("2 days ago");
    } else if (remainingDays == -1) {
        msg = i18n("yesterday");
    } else if (remainingDays == 0) {
        msg = i18n("today");
    } else if (remainingDays == 1) {
        msg = i18n("tomorrow");
    } else {
        msg = i18n("in %1 days",remainingDays);
    }
    return msg;
}


BirthdayEntry::BirthdayEntry(const QString &name, const QDate &date)
    :AbstractAnnualEventEntry(name, date)
{
}

BirthdayEntry::~BirthdayEntry() 
{
}

void BirthdayEntry::createModelItems(QList<QStandardItem*> &items) const
{
    items.append(new QStandardItem(m_name));
    items[0]->setIcon(BirthdayEntry::m_icon);
    items.append(new QStandardItem(QString::number(m_age)));
    items.append(new QStandardItem(m_date.toString("d. M.")));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));
}


NamedayEntry::NamedayEntry(const QString &name, const QDate &date)
    : AbstractAnnualEventEntry(name, date), m_aggregated(false)
{
}

NamedayEntry::~NamedayEntry() 
{
}

void NamedayEntry::createModelItems(QList<QStandardItem*> &items) const
{
    items.append(new QStandardItem(m_name));
    if (!m_aggregated) items[0]->setIcon(NamedayEntry::m_icon);
    items.append(new QStandardItem(QString::number(m_age)));
    items.append(new QStandardItem(m_aggregated ? "" : m_date.toString("d. M.")));
    items.append(new QStandardItem(m_aggregated ? "" : remainingDaysString(remainingDays())));
}

AggregatedNamedayEntry::AggregatedNamedayEntry(const QString &name, const QDate &date)
    : AbstractAnnualEventEntry(name, date)
{
}

AggregatedNamedayEntry::~AggregatedNamedayEntry()
{
}

void AggregatedNamedayEntry::addNamedayEntry(NamedayEntry &namedayEntry)
{
    m_storedEntries.append(namedayEntry);
    namedayEntry.setAggregated(true);
}

void AggregatedNamedayEntry::createModelItems(QList<QStandardItem*> &items) const
{
    items.append(new QStandardItem(QString("%1 (%2)").arg(m_name).arg(m_storedEntries.size())));
    items[0]->setIcon(AggregatedNamedayEntry::m_icon);
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(m_date.toString("d. M.")));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));

    foreach(NamedayEntry storedEntry, m_storedEntries) {
        QList<QStandardItem*> storedEntryItems;
        storedEntry.createModelItems(storedEntryItems);
        items[0]->appendRow(storedEntryItems);
    }
}
