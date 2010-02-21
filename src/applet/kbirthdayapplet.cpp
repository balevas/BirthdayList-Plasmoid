#include "kbirthdayapplet.h"

/* ************************************************
 *  @name: kbirthdayapplet.cpp 
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

// Qt
#include <QPainter>
#include <QFontMetrics>
#include <QSizeF>
#include <QPair>
#include <QTime>
#include <QStandardItemModel>
#include <QTreeView>
#include <QGraphicsLinearLayout>

// KDE
#include <KConfigDialog>
#include <KDebug>

// Plasma
#include <Plasma/Theme>
#include <Plasma/Svg>
#include <Plasma/IconWidget>
#if KDE_IS_VERSION(4,1,80)
#include <Plasma/ToolTipManager>
#endif

//own
#include "blistentry.h"

// linking this class to the desktop file so plasma can load it
K_EXPORT_PLASMA_APPLET(kbirthdayapplet, KBirthdayApplet)


typedef QPair<QString, QDate> KabEntry;
Q_DECLARE_METATYPE(KabEntry)


static const char *BIRTHDAY_SOURCE    = "Birthdays";
static const char *ANNIVERSARY_SOURCE = "Anniversaries";
static const char *UTC_SOURCE         = "UTC";

KBirthdayApplet::KBirthdayApplet(QObject *parent, const QVariantList &args)
    : Plasma::PopupApplet(parent, args),
    m_graphicsWidget(0),
    m_treeView(0),
    m_model(0),
    m_isOnPanel(false),
    m_pKabcEngine(0),
    m_pBirthdays(0),
    m_pAnniversaries(0),
    m_pShownEntries(0),
    m_svg(0),
    m_columnWidthName(0),
    m_columnWidthAge(0),
    m_columnWidthDate(0),
    m_columnWidthRemaining(0)
{
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    resize(250, 300);
}

KBirthdayApplet::~KBirthdayApplet()
{
    if (hasFailedToLaunch()) {
        //Do some cleanup here
    } else {
    }
    delete m_graphicsWidget;
    delete m_pBirthdays;
    delete m_pAnniversaries;
    delete m_pShownEntries;
}

void KBirthdayApplet::init()
{
    m_pIcon = new Plasma::IconWidget(KIcon("cookie",NULL), QString());
    setPopupIcon(m_pIcon->icon());

    m_pKabcEngine = dataEngine("kabc");
    if (m_pKabcEngine) {
        m_pKabcEngine->connectSource(BIRTHDAY_SOURCE, this);
        m_pKabcEngine->connectSource(ANNIVERSARY_SOURCE, this);
    } else {
        //kDebug() << "Could not load kabc dataEngine";
        setFailedToLaunch(true, "Could not load kabc dataEngine");
    }
    Plasma::DataEngine* timeEngine = dataEngine("time");
    if (timeEngine) {
        timeEngine->connectSource("UTC", this, 360000, Plasma::AlignToHour);
    } else {
        kDebug() << "Warning: Could not load time dataEngine - no nightly update possible";
    }
    
    KConfigGroup configGroup = config();
    QColor pastColor = configGroup.readEntry("Past Color",           QColor(150,150,150));
    m_brushForegroundPast = QBrush(pastColor);
    QColor highlightColor = configGroup.readEntry("Highlight Color", QColor(170,0,0));
    highlightColor.setAlpha(64);
    m_brushForegroundHighlight = QBrush(highlightColor);
    m_highlightTreshold = configGroup.readEntry("Highlight Treshold", 2);
    m_eventTreshold = configGroup.readEntry("Event Treshold",       30);
    m_pastTreshold = configGroup.readEntry("Past Treshold",           2);
    m_showAnniversaries = configGroup.readEntry("Show Anniversaries", true);
    
    m_columnWidthName = configGroup.readEntry("Name Column Width", 0);
    m_columnWidthAge = configGroup.readEntry("Age Column Width", 0);
    m_columnWidthDate = configGroup.readEntry("Date Column Width", 0);
    m_columnWidthRemaining = configGroup.readEntry("Remaining Column Width", 0);

    m_svg = new Plasma::Svg(this);
    m_svg->setImagePath("widgets/birthdaycake");
    m_svg->setContainsMultipleImages(false);
 
    graphicsWidget();
}

QGraphicsWidget *KBirthdayApplet::graphicsWidget()
{
  if (!m_graphicsWidget) {
    m_graphicsWidget = new QGraphicsWidget(this);
    m_graphicsWidget->setMinimumSize(225, 150);
    m_graphicsWidget->setPreferredSize(350, 200);
    
    	m_treeView = new Plasma::TreeView(m_graphicsWidget);
	m_treeView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	QTreeView *treeView =m_treeView->nativeWidget();
 	treeView->setAlternatingRowColors( true );
	treeView->setAllColumnsShowFocus( true );
	treeView->setRootIsDecorated(false);
	//treeView->setSortingEnabled(true);
	treeView->setSelectionMode( QAbstractItemView::NoSelection );
	treeView->setSelectionBehavior( QAbstractItemView::SelectRows );
	treeView->setItemsExpandable(true);
	treeView->setExpandsOnDoubleClick(true);

    
    	m_model = new QStandardItemModel(0,3);
	updateModels();
	m_treeView->setModel( m_model );
    
	QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical);
	layout->setContentsMargins( 0, 0, 0, 0 );
	layout->setSpacing( 0 );
	layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

//	QGraphicsLayout *layoutTop = new QGraphicsLinearLayout(Qt::Horizontal); // = createLayoutTitle();
//	layout->addItem( layoutTop );

	layout->addItem( m_treeView );
	m_graphicsWidget->setLayout(layout);
	
	useCurrentPlasmaTheme();
  }
  return m_graphicsWidget;
}

void KBirthdayApplet::updateModels()
{
    qDebug() << "CREATING BIRTHDAY MODEL";
    
    if (!m_model) m_model = new QStandardItemModel(0,3);
    m_model->clear();
    QStringList headerTitles;
    headerTitles << "Name" << "Age" << "Date" << "Remaining";
    m_model->setHorizontalHeaderLabels(headerTitles);
    setColumnWidths();
    
    QStandardItem *parentItem = m_model->invisibleRootItem();
    
    if (m_pShownEntries && ! m_pShownEntries->isEmpty()) {
        foreach(const AbstractAnnualEventEntry *entry, *m_pShownEntries) {
            if ( testThreshold(entry->remainingDays())) {
		    QList<QStandardItem*> items;
		    entry->createModelItems(items);

		    for (int i=0; i<items.size(); ++i) items[i]->setEditable(false);

		    if (entry->remainingDays() < 0) {
		      for (int i=0; i<items.size(); ++i) {
		        //items[i]->setForeground(m_brushForegroundPast);
			items[i]->setBackground(m_brushForegroundPast);
		      }
		    }
		    else if (entry->remainingDays() < m_highlightTreshold) {
		      for (int i=0; i<items.size(); ++i) {
			items[i]->setBackground(m_brushForegroundHighlight);
		      }
		    }
		    
		    parentItem->appendRow(items);
                    // kDebug() << "Found birthday event" << entry->name();
            }
        }
    }
}

void KBirthdayApplet::setColumnWidths() {
  if (!m_treeView || !m_model) return;
  
  QTreeView *qTreeView = m_treeView->nativeWidget();
  if (m_columnWidthName == 0 || m_columnWidthAge == 0 || m_columnWidthDate == 0 || m_columnWidthRemaining == 0) {
    for (int i=0; i<m_model->columnCount(); ++i) {
      qTreeView->resizeColumnToContents(i);
    }
    qDebug() << "Resetting column widths";
  }
  else {
    qTreeView->setColumnWidth(0, m_columnWidthName);
    qTreeView->setColumnWidth(1, m_columnWidthAge);
    qTreeView->setColumnWidth(2, m_columnWidthDate);
    qTreeView->setColumnWidth(3, m_columnWidthRemaining);
    qDebug() << "Applying column widths: " << m_columnWidthName << ", " << m_columnWidthAge << ", " << m_columnWidthDate << ", " << m_columnWidthRemaining;
  }
}

void KBirthdayApplet::useCurrentPlasmaTheme() {
   /* QFont font = Plasma::Theme::defaultTheme()->font( Plasma::Theme::DefaultFont );
    int newPixelSize = qCeil((float)font.pixelSize() * 1.4f);
    if ( newPixelSize > 1 )
	font.setPixelSize( newPixelSize );
   // m_label->setFont( font );*/

    // Get theme colors
    QColor textColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor );
    QColor baseColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor );
    QColor altBaseColor = baseColor.darker();
    // 	int green = altBaseColor.green() * 1.8;
    // 	altBaseColor.setGreen( green > 255 ? 255 : green ); // tint green
    QColor buttonColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor );
    baseColor.setAlpha(50);
    altBaseColor.setAlpha(50);
    buttonColor.setAlpha(130);
