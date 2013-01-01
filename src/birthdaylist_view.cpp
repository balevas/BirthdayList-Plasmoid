/**
 * @file    birthdaylist_view.cpp
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

#include "birthdaylist_view.h"
#include "birthdaylist_model.h"
#include <Plasma/Theme>
#include <Plasma/TreeView>
#include <KIcon>
#include <KMimeTypeTrader>
#include <KToolInvocation>
#include <QAction>
#include <QHeaderView>
#include <QTreeView>


BirthdayListViewConfiguration::BirthdayListViewConfiguration() :
showColumnHeaders(true),
showColName(true),
showColAge(true),
showColDate(true),
showColWhen(true),
columnWidthName(0),
columnWidthAge(0),
columnWidthDate(0),
columnWidthWhen(0)
{
}


BirthdayListView::BirthdayListView(BirthdayListModel *model, QGraphicsWidget *parent)
: Plasma::TreeView(parent),
m_model(model)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QTreeView *treeView = nativeWidget();
    treeView->setAlternatingRowColors(true);
    treeView->setAnimated(true);
    treeView->setAllColumnsShowFocus(true);
    treeView->setRootIsDecorated(false);
    //treeView->setSortingEnabled(true);
    treeView->setSelectionMode(QAbstractItemView::NoSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView->setItemsExpandable(true);
    treeView->setExpandsOnDoubleClick(true);
    
    setModel(m_model);
    setMinimumHeight(10);
    
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(plasmaThemeChanged()));
    connect(nativeWidget()->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(columnsResized(int,int,int)));
}

BirthdayListView::~BirthdayListView()
{
}

void BirthdayListView::setConfiguration(BirthdayListViewConfiguration newConf) 
{
    m_conf = newConf;

    // update view
    setColumnWidths();
    usePlasmaThemeColors();
}

BirthdayListViewConfiguration BirthdayListView::getConfiguration() const 
{
    return m_conf;
}

QList<QAction *> BirthdayListView::contextualActions()
{
    QList<QAction *> currentActions;

    QModelIndex idx = nativeWidget()->currentIndex();
    bool lastContextMenuEventOnTree = nativeWidget()->underMouse() && !nativeWidget()->header()->underMouse();
    if (lastContextMenuEventOnTree && idx.isValid()) {
       
       QString name = getSelectedLineItem(0);

       QString selectedEntryEmail = getSelectedLineItem(4);
        if (!selectedEntryEmail.isEmpty()) {
            KIcon mailerIcon("internet-mail");
            QAction *actionSendEmail = new QAction(mailerIcon, QString(i18n("Send Email to %1")).arg(name), this);
            connect(actionSendEmail, SIGNAL(triggered()), this, SLOT(sendEmail()));
            currentActions.append(actionSendEmail);
        }
        
        KService::Ptr browserService = KMimeTypeTrader::self()->preferredService("text/html");
        KIcon browserIcon("konqueror");
        if (!browserService.isNull()) browserIcon = KIcon(browserService->icon());

        QString selectedEntryUrl = getSelectedLineItem(5);
        if (!selectedEntryUrl.isEmpty()) {
            QAction *actionVisitHomepage = new QAction(browserIcon, QString(i18n("Visit %1's Homepage")).arg(name), this);
            connect(actionVisitHomepage, SIGNAL(triggered()), this, SLOT(visitHomepage()));
            currentActions.append(actionVisitHomepage);
        }
    }
    
    return currentActions;
}

QString BirthdayListView::getSelectedLineItem(int column)
{
    QModelIndex idx = nativeWidget()->currentIndex();
    if (idx.isValid()) return m_model->itemFromIndex((m_model->index(idx.row(), column, idx.parent())))->text();
    else return "";
}

void BirthdayListView::setColumnWidths() 
{
    QTreeView *qTreeView = nativeWidget();

    qTreeView->setHeaderHidden(!m_conf.showColumnHeaders);
    qTreeView->setColumnHidden(0, !m_conf.showColName);
    qTreeView->setColumnHidden(1, !m_conf.showColAge);
    qTreeView->setColumnHidden(2, !m_conf.showColDate);
    qTreeView->setColumnHidden(3, !m_conf.showColWhen);
    qTreeView->setColumnHidden(4, true);
    qTreeView->setColumnHidden(5, true);

    if (m_conf.columnWidthName < 10) qTreeView->resizeColumnToContents(0);
    else qTreeView->setColumnWidth(0, m_conf.columnWidthName);

    if (m_conf.columnWidthAge < 10) qTreeView->resizeColumnToContents(1);
    else qTreeView->setColumnWidth(1, m_conf.columnWidthAge);

    if (m_conf.columnWidthDate < 10) qTreeView->resizeColumnToContents(2);
    else qTreeView->setColumnWidth(2, m_conf.columnWidthDate);

    if (m_conf.columnWidthWhen < 10) qTreeView->resizeColumnToContents(3);
    else qTreeView->setColumnWidth(3, m_conf.columnWidthWhen);
}

void BirthdayListView::plasmaThemeChanged() 
{
    usePlasmaThemeColors();
}
    
void BirthdayListView::usePlasmaThemeColors() 
{
    //QFont font = Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont);

    // Get theme colors
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QColor baseColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
    QColor altBaseColor = baseColor.darker();
    QColor buttonColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
    baseColor.setAlpha(50);
    altBaseColor.setAlpha(50);
    buttonColor.setAlpha(130);

    // Create palette with the used theme colors
    QPalette p = palette();
    p.setColor(QPalette::Background, baseColor);
    p.setColor(QPalette::Base, baseColor);
    p.setColor(QPalette::AlternateBase, altBaseColor);
    p.setColor(QPalette::Button, buttonColor);
    p.setColor(QPalette::Foreground, textColor);
    p.setColor(QPalette::Text, textColor);

    nativeWidget()->setPalette(p);
    nativeWidget()->header()->setPalette(p);

    QBrush textBrush = QBrush(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));
    for (int i = 0; i < m_model->columnCount(); ++i) {
        m_model->horizontalHeaderItem(i)->setForeground(textBrush);
    }
}

void BirthdayListView::columnsResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(oldSize);
    
    switch (logicalIndex) {
        case 0:
            m_conf.columnWidthName = newSize;
            break;
        case 1:
            m_conf.columnWidthAge = newSize;
            break;
        case 2:
            m_conf.columnWidthDate = newSize;
            break;
        case 3:
            m_conf.columnWidthWhen = newSize;
            break;
    }
}

void BirthdayListView::sendEmail() 
{
  KToolInvocation::invokeMailer(getSelectedLineItem(4), "");
}

void BirthdayListView::visitHomepage() 
{
  KToolInvocation::invokeBrowser(getSelectedLineItem(5));
}
