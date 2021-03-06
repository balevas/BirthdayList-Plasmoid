/**
 * @file    birthdaylist_modelentry.cpp
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


#include "birthdaylist_modelentry.h"
#include <KLocalizedString>
#include <QStandardItemModel>


int BirthdayList::AbstractAnnualEventEntry::m_pastThreshold = 7;
KIcon BirthdayList::BirthdayEntry::m_icon("bl_cookie.png");
KIcon BirthdayList::NamedayEntry::m_icon("bl_date.png");
KIcon BirthdayList::AggregatedNamedayEntry::m_icon("bl_date.png");
KIcon BirthdayList::AnniversaryEntry::m_icon("bl_rings.png");


BirthdayList::AbstractAnnualEventEntry::AbstractAnnualEventEntry(const QString &name, const QDate &date, QString email, QString url)
: m_name(name), m_date(date), m_email(email), m_url(url) 
{
    QDate today = QDate::currentDate();

    m_currentAnniversary = QDate(today.year(), m_date.month(), m_date.day());
    int daysToAnniversary = today.daysTo(m_currentAnniversary);
    if (daysToAnniversary < -m_pastThreshold) {
        m_currentAnniversary = QDate(today.year() + 1, m_date.month(), m_date.day());
    } else if (daysToAnniversary > today.daysInYear() - m_pastThreshold) {
        m_currentAnniversary = QDate(today.year() - 1, m_date.month(), m_date.day());
    }

    m_remainingDays = today.daysTo(m_currentAnniversary);
    m_age = m_currentAnniversary.year() - m_date.year();
}

BirthdayList::AbstractAnnualEventEntry::~AbstractAnnualEventEntry() 
{
}

bool BirthdayList::AbstractAnnualEventEntry::lessThan(const AbstractAnnualEventEntry *a, const AbstractAnnualEventEntry *b) 
{
    if (a->m_remainingDays != b->m_remainingDays) return a->m_remainingDays < b->m_remainingDays;
    else if (a->m_age != b->m_age) return a->m_age < b->m_age;
    else return a->m_name < b->m_name;
}

QString BirthdayList::AbstractAnnualEventEntry::remainingDaysString(const int remainingDays) 
{
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


BirthdayList::BirthdayEntry::BirthdayEntry(const QString &name, const QDate &date, QString email, QString url)
: AbstractAnnualEventEntry(name, date, email, url) {
}

BirthdayList::BirthdayEntry::~BirthdayEntry() {
}

void BirthdayList::BirthdayEntry::createModelItems(QList<QStandardItem*> &items, QString dateFormat) const 
{
    items.append(new QStandardItem(m_name));
    items[0]->setIcon(BirthdayEntry::m_icon);
    items.append(new QStandardItem(QString::number(m_age)));
    items.append(new QStandardItem(m_currentAnniversary.toString(dateFormat)));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));
    items.append(new QStandardItem(m_email));
    items.append(new QStandardItem(m_url));
}

bool BirthdayList::BirthdayEntry::hasEvent() const 
{
    return true;
}


BirthdayList::NamedayEntry::NamedayEntry(const QString &name, const QDate &date, QString email, QString url)
: AbstractAnnualEventEntry(name, date, email, url), m_aggregated(false) {
}

BirthdayList::NamedayEntry::~NamedayEntry() 
{
}

void BirthdayList::NamedayEntry::createModelItems(QList<QStandardItem*> &items, QString dateFormat) const 
{
    items.append(new QStandardItem(m_name));
    items[0]->setIcon(NamedayEntry::m_icon);
    items.append(new QStandardItem(m_age >= 0 ? QString::number(m_age) : ""));
    items.append(new QStandardItem(m_aggregated ? "" : m_currentAnniversary.toString(dateFormat)));
    items.append(new QStandardItem(m_aggregated ? "" : remainingDaysString(remainingDays())));
    items.append(new QStandardItem(m_email));
    items.append(new QStandardItem(m_url));
}

bool BirthdayList::NamedayEntry::hasEvent() const 
{
    return true;
}


BirthdayList::AggregatedNamedayEntry::AggregatedNamedayEntry(const QString &name, const QDate &date)
: AbstractAnnualEventEntry(name, date, "", "") 
{
}

BirthdayList::AggregatedNamedayEntry::~AggregatedNamedayEntry() 
{

    foreach(NamedayEntry *storedEntry, m_storedEntries) {
        delete storedEntry;
    }
    m_storedEntries.clear();
}

void BirthdayList::AggregatedNamedayEntry::addNamedayEntry(NamedayEntry *namedayEntry) 
{
    namedayEntry->setAggregated(true);
    m_storedEntries.append(namedayEntry);
}

void BirthdayList::AggregatedNamedayEntry::createModelItems(QList<QStandardItem*> &items, QString dateFormat) const 
{
    if (m_storedEntries.empty()) items.append(new QStandardItem(m_name));
    else items.append(new QStandardItem(QString("%1 (%2)").arg(m_name).arg(m_storedEntries.size())));
    items[0]->setIcon(AggregatedNamedayEntry::m_icon);
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(m_currentAnniversary.toString(dateFormat)));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));
    items.append(new QStandardItem(""));
    items.append(new QStandardItem(""));

    foreach(NamedayEntry *storedEntry, m_storedEntries) {
        QList<QStandardItem*> storedEntryItems;
        storedEntry->createModelItems(storedEntryItems, dateFormat);
        items[0]->appendRow(storedEntryItems);
    }
}

bool BirthdayList::AggregatedNamedayEntry::hasEvent() const 
{
    return !m_storedEntries.empty();
}


BirthdayList::AnniversaryEntry::AnniversaryEntry(const QString &name, const QDate &date, QString email, QString url)
: AbstractAnnualEventEntry(name, date, email, url) 
{
}

BirthdayList::AnniversaryEntry::~AnniversaryEntry() 
{
}

void BirthdayList::AnniversaryEntry::createModelItems(QList<QStandardItem*> &items, QString dateFormat) const 
{
    items.append(new QStandardItem(m_name));
    items[0]->setIcon(AnniversaryEntry::m_icon);
    items.append(new QStandardItem(QString::number(m_age)));
    items.append(new QStandardItem(m_currentAnniversary.toString(dateFormat)));
    items.append(new QStandardItem(remainingDaysString(remainingDays())));
    items.append(new QStandardItem(m_email));
    items.append(new QStandardItem(m_url));
}

bool BirthdayList::AnniversaryEntry::hasEvent() const 
{
    return true;
}
