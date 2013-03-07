#ifndef BIRTHDAYLIST_APPLET_H
#define BIRTHDAYLIST_APPLET_H

/**
 * @file    birthdaylist_applet.h
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


#include <Plasma/PopupApplet>

namespace BirthdayList {
    class ConfigHelper;
    class Model;
    class View;
    class ViewConfiguration;
};
class KAboutData;


namespace BirthdayList
{
    class Applet : public Plasma::PopupApplet 
    {
        Q_OBJECT
    public:
        Applet(QObject *parent, const QVariantList &args);
        ~Applet();

        /** Initializes the applet (called by Plasma automatically). */
        void init();

        /** Creates the widget that will be shown in the Plasma applet. */
        QGraphicsWidget *graphicsWidget();
        virtual QList<QAction *> contextualActions();
        
    private slots:
        /** Receives a notification when the user accepts the configuration change. */
        void configAccepted();
        void viewSettingChanged();
        void about();

    private:
        /** Creates the configuration dialog and fills it with current settings. */
        void createConfigurationInterface(KConfigDialog *parent);

        /** BirthdayList specific contents of the about dialog */
        KAboutData *m_aboutData;

        /** Helper object to read/write the model and view configuration from/to the persistent storage and configuration UI */
        ConfigHelper *m_configHelper;

        /** Internal data model (table contents) */
        Model *m_model;
        
        /** View of the data model (table) */
        View *m_view;

        /** Widget containing the BirthdayList view and shown in the Plasma applet */
        QGraphicsWidget *m_graphicsWidget;
    };
};


#endif //BIRTHDAYLIST_APPLET_H