/**
 * @file    birthdaylistentry.cpp
 * @author  Karol Slanina
 * @version 0.5.0
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

#include "birthdaylistentry.h"

#include <KLocalizedString>


int AbstractAnnualEventEntry::m_pastThreshold = 7;
KIcon BirthdayEntry::m_icon("bl_cookie.png");
KIcon NamedayEntry::m_icon("bl_date.png");
KIcon AggregatedNamedayEntry::m_icon("bl_date.png");
KIcon AnniversaryEntry::m_icon("bl_rings.png");


AbstractAnnualEventEntry::AbstractAnnualEventEntry(const QString &name, const QDate &date)
: m_name(name), m_date(date) {
    QDate today = QDate::currentDate();

    QDate currentAnniversary = QDate(today.year(), m_date.month(), m_date.day());
    int daysToAnniversary = today.daysTo(currentAnniversary);
    if (daysToAnniversary < -m_pastThreshold) {
        currentAnniversary = QDate(today.year() + 1, m_date.month(), m_date.day());
    } else if (daysToAnniversary > today.daysInYear() - m_pastThreshold) {
        currentAnniversary = QDate(today.year() - 1, m_date.month(), m_date.day());
    }

    m_remainingDays = today.daysTo(currentAnniversary);
    m_age = currentAnniversary.year() - m_date.year();
}

AbstractAnnualEventEntry::~AbstractAnnualEventEntry() {
}

bool AbstractAnnualEventEntry::lessThan(const AbstractAnnualEventEntry *a, const AbstractAnnualEventEntry *b) {
    if (a->m_remainingDays != b->m_remainingDays) return a->m_remainingDays < b->m_remainingDays;
    else if (a->m_age != b->m_age) return a->m_age < b->m_age;
    else return a->m_name < b->m_name;
}

QString AbstractAnnualEventEntry::remainingDaysString(const int remainingDays) {
    QString msg;
    if (remainingDays < -2) {
        msg = i18np("1 day ago", "%1 days ago", -remainingDays);
    } else if (remainingDays == -2) { // in some languages there may be a better expression than "2 days ago"
        msg = i18n("2 days ago");
    } else if (remainingDays == -1) {
        msg = i18n("yesterday");
    } else if (remainingDays == 0) {
        msg = i18n("today");
    } else if (remainingDays == 1) {
        msg = i18n("tomorrow");
    } else if (remainingDays == 2) { // in some languages there may be a better expression than "in 2 days"
        msg = i18n("in 2 days");
    } else {
        msg = i18np("in 1 day", "in %1 days", remainingDays);
    }
    return msg;
}


BirthdayEntry::BirthdayEntry(const QString &name, const QDate &date)
: AbstractAnnualEventEntry(name, date) {
}

BirthdayEntry::~BirthdayEntry() {
}

void BirthdayEntry::createModelItems(QList<QStandardItem*> &items) const {
    items.append(new QStandardItem(m_name));
    items[0]->setIcon(BirthdayEntry::m_icon);
    items.append(new QStandardItem(QString::number(m_age)));
    items.append(new QStandardItem(m_date.toString("d. M.")));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));
}


NamedayEntry::NamedayEntry(const QString &name, const QDate &date)
: AbstractAnnualEventEntry(name, date), m_aggregated(false) {
}

NamedayEntry::~NamedayEntry() {
}

void NamedayEntry::createModelItems(QList<QStandardItem*> &items) const {
    items.append(new QStandardItem(m_name));
    items[0]->setIcon(NamedayEntry::m_icon);
    items.append(new QStandardItem(QString::number(m_age)));
    items.append(new QStandardItem(m_aggregated ? "" : m_date.toString("d. M.")));
    items.append(new QStandardItem(m_aggregated ? "" : remainingDaysString(remainingDays())));
}


AggregatedNamedayEntry::AggregatedNamedayEntry(const QString &name, const QDate &date)
: AbstractAnnualEventEntry(name, date) {
}

AggregatedNamedayEntry::~AggregatedNamedayEntry() {

    foreach(NamedayEntry *storedEntry, m_storedEntries) {
        delete storedEntry;
    }
    m_storedEntries.clear();
}

void AggregatedNamedayEntry::addNamedayEntry(NamedayEntry *namedayEntry) {
    namedayEntry->setAggregated(true);
    m_storedEntries.append(namedayEntry);
}

void AggregatedNamedayEntry::createModelItems(QList<QStandardItem*> &items) const {
    items.append(new QStandardItem(QString("%1 (%2)").arg(m_name).arg(m_storedEntries.size())));
    items[0]->setIcon(AggregatedNamedayEntry::m_icon);
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(m_date.toString("d. M.")));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));

    foreach(NamedayEntry *storedEntry, m_storedEntries) {
        QList<QStandardItem*> storedEntryItems;
        storedEntry->createModelItems(storedEntryItems);
        items[0]->appendRow(storedEntryItems);
    }
}


AnniversaryEntry::AnniversaryEntry(const QString &name, const QDate &date)
: AbstractAnnualEventEntry(name, date) {
}

AnniversaryEntry::~AnniversaryEntry() {
}

void AnniversaryEntry::createModelItems(QList<QStandardItem*> &items) const {
    items.append(new QStandardItem(m_name));
    items[0]->setIcon(AnniversaryEntry::m_icon);
    items.append(new QStandardItem(QString::number(m_age)));
    items.append(new QStandardItem(m_date.toString("d. M.")));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));
}