//    m_colorSubItemLabels = textColor;
//    m_colorSubItemLabels.setAlpha( 170 );

    // Create palette with the used theme colors
    QPalette p = palette();
    p.setColor( QPalette::Background, baseColor );
    p.setColor( QPalette::Base, baseColor );
    p.setColor( QPalette::AlternateBase, altBaseColor );
    p.setColor( QPalette::Button, buttonColor );
    p.setColor( QPalette::Foreground, textColor );
    p.setColor( QPalette::Text, textColor );

    if (m_treeView) {
    QTreeView *treeView = m_treeView->nativeWidget();
    treeView->setPalette(p);
    treeView->header()->setPalette( p );
    }

    QBrush textBrush = QBrush( Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor) );
    if (m_model) {
      for (int i=0; i<m_model->columnCount(); ++i) {
      	m_model->horizontalHeaderItem(i)->setForeground( textBrush );
      }
    }

    // To set new text color of the header items
    //setDepartureArrivalListType( m_settings.departureArrivalListType() );
}

void KBirthdayApplet::constraintsEvent(Plasma::Constraints constraints)
{
    if (constraints & Plasma::FormFactorConstraint){
        Plasma::FormFactor formFctr = formFactor();
        if (formFctr == Plasma::Horizontal || formFctr == Plasma::Vertical) {
            setMinimumSize(QSize(42,42));
            setAspectRatioMode(Plasma::ConstrainedSquare);
#if KDE_IS_VERSION(4,1,80)
            Plasma::ToolTipManager::self()->registerWidget(this);
#endif
            setAspectRatioMode(Plasma::IgnoreAspectRatio);
            m_isOnPanel = true;
            kDebug() << "PANEL PANEL PANEL";

        } else if (formFctr == Plasma::MediaCenter || formFctr == Plasma::Planar) {
#if KDE_IS_VERSION(4,1,80)
            Plasma::ToolTipManager::self()->unregisterWidget(this);
#endif
            m_isOnPanel = false;
        }
    }
}

