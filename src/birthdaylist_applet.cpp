/**
 * @file    birthdaylist_applet.cpp
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


#include "birthdaylist_applet.h"
#include "birthdaylist_aboutdata.h"
#include "birthdaylist_confighelper.h"
#include "birthdaylist_model.h"
#include "birthdaylist_view.h"
#include <KAboutApplicationDialog>
#include <KConfigDialog>
#include <QGraphicsLinearLayout>
#include <QTreeView>


// linking this class to the desktop file so plasma can load it
K_EXPORT_PLASMA_APPLET(birthdaylist, BirthdayListApplet)


BirthdayListApplet::BirthdayListApplet(QObject *parent, const QVariantList &args)
: Plasma::PopupApplet(parent, args),
m_aboutData(new BirthdayListAboutData()),
m_configHelper(new BirthdayListConfigHelper()),
m_model(new BirthdayListModel()),
m_view(new BirthdayListView(m_model, 0)),
m_graphicsWidget(0)
{
    kDebug() << "Creating BirthdayList plasmoid";
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
   
    //resize(350, 200);
}

BirthdayListApplet::~BirthdayListApplet() 
{
    delete m_graphicsWidget;
    // m_view should be deleted automatically
    delete m_model;
    delete m_configHelper;
    delete m_aboutData;
}

void BirthdayListApplet::init() 
{
    setPopupIcon(KIcon("bl_cookie", NULL));

    // configure the model and view from the persisted plasmoid configuration
    BirthdayListModelConfiguration modelConf;
    BirthdayListViewConfiguration viewConf;
    
    KConfigGroup configGroup = config();
    m_configHelper->loadConfiguration(configGroup, modelConf, viewConf);
    m_model->setConfiguration(modelConf);
    m_view->setConfiguration(viewConf);
}

QGraphicsWidget *BirthdayListApplet::graphicsWidget() 
{
    if (!m_graphicsWidget) {
        m_graphicsWidget = new QGraphicsWidget(this);

        QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addItem(m_view);
        m_graphicsWidget->setLayout(layout);
        m_view->setParent(m_graphicsWidget);
    }
    
    return m_graphicsWidget;
}

QList<QAction *> BirthdayListApplet::contextualActions()
{
    // start with context menu actions based on the current location in the tree
    QList<QAction *> currentActions = m_view->contextualActions();

    QAction *separator1 = new QAction(this);
    separator1->setSeparator(true);
    currentActions.append(separator1);

    KIcon aboutIcon("help-about");
    QAction *actionAbout = new QAction(aboutIcon, i18nc("Show About Dialog", "About"), this);
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    currentActions.append(actionAbout);

    QAction *separator2 = new QAction(this);
    separator2->setSeparator(true);
    currentActions.append(separator2);
    
    return currentActions;
}

void BirthdayListApplet::createConfigurationInterface(KConfigDialog *parent) 
{
    BirthdayListModelConfiguration modelConf = m_model->getConfiguration();
    BirthdayListViewConfiguration viewConf = m_view->getConfiguration();
    
    m_configHelper->createConfigurationUI(parent, m_model, modelConf, viewConf);

    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    parent->resize(parent->minimumSizeHint());
}

void BirthdayListApplet::configAccepted() 
{
    BirthdayListModelConfiguration modelConf;
    // get the view configuration from the view since it contains some items that are updated directly by the view (such as column widths)
    BirthdayListViewConfiguration viewConf = m_view->getConfiguration();

    m_configHelper->updateConfigurationFromUI(modelConf, viewConf);
    
    m_model->setConfiguration(modelConf);
    m_view->setConfiguration(viewConf);

    KConfigGroup configGroup = config();
    m_configHelper->storeConfiguration(configGroup, modelConf, viewConf);

    emit configNeedsSaving();
}

void BirthdayListApplet::about() 
{
  KAboutApplicationDialog *aboutDialog = new KAboutApplicationDialog(m_aboutData);
  connect(aboutDialog, SIGNAL(finished()), aboutDialog, SLOT(deleteLater()));
  aboutDialog->show();
}


#include "birthdaylist_applet.moc"
