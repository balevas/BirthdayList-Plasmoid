#ifndef BIRTHDAYLIST_VIEW_H
#define BIRTHDAYLIST_VIEW_H

/**
 * @file    birthdaylist_view.h
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


#include <Plasma/TreeView>
#include "birthdaylist_aboutdata.h"

namespace BirthdayList {
    class Model;
};
class QGraphicsWidget;


namespace BirthdayList
{
    struct ViewConfiguration 
    {
        ViewConfiguration();

        bool showColumnHeaders;
        bool showColName;
        bool showColAge;
        bool showColDate;
        bool showColWhen;
        int columnWidthName;
        int columnWidthAge;
        int columnWidthDate;
        int columnWidthWhen;
        int visualIndexName;
        int visualIndexAge;
        int visualIndexDate;
        int visualIndexWhen;
    };


    class View : public Plasma::TreeView
    {
        Q_OBJECT
    public:
        View(Model *model, QGraphicsWidget *parent = 0);
        ~View();
        
        void setConfiguration(ViewConfiguration newConf);
        ViewConfiguration getConfiguration() const;

        QList<QAction *> contextualActions();

        void setColumnSettings();
        /** Changes the tree and item colors according to the current Plasma theme. */
        void usePlasmaThemeColors();

    signals:
        void settingsChanged();
        
    private:
        ViewConfiguration m_conf;
    
        Model *m_model;

        QString getSelectedLineItem(int column);

    private slots:
        /** Receives a notification when the system plasma theme is changed. */
        void plasmaThemeChanged();
        void columnsResized(int logicalIndex, int oldSize, int newSize);
        void columnsMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex);
        
        void sendEmail();
        void visitHomepage();
    };
};


#endif //BIRTHDAYLIST_VIEW_H