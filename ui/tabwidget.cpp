/*
                kueue - keep track of your SR queue
         (C) 2011 - 2012 Stefan Bogner <sbogner@suse.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the

    Free Software Foundation, Inc.
    59 Temple Place - Suite 330
    Boston, MA  02111-1307, USA

    Have a lot of fun :-)

*/

#include "tabwidget.h"
#include "settings.h"
#include "kueuewindow.h"
#include "kueue.h"
#include "nsa/nsa.h"
#include "kueuethreads.h"
#include "browsers/helpbrowser.h"

#include <QGridLayout> 
#include <QMenu>
#include <QDesktopServices>
#include <QShortcut>
#include <QFileDialog>

TabWidget* TabWidget::instance = 0;

TabWidget& TabWidget::tw()
{
    if ( !instance )
    {
        instance = new TabWidget;
    }
 
    return *instance; 
}

void TabWidget::destroy()
{
    if ( instance )
    {
        delete instance;
    }
      
    instance = 0;
}

TabWidget::TabWidget( QWidget* parent )
{
    qDebug() << "[TABWIDGET] Constructing";

    // set the tab position 
    
    setTabsPosition();
    mStatusBar = &mStatusBar->getInstance();
    // create the tabbar
    
    mBar = new TabBar;
    setTabBar( mBar );
    
    // connect the mouse clicks
    
    connect( mBar, SIGNAL( tabRightClicked( int, QPoint ) ),
             this, SLOT( tabRightClicked( int, QPoint ) ) );
    
    connect( mBar, SIGNAL( tabMiddleClicked( int, QPoint ) ),
             this, SLOT( tabMiddleClicked( int, QPoint ) ) );
   
    TabButton* newTabButton = new TabButton( this );
    TabButton* mMenuButton = new TabButton( this );
    
    connect( newTabButton, SIGNAL(clicked()), 
             this, SLOT( addUnityBrowser()) );
    
    connect( mMenuButton, SIGNAL( clicked() ),
             mMenuButton, SLOT( showMenu()) );
    
    mMenuButton->setMenu( kueueMainMenu() );

    newTabButton->setIcon( QIcon( ":icons/menus/newtab.png" ) );
    mMenuButton->setIcon( QIcon(":/icons/kueue.png") );
    
    if ( Settings::unityEnabled() )
    {
        setCornerWidget( newTabButton, Qt::TopRightCorner );
    }
    
    setCornerWidget( mMenuButton, Qt::TopLeftCorner );

    // create the main browser tabs...
    
    mQueueBrowser = new QueueBrowser( this );
    mPersonalTab = new WebViewWithSearch( mQueueBrowser, this );
    
    connect( mQueueBrowser, SIGNAL( setMenus() ), 
             this, SLOT( setMenus() ) );
    
    connect( mQueueBrowser, SIGNAL( expandAll() ),
             this, SLOT( expandAllTables() ) );
        
    connect( mQueueBrowser, SIGNAL( closeAll() ),
             this, SLOT( closeAllTables() ) );
    
    mQmonBrowser = new QMonBrowser( this );
    mMonitorTab = new WebViewWithSearch( mQmonBrowser, this );

    mStatsBrowser = new StatsBrowser( this );
    mStatsTab = new WebViewWithSearch( mStatsBrowser, this );

    mUnityTab = new UnityWidget( this );
    mUnityBrowser = mUnityTab->browser();
    
    // ...and add them to the tabbar
    
    insertTab( 0, mPersonalTab, QIcon( ":icons/conf/targets.png" ), "Personal queue" );
    insertTab( 1, mMonitorTab, QIcon( ":/icons/conf/monitor.png" ), "Queue monitor" );
    insertTab( 2, mStatsTab, QIcon( ":/icons/conf/stats.png" ), "Statistics" );
    insertTab( 3, mUnityTab, QIcon( ":/icons/menus/siebel.png" ), "Unity" );    
    
    QShortcut* search = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ), this );
   
    connect( search, SIGNAL( activated() ), 
             this, SLOT( showSearch() ) );
    
    refreshTabs();
}

TabWidget::~TabWidget()
{
    qDebug() << "[TABWIDGET] Destroying";
}

void TabWidget::refreshTabs()
{
    int current = currentIndex();
    
    if ( !Settings::monitorEnabled() ) 
    {
        showMonitorTab( false );
    }
    else 
    {   
        showMonitorTab( true );
    }
    
    if ( !Settings::statsEnabled() ) 
    {
        showStatsTab( false );
    }
    else 
    {
        showStatsTab( true );
    }
    
    if ( !Settings::unityEnabled() )
    {
        showUnityTab( false );
    }
    else
    {
        showUnityTab( true );
    }
    
    setCurrentIndex( current );
}

void TabWidget::setTabsPosition()
{
    if ( Settings::showTabsAtTop() )
    {
        setTabPosition( QTabWidget::North );
    }
    else
    {
        setTabPosition( QTabWidget::South );
    }
}

void TabWidget::tabMiddleClicked( int id, QPoint point )
{
    if ( mUnityBrowserMap.keys().contains( id ) )
    {
        openClipboardInUnity( id );
    }
}

void TabWidget::tabRightClicked( int id, QPoint point )
{
    if ( mUnityBrowserMap.keys().contains( id ) )
    {
        unityTabMenu( id, point );
    }
}

void TabWidget::showPersonalTab( bool b )
{
    if ( b )
    {
        insertTab( 0, mPersonalTab, QIcon( ":/icons/conf/targets.png" ), "Personal queue" );
    }
    else
    {
        removeTab( indexOf( mPersonalTab ) );
    }
}

void TabWidget::showMonitorTab( bool b )
{
    if ( b )
    {
        insertTab( 1, mMonitorTab, QIcon( ":/icons/conf/monitor.png" ), "Queue monitor" );
    }
    else
    {
        removeTab( indexOf( mMonitorTab ) );
    }
}

void TabWidget::showStatsTab( bool b )
{
    if ( b )
    {
        insertTab( 2, mStatsTab, QIcon( ":/icons/conf/stats.png" ), "Statistics" );
    }
    else
    {
        removeTab( indexOf( mStatsTab ) );
    }
}

void TabWidget::showUnityTab( bool b )
{
    if ( b )
    {
        insertTab( 3, mUnityTab, QIcon( ":/icons/menus/siebel.png" ), "Unity" );
    }
    else
    {
        removeTab( indexOf( mUnityTab ) );    
    }
}

void TabWidget::addUnityBrowser()
{   
    // create a new unitywidget and add it as a tab
    
    UnityWidget* w = new UnityWidget( this );

    int tab = addTab( w, QIcon( ":/icons/menus/siebel.png" ), "Unity" );
    
    // set the tabId for the widget for tab handling 
    
    w->setTabId( tab );
    
    // append the widget to a list to build the map from when a tab is closed
    // and set the id + browser in the map for identification
    
    mUnityWidgetList.append( w );
    mUnityBrowserMap[ tab ] = w->browser();
}

void TabWidget::removeUnityBrowser( int tab )
{
    // first remove the tab from the tabbar
    
    removeTab( tab );
    
    // then delete the associated unitybrowser
    
    delete mUnityBrowserMap[ tab ];
    
    // clear the browser map...
    
    mUnityBrowserMap.clear();
    
    // ...remove the tab widget from the widget list...
    
    for ( int i = 0; i < mUnityWidgetList.count(); ++i )
    {
        if ( mUnityWidgetList.at( i )->tabId() == tab )
        {
            delete mUnityWidgetList.at( i );
            mUnityWidgetList.removeAt( i );
        }
    }
    
    // ...and rebuild the map with the correct tab ids
    
    for ( int i = 0; i < mUnityWidgetList.count(); ++i )
    {
        QWidget* tw = qobject_cast< QWidget* >( mUnityWidgetList.at(i) );

        mUnityWidgetList.at( i )->setTabId( indexOf( tw ) );
        mUnityBrowserMap[ indexOf( tw ) ] = mUnityWidgetList.at( i )->browser();
    }
}

void TabWidget::updateQmonBrowser()
{
    mQmonBrowser->update();
}

void TabWidget::updateQueueBrowser()
{
    mQueueBrowser->update();
}

void TabWidget::updateStatsBrowser()
{
    mStatsBrowser->update();
}

void TabWidget::showSrInUnityBrowser(int browser, const QString& sr )
{
    mUnityBrowserMap[ browser ]->querySR( sr );
}

void TabWidget::switchToMainUnityTab()
{
    setCurrentIndex( indexOf( mUnityTab ) );
}

void TabWidget::switchToTab( int index )
{
    setCurrentIndex( index );
}

QMenu* TabWidget::kueueMainMenu()
{
    QMenu* menu = new QMenu( this );
    
    QMenu* kueue = new QMenu( menu );
    kueue->setIcon( QIcon( ":/icons/kueue.png" ) );
    kueue->setTitle( "Kueue" );
    
    mActionConfig = new QAction( kueue );
    mActionConfig->setText( "Configure kueue" );
    mActionConfig->setIcon( QIcon( ":/icons/menus/configure.png" ) );
    
    mActionNSA = new QAction( kueue );
    mActionNSA->setText( "Generate NSA Report" );
    mActionNSA->setIcon( QIcon( ":/icons/menus/nsa.png" ) );
    
    if ( Settings::unityEnabled() )
    {
        mActionAddUnityTab = new QAction( kueue );
        mActionAddUnityTab->setText( "Add Unity tab" );
        mActionAddUnityTab->setIcon( QIcon( ":/icons/menus/web.png" ) );
    }
        
    mActionQuit = new QAction( kueue );
    mActionQuit->setText( "Quit" );
    mActionQuit->setIcon( QIcon( ":/icons/menus/quit.png" ) );
    
    kueue->addAction( mActionConfig );
    
    if ( Settings::unityEnabled() )
    {
        kueue->addAction( mActionAddUnityTab );
    }
    
    #ifndef IS_WIN32
    kueue->addAction( mActionNSA );    
    #endif
    kueue->addAction( mActionQuit );
    
    QMenu* view = new QMenu( menu );
    view->setIcon( QIcon( ":/icons/menus/sort.png" ) );
    view->setTitle( "View" );
    
    QMenu* filter = new QMenu( menu );
    filter->setIcon( QIcon( ":/icons/menus/filter.png" ) );
    filter->setTitle( "Filter" );

    mActionAwaitingCustomer = new QAction( filter );
    mActionAwaitingCustomer->setText( "Awaiting Customer" );
    mActionAwaitingCustomer->setCheckable( true );
    
    mActionAwaitingSupport = new QAction( filter );
    mActionAwaitingSupport->setText( "Awaiting Support" );
    mActionAwaitingSupport->setCheckable( true );
    
    mActionOthers = new QAction( filter );
    mActionOthers->setText( "Other" );
    mActionOthers->setCheckable( true );
    
    mActionShowCR = new QAction( filter );
    mActionShowCR->setText( "Show CRs" );
    mActionShowCR->setCheckable( true );
    
    mActionShowSR = new QAction( filter );
    mActionShowSR->setText( "Show SRs" );
    mActionShowSR->setCheckable( true );
    
    filter->addAction( mActionAwaitingCustomer );
    filter->addAction( mActionAwaitingSupport );
    filter->addAction( mActionOthers );
    filter->addSeparator();
    filter->addAction( mActionShowCR );
    filter->addAction( mActionShowSR );
    
    view->addMenu( filter );
    
    QMenu* sortby = new QMenu( menu );
    sortby->setIcon( QIcon( ":/icons/menus/sort.png" ) );
    sortby->setTitle( "Sort by" );
    
    mActionSortUpdate = new QAction( sortby );
    mActionSortUpdate->setText( "Last activity" );
    mActionSortUpdate->setCheckable( true );
    
    mActionSortAge = new QAction( sortby );
    mActionSortAge->setText( "Age" );
    mActionSortAge->setCheckable( true );
    
    sortby->addAction( mActionSortUpdate );
    sortby->addAction( mActionSortAge );
    
    view->addMenu( sortby );
    
    QMenu* todolist = new QMenu( menu );
    todolist->setIcon( QIcon( ":/icons/menus/todo.png" ) );
    todolist->setTitle( "TO-DO List" );
    
    mActionTodoShowUp = new QAction( todolist );
    mActionTodoShowUp->setText( "Update" );
    mActionTodoShowUp->setCheckable( true );
    
    mActionTodoShowStat = new QAction( todolist );
    mActionTodoShowStat->setText( "Suspicious status" );
    mActionTodoShowStat->setCheckable( true );
    
    mActionTodoShowSmilies = new QAction( todolist );
    mActionTodoShowSmilies->setText( "Show smilies" );
    mActionTodoShowSmilies->setCheckable( true );
    
    todolist->addAction( mActionTodoShowUp );
    todolist->addAction( mActionTodoShowStat );
    todolist->addSeparator();
    todolist->addAction( mActionTodoShowSmilies );
    
    view->addMenu( todolist );
    
    QMenu* other = new QMenu( menu );
    other->setIcon( QIcon( ":/icons/menus/misc.png" ) );
    other->setTitle( "Other" );

    mActionCloseSrTables = new QAction( other );
    mActionCloseSrTables->setText( "Close all SR tables" );
    
    mActionExpandSrTables = new QAction( other );
    mActionExpandSrTables->setText( "Expand all SR tables" );

    other->addAction( mActionCloseSrTables );
    other->addAction( mActionExpandSrTables );
    
    view->addMenu( other );
    
    QMenu* help = new QMenu( menu );
    help->setIcon( QIcon( ":/icons/menus/help.png" ) );
    help->setTitle( "Help" );

    mActionHelp = new QAction( help );
    mActionHelp->setText( "Help" );
    mActionHelp->setIcon( QIcon( ":/icons/menus/help.png" ) );
    
    mActionBug = new QAction( help );
    mActionBug->setText( "Report Bug" );
    mActionBug->setIcon( QIcon( ":/icons/menus/reportbug.png" ) );
    
    mActionFeature = new QAction( help );
    mActionFeature->setText( "Suggest feature" );
    mActionFeature->setIcon( QIcon( ":/icons/menus/feature.png" ) );
    
    mActionAbout = new QAction( help );
    mActionAbout->setText( "About kueue" );
    mActionAbout->setIcon( QIcon( ":/icons/menus/about.png" ) );
    
    help->addAction( mActionHelp );
    help->addAction( mActionBug );
    help->addAction( mActionFeature );
    help->addAction( mActionAbout );
    
    menu->addMenu( kueue );
    menu->addMenu( view );
    menu->addMenu( help );
    
    setMenus();

    connect( mActionQuit, SIGNAL( activated() ),
             qApp, SLOT( quit() ) );
    connect( mActionConfig, SIGNAL( activated() ),
             this, SIGNAL( openConfig() ) );
    connect( mActionShowSR, SIGNAL( toggled( bool ) ),
             this, SLOT( setShowSR( bool ) ) );
    connect( mActionShowCR, SIGNAL( toggled( bool ) ), 
             this, SLOT( setShowCR( bool ) ) );
    connect( mActionTodoShowUp, SIGNAL( toggled( bool ) ),
             this, SLOT( setTodoShowUp( bool ) ) );
    connect( mActionTodoShowStat, SIGNAL( toggled( bool ) ),
             this, SLOT( setTodoShowStat( bool ) ) );
    connect( mActionTodoShowSmilies, SIGNAL( toggled( bool ) ),
             this, SLOT( setTodoShowSmilies( bool ) ) );
    connect( mActionSortUpdate, SIGNAL( toggled( bool ) ),
             this, SLOT( setSortUpdate( bool ) ) );
    connect( mActionSortAge, SIGNAL( toggled( bool ) ), 
             this, SLOT( setSortAge( bool ) ) );
    connect( mActionAwaitingCustomer, SIGNAL( toggled( bool ) ),
             this, SLOT( setShowAwaitingCustomer( bool ) ) );
    connect( mActionAwaitingSupport, SIGNAL( toggled( bool ) ),
             this, SLOT( setShowAwaitingSupport( bool ) ) );
    connect( mActionOthers, SIGNAL( toggled( bool ) ),
             this, SLOT( setShowStatusOthers( bool ) ) );
    connect( mActionExpandSrTables, SIGNAL( activated() ),
             this, SLOT( expandAllTables() ) );
    connect( mActionCloseSrTables, SIGNAL( activated() ),
             this, SLOT( closeAllTables() ) );
    connect( mActionAbout, SIGNAL( activated() ),
             this, SLOT( aboutDialog() ) );
    connect( mActionBug, SIGNAL( activated() ),
             this, SLOT( reportBug() ) );
    connect( mActionFeature, SIGNAL( activated() ),
             this, SLOT( suggestFeature() ) );
    connect( mActionHelp, SIGNAL( activated() ),
             this, SLOT( openHelp() ) );
    
    if ( Settings::unityEnabled() )
    {
        connect( mActionAddUnityTab, SIGNAL( activated() ), 
                 this, SLOT( addUnityBrowser() ) );
    }
    
    connect( mActionNSA, SIGNAL( activated() ), 
             this, SLOT( makeNsaReport() ) );

    return menu;
}

void TabWidget::unityTabMenu( int tab, const QPoint& p )
{
    QMap<int, QString> map;
    
    QMenu* menu = new QMenu( this );

    QAction* closeTab = new QAction( "Close tab", menu );
    QAction* clipboard = new QAction( "Open SR# in clipboard", menu );

    connect( closeTab, SIGNAL( triggered() ),
             this, SLOT( closeActionTriggered() ) );

    connect( clipboard, SIGNAL( triggered() ),
             this, SLOT( clipboardActionTriggered() ) );

    closeTab->setData( tab );
    clipboard->setData( tab );
    
    closeTab->setIcon( QIcon( ":/icons/menus/quit.png" ) );
    clipboard->setIcon( QIcon( ":/icons/menus/clipboard.png" ) );
    
    menu->addAction( closeTab );
    menu->addAction( clipboard );
    
    menu->exec( p );
}

void TabWidget::openInUnityImp( const QString& sr )
{
    if ( Kueue::isSrNr( sr ) )
    {
        mUnityBrowser->querySR( sr );
        switchToMainUnityTab();
    }
}

void TabWidget::closeActionTriggered()
{
    QAction* action = qobject_cast<QAction*>( QObject::sender() );
    removeUnityBrowser( action->data().toInt() );
    
}

void TabWidget::clipboardActionTriggered()
{
    QAction* action = qobject_cast<QAction*>( QObject::sender() );
    
    if ( Kueue::isSrNr( Kueue::getClipboard() ) )
    {
        showSrInUnityBrowser( action->data().toInt(), Kueue::getClipboard() );
    }
}

void TabWidget::setMenus()
{
    mActionShowSR->setChecked( Settings::showSR() );
    mActionShowCR->setChecked( Settings::showCR() );
    mActionSortUpdate->setChecked( Settings::sortUpdate() );
    mActionSortAge->setChecked( Settings::sortAge() );
    mActionTodoShowUp->setChecked( Settings::todoShowUp() );
    mActionTodoShowStat->setChecked( Settings::todoShowStat() );
    mActionTodoShowSmilies->setChecked( Settings::todoShowSmilies() );
    mActionAwaitingCustomer->setChecked( Settings::showAwaitingCustomer() );
    mActionAwaitingSupport->setChecked( Settings::showAwaitingSupport() );
    mActionOthers->setChecked( Settings::showStatusOthers() );  
}

void TabWidget::closeAllTables()
{
    Database::closeAllTables();
    mQueueBrowser->update();
}

void TabWidget::expandAllTables()
{
    Database::expandAllTables();
    mQueueBrowser->update();
}

void TabWidget::openClipboardInUnity( int browser )
{
    if ( Kueue::isSrNr( Kueue::getClipboard() ) )
    {
        showSrInUnityBrowser( browser, Kueue::getClipboard() );
        switchToTab( browser );
    }
}

void TabWidget::setShowSR( bool s )
{
    Settings::setShowSR( s );
    updateUiData();
}

void TabWidget::setShowCR( bool s )
{
    Settings::setShowCR( s );
    updateUiData();
}

void TabWidget::setTodoShowUp( bool s )
{
    Settings::setTodoShowUp( s );
    updateUiData();
}

void TabWidget::setTodoShowStat( bool s )
{
    Settings::setTodoShowStat( s );
    updateUiData();
}

void TabWidget::setTodoShowSmilies( bool s )
{
    Settings::setTodoShowSmilies( s );
    updateUiData();
}

void TabWidget::setSortAge( bool s )
{
    Settings::setSortUpdate( !s );
    Settings::setSortAge( s );
    mActionSortUpdate->setChecked( !s );    
}

void TabWidget::setSortUpdate( bool s )
{
    Settings::setSortUpdate( s );
    Settings::setSortAge( !s );
    mActionSortAge->setChecked( !s );
    updateUiData();
}

void TabWidget::setShowAwaitingCustomer( bool s )
{
    Settings::setShowAwaitingCustomer( s );
    updateUiData();
}

void TabWidget::setShowAwaitingSupport( bool s )
{
    Settings::setShowAwaitingSupport( s );
    updateUiData();
}

void TabWidget::setShowStatusOthers( bool s )
{
    Settings::setShowStatusOthers( s );
    updateUiData();
}

void TabWidget::aboutDialog()
{
    QMessageBox::about( this, "About", "<b>kueue " + QApplication::applicationVersion() + 
                              "</b><br>(C) 2011 Stefan Bogner<br><a href='mailto:sbogner@suse.com'>sbogner@suse.com</a><br>"
                              "<br>Kueue is a Qt application that helps you to keep track of your SR queue and implements queue "
                              "monitor functionality. It is available for Linux, Windows and Mac OSX." );
}

void TabWidget::reportBug()
{
    QString url = "mailto:sbogner@suse.com?subject=Bug%20in%20kueue%20" + QApplication::applicationVersion() + 
                  "&body=Please%20describe%20the%20bug%20and%20steps%20on%20how%20to%20reproduce%20in%20detail";
    
    QDesktopServices::openUrl( QUrl( url ) );
}

void TabWidget::suggestFeature()
{
    QString url = "mailto:sbogner@suse.com?subject=Feature%20for%20kueue&body=Please%20describe%20the%20feature%20in%20detail.";
    
    QDesktopServices::openUrl( QUrl( url ) );
}

void TabWidget::openHelp()
{
    HelpBrowser* h = new HelpBrowser( this );
    h->show();
    
    connect( h, SIGNAL( rejected() ), 
             this, SLOT( helpClosed() ) );
}

void TabWidget::helpClosed()
{
    HelpBrowser* h = qobject_cast<HelpBrowser*>( sender() );
    delete h;
}

void TabWidget::openWebsite()
{
    QDesktopServices::openUrl( QUrl( "http://www.kueue.tk" ) );
}

void TabWidget::updateUiData()
{
    mQmonBrowser->update();
    mQueueBrowser->update();
    mStatsBrowser->update();
}

void TabWidget::showSearch()
{
    webViewSearch( currentIndex() )->showFind();
}

WebViewSearch* TabWidget::webViewSearch( int index )
{
    QWidget *widget = this->widget( index );
    WebViewWithSearch* webViewWithSearch = qobject_cast<WebViewWithSearch*>( widget );
    
    return webViewWithSearch->m_webViewSearch;
}

/* 
 * 
 *       TabBar class
 * 
 */

TabBar::TabBar( QObject* parent )
{
    qDebug() << "[TABBAR] Constructing";
}

TabBar::~TabBar()
{
    qDebug() << "[TABBAR] Destroying";
}

int TabBar::selectTab( const QPoint &pos ) const
{
    // returns the id for the tab at pos, or -1 if there is no tab
    
    const int tabCount = count();
  
    for ( int i = 0; i < tabCount; ++i )
    {
        if ( tabRect( i ).contains( pos ) )
        {
            return i;
        }
    }
    
    return -1;
}

void TabBar::mousePressEvent( QMouseEvent* event )
{
    if ( ( event->button() == Qt::RightButton ) &&
         ( selectTab( event->pos() ) != -1 ) )
    {
        emit tabRightClicked( selectTab( event->pos() ), event->globalPos() );
    }
    else if ( ( event->button() == Qt::MiddleButton ) &&
              ( selectTab( event->pos() ) != -1 ) )
    {
        emit tabMiddleClicked( selectTab( event->pos() ), event->globalPos() );
    }
    
    QTabBar::mousePressEvent( event );
}

/* 
 * 
 *       TabButton class
 * 
 */

TabButton::TabButton( QWidget* parent ) : QToolButton( parent )
{
    qDebug() << "[TABBUTTON] Constructing";
}

TabButton::~TabButton()
{
    qDebug() << "[TABBUTTON] Destroying";
}

void TabButton::mousePressEvent( QMouseEvent* e )
{
    if ( e->button() == Qt::MiddleButton )
    {
        emit middleClicked();
    }
    
    QToolButton::mousePressEvent( e );
}

void TabWidget::makeNsaReport()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select Supportconfig", QDir::homePath(), "Supportconfig archives (*.tbz)" );
    NSA* n = new NSA( filename ); 
}

#include "tabwidget.moc"
