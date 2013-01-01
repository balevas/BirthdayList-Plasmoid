#ifndef BIRTHDAYLIST_MODELENTRY_H
#define BIRTHDAYLIST_MODELENTRY_H

/**
 * @file    birthdaylist_modelentry.h
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


#include <KDebug>
#include <KIcon>
#include <QDate>
#include <QStandardItem>
#include <QString>


/**
 * Abstract parent class for all types of entries that can be placed in the birthday list.
 * Contains all the common functions used by the widget.
 */
class AbstractAnnualEventEntry {
public:
    AbstractAnnualEventEntry(const QString &name, const QDate &date, QString email, QString url);
    virtual ~AbstractAnnualEventEntry();

    /** Returns the name of the contact. */
    const QString& name() const {
        return m_name;
    }

    /** Returns the age of the contact. */
    int age() const {
        return m_age;
    }

    /** Returns the date of the corresponding contact's event. */
    const QDate& date() const {
        return m_date;
    }

    /** Returns the number of days remaining to the corresponding contact's event. */
    int remainingDays() const {
        return m_remainingDays;
    }

    /** Creates the representation of this entry in the tree view's model. */
    virtual void createModelItems(QList<QStandardItem*> &items, QString dateFormat) const = 0;

    /** Indicates if this entry is bound to one or more events in the selected address book. */
    virtual bool hasEvent() const = 0;

    /** Comparator used to sort the event entries by time. */
    static bool lessThan(const AbstractAnnualEventEntry *a, const AbstractAnnualEventEntry *b);

    /** Sets the number of days in the past, which will be taken as the boundary between the
     *  past and future events */
    static void setPastThreshold(int threshold) {
        m_pastThreshold = threshold;
    }

protected:
    /** Turns the number of remaining days to a readable text. */
    static QString remainingDaysString(const int remainingDays);

protected:
    QString m_name;
    int m_age;
    QDate m_date;
    int m_remainingDays;
    QString m_email;
    QString m_url;

    static int m_pastThreshold;
};


/**
 * Specific class for entries representing contacts' birthdays.
 */
class BirthdayEntry : public AbstractAnnualEventEntry {
public:
    BirthdayEntry(const QString &name, const QDate &date, QString email, QString url);
    virtual ~BirthdayEntry();

    virtual void createModelItems(QList<QStandardItem*> &items, QString dateFormat) const;
    virtual bool hasEvent() const;

private:
    static KIcon m_icon;
};


/**
 * Specific class for entries representing contacts' namedays.
 */
class NamedayEntry : public AbstractAnnualEventEntry {
public:
    NamedayEntry(const QString &name, const QDate &date, QString email, QString url);
    virtual ~NamedayEntry();

    /** Marks this entry as aggregated (affects visualisation). */
    void setAggregated(bool aggregated) {
        m_aggregated = aggregated;
    }

    virtual void createModelItems(QList<QStandardItem*> &items, QString dateFormat) const;
    virtual bool hasEvent() const;

private:
    bool m_aggregated;
    static KIcon m_icon;
};


/**
 * Specific class that aggregates the nameday events occurring on one day.
 */
class AggregatedNamedayEntry : public AbstractAnnualEventEntry {
public:
    AggregatedNamedayEntry(const QString &name, const QDate &date);
    virtual ~AggregatedNamedayEntry();

    /** Registers the given nameday entry in the aggregation. */
    void addNamedayEntry(NamedayEntry *namedayEntry);

    virtual void createModelItems(QList<QStandardItem*> &items, QString dateFormat) const;
    virtual bool hasEvent() const;

private:
    QList<NamedayEntry *> m_storedEntries;
    static KIcon m_icon;
};


/**
 * Specific class for entries representing contacts' anniversaries.
 */
class AnniversaryEntry : public AbstractAnnualEventEntry {
public:
    AnniversaryEntry(const QString &name, const QDate &date, QString email, QString url);
    virtual ~AnniversaryEntry();

    virtual void createModelItems(QList<QStandardItem*> &items, QString dateFormat) const;
    virtual bool hasEvent() const;

private:
    static KIcon m_icon;
};


#endif //BIRTHDAYLIST_MODELENTRY_H