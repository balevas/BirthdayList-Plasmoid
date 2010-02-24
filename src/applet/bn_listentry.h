#ifndef BLISTENTRY_H
#define BLISTENTRY_H

/* ************************************************
 *  @name: blistentry.h 
 *  @author: Meinhard Ritscher
 *  @date: 2008-10-21
 *
 *  $Id:  $
 *
 *  Description
 *  ============
 *  Data object storing all necessary info around
 *  the event
 *
 *  Histrory
 *  ============
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
 *   A copy of the license should be part of the package. If not, you can  *
 *   find it here: http://www.gnu.org/licenses/gpl.html                    *
 *                                                                         *
 ***************************************************************************/

#include <QString>
#include <QDate>
#include <QStandardItem>

#include <KDebug>
#include <KIcon>



class AbstractAnnualEventEntry {
    public:
        AbstractAnnualEventEntry(const QString &name, const QDate &date);
        virtual ~AbstractAnnualEventEntry();

        const QString& name() const { return m_name; }
        int age() const { return m_age; }
        const QDate& date() const { return m_date; }
        int remainingDays() const { return m_remainingDays; }

        void calculateDays();
	virtual void createModelItems(QList<QStandardItem*> &items) const = 0;

	static bool lessThan(const AbstractAnnualEventEntry *a, const AbstractAnnualEventEntry *b);
	
     protected:
        static QString remainingDaysString(const int remainingDays);


    protected:
    /** @brief The formatted name of that person. */
        QString m_name;
    /** @brief How old turns the person / the anniversary. */
        int m_age;
    /** @brief The date of the event*/
        QDate m_date;
    /** @brief How many days until the event (this might be negative). */
        int m_remainingDays;
	
	static int historyMargin;
};


class BirthdayEntry : public AbstractAnnualEventEntry {
    public:
        BirthdayEntry(const QString &name, const QDate &date);
        virtual ~BirthdayEntry();

	virtual void createModelItems(QList<QStandardItem*> &items) const;

  private:
    	static KIcon m_icon;
};


class NamedayEntry : public AbstractAnnualEventEntry {
    public:
        NamedayEntry(const QString &name, const QDate &date);
        virtual ~NamedayEntry();
	
	void setAggregated(bool aggregated) { m_aggregated = aggregated; }

	virtual void createModelItems(QList<QStandardItem*> &items) const;

  private:
        bool m_aggregated;
    	static KIcon m_icon;
};


class AggregatedNamedayEntry : public AbstractAnnualEventEntry {
    public:
        AggregatedNamedayEntry(const QString &name, const QDate &date);
        virtual ~AggregatedNamedayEntry();

        void addNamedayEntry(NamedayEntry &namedayEntry);

	virtual void createModelItems(QList<QStandardItem*> &items) const;

  private:
        QList<NamedayEntry> m_storedEntries;
    	static KIcon m_icon;
};

#endif