void KBirthdayApplet::paintInterface(QPainter *p,
    const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(p)
    Q_UNUSED(option)
    Q_UNUSED(contentsRect)
}

void KBirthdayApplet::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget;

    m_ui.setupUi(widget);

    parent->setButtons(KDialog::Ok | KDialog::Cancel);
    parent->addPage(widget, parent->windowTitle(), icon());

    //m_ui.colour->setColor(m_colour);
    m_ui.pastColour->setColor(m_brushForegroundPast.color());
    m_ui.highlightColour->setColor(m_brushForegroundHighlight.color());

    m_ui.spinEventTreshold->setValue(m_eventTreshold);
    m_ui.spinPast->setValue(m_pastTreshold);
    m_ui.spinHighlight->setValue(m_highlightTreshold);

    m_ui.showAnniversaries->setChecked(m_showAnniversaries);

    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
}

/*
 * This method is called when the user clicks ok
 * in the config dialog
 *
 */

void KBirthdayApplet::configAccepted()
{
    m_brushForegroundPast.setColor(m_ui.pastColour->color());
    m_brushForegroundHighlight.setColor(m_ui.highlightColour->color());

    m_eventTreshold     = m_ui.spinEventTreshold->value();
    m_pastTreshold      = m_ui.spinPast->value();
    m_highlightTreshold = m_ui.spinHighlight->value();

    m_showAnniversaries = m_ui.showAnniversaries->isChecked();

    KConfigGroup configGroup = config();
    configGroup.writeEntry("Past Color", m_brushForegroundPast.color());
    configGroup.writeEntry("Highlight Color", m_brushForegroundHighlight.color());
    configGroup.writeEntry("Highlight Treshold",m_highlightTreshold);
    configGroup.writeEntry("Event Treshold",m_eventTreshold);
    configGroup.writeEntry("Past Treshold",m_pastTreshold);
    configGroup.writeEntry("Show Anniversaries",m_showAnniversaries);

    QTreeView *qTreeView = m_treeView->nativeWidget();
    configGroup.writeEntry("Name Column Width",qTreeView->columnWidth(0));
    configGroup.writeEntry("Age Column Width",qTreeView->columnWidth(1));
    configGroup.writeEntry("Date Column Width",qTreeView->columnWidth(2));
    configGroup.writeEntry("Remaining Column Width",qTreeView->columnWidth(3));

    updateEventCount();
    updateModels();
    update();
    useCurrentPlasmaTheme();

    emit configNeedsSaving();
}

void KBirthdayApplet::dataUpdated(const QString &name, const Plasma::DataEngine::Data &data)
{
     qDebug() << "UPDATING BIRTHDAY DATA";
    if (QString::compare(name, BIRTHDAY_SOURCE) == 0){
        updateEventList(data["Birthdays"].toList(), &m_pBirthdays, true);
        updateEventCount();
    }else if (QString::compare(name, ANNIVERSARY_SOURCE) == 0){
        updateEventList(data["Anniversaries"].toList(), &m_pAnniversaries, false);
        updateEventCount();
    }else if (QString::compare(name, UTC_SOURCE) == 0){
        // if midnight
        if ( QTime::currentTime().hour() == 0 ) {
	    if(m_pBirthdays){
	    //if started round midnight thees might not have
	    //created yet
            foreach(AbstractAnnualEventEntry *entry, *m_pBirthdays) {
                entry->calculateDays();
            }
	    }
	    if(m_pAnniversaries){
            foreach(AbstractAnnualEventEntry *entry, *m_pAnniversaries) {
                entry->calculateDays();
            }
        }
    }
    }

    updateModels();
    update();
    useCurrentPlasmaTheme();
}

void KBirthdayApplet::updateEventList(const QList<QVariant> &list, QList<AbstractAnnualEventEntry*>** eventList, bool birthday)
{
    delete *eventList;
    *eventList = new QList<AbstractAnnualEventEntry*>();
    foreach(const QVariant &person, list){
        KabEntry ke = person.value<KabEntry>();
        AbstractAnnualEventEntry *entry;
	if (birthday) entry = new BirthdayEntry(ke.first, ke.second);
	else entry = new NamedayEntry(ke.first, ke.second);
        (*eventList)->append(entry);
    }
}

void KBirthdayApplet::updateEventCount()
{
    delete m_pShownEntries;
    m_pShownEntries = new QList<AbstractAnnualEventEntry*>();
    if (m_pBirthdays) m_pShownEntries->append(*m_pBirthdays);
    if (m_pAnniversaries && m_showAnniversaries) {
        if (true) {
            qSort(m_pAnniversaries->begin(), m_pAnniversaries->end(), AbstractAnnualEventEntry::lessThan);
            QMap<QDate, AggregatedNamedayEntry*> aggregatedEntries;
            int curYear = QDate::currentDate().year();

            foreach(AbstractAnnualEventEntry *entry, *m_pAnniversaries) {
                NamedayEntry *namedayEntry = (NamedayEntry *) entry;
                const QDate namedayDate = entry->date();
                QDate curYearDate(curYear, namedayDate.month(), namedayDate.day());

                AggregatedNamedayEntry *aggregatedEntry = 0;
                if (aggregatedEntries.contains(curYearDate))
                    aggregatedEntry = aggregatedEntries[curYearDate];
                else {
                    aggregatedEntry = new AggregatedNamedayEntry("aggreg", curYearDate);
                    aggregatedEntries[curYearDate] = aggregatedEntry;
                }
                aggregatedEntry->addNamedayEntry(*namedayEntry);
            }

            foreach(AggregatedNamedayEntry *entry, aggregatedEntries) {
                m_pShownEntries->append(entry);
            }
        }
        else {
            m_pShownEntries->append(*m_pAnniversaries);
        }
    }
    qSort(m_pShownEntries->begin(), m_pShownEntries->end(), AbstractAnnualEventEntry::lessThan);
  
    kDebug() << "Birthday event count: " << m_pShownEntries->size();
}

bool KBirthdayApplet::testThreshold(const int remainingDays)
{
  return ( (remainingDays > 0 && remainingDays <= m_eventTreshold) ||
          (remainingDays < 0 && remainingDays >= (m_pastTreshold* -1)) ||
                remainingDays == 0)?true:false;
}


#include "kbirthdayapplet.moc"
