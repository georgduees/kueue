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
    
    PLEASE NOTE:
    
    This is all very ugly and hacky, mostly due to the fact that there is no
    API and this has to be workarounded by parsing urls, titles, elements etc.
    
    If somebody knows a better way to achieve similar functionality, I'm
    open for suggestions :-)
*/

#include "unitypage.h"

#include <QMessageBox>

void UnityPage::closeSr()
{
    mCloseDialog = new CloseDialog( this, mCurrentSR );
    
    connect( mCloseDialog, SIGNAL( rejected() ), 
             this, SLOT( closeSrRejected() ) );
    
    connect( mCloseDialog, SIGNAL( accepted() ), 
             this, SLOT( closeSrAccepted() ) );
    
    mCloseDialog->exec();
}

void UnityPage::closeSrRejected()
{
    delete mCloseDialog;
}

void UnityPage::closeSrAccepted()
{
    if ( ( ( mViewFrame->findFirstElement( "title" ).toInnerXml() == "Service Request Activities" ) ||
           ( mViewFrame->findFirstElement( "title" ).toInnerXml() == "Service Request Related SRs" ) ) &&
           ( mCurrentSR == mCloseDialog->sr() ) )
    {
        closeSrFirst();
    }
    else
    {
        mCloseSR = true;
        querySR( mCloseDialog->sr() );    
    }
}

bool UnityPage::checkMandatoryForClose()
{
    QWebElementCollection c = mViewFrame->findAllElements( "input" );
    
    for ( int i = 0; i < c.count(); ++i ) 
    {  
        if ( ( c.at(i).attribute( "tabindex" ) == "1025" ) && 
             ( c.at(i).attribute( "onchange" ).startsWith( "trackChange" ) ) )
        {
            if ( c.at( i ).attribute( "value" ).isEmpty() )
            {
                return false;
            }
        }
       
        if ( ( c.at(i).attribute( "tabindex" ) == "1035" ) && 
             ( c.at(i).attribute( "onchange" ).startsWith( "trackChange" ) ) )
        {
            if ( c.at( i ).attribute( "value" ).isEmpty() )
            {
                return false;
            }
        }
    }
   
    return true;
}

void UnityPage::closeSrFirst()
{
    disconnect( mViewFrame, 0, 0, 0 );    
    
    if ( !checkMandatoryForClose() )
    {
        QMessageBox::critical( 0, "Error", "Closing SR not possible, please check the mandatory fields" );
        mCloseSR = false;
        delete mCloseDialog;
    }
    else
    {   
        emit pageErbert( "Closing SR#" + mCloseDialog->sr() );
        
        connect( mViewFrame, SIGNAL( loadFinished( bool ) ),
                this, SLOT( closeSrSecond() ) );   
        
        QWebElementCollection c = mViewFrame->findAllElements( "select" );
        QString changeJS;
        QWebElement done;
        QWebElement e;
        
        for ( int i = 0; i < c.count(); ++i )
        {
            if ( c.at(i).attribute( "id" ).contains( "s_1_1_95" ) )
            {
                changeJS = c.at(i).attribute( "onchange" );
                
                QWebElementCollection d = c.at(i).findAll( "*" );
                
                for ( int i = 0; i < d.count(); ++i )
                {
                    d.at(i).removeAttribute("selected");
                    
                    if ( d.at( i ).attribute( "value" ) == "Closed" )
                    {
                        e = d.at( i );
                    }
                }

                e.setAttribute( "selected", "Yes" );
                mViewFrame->evaluateJavaScript( changeJS );
            }
        }
    }
}

void UnityPage::closeSrSecond()
{
    disconnect( mViewFrame, 0, 0, 0 );
    
    connect( mViewFrame, SIGNAL( loadFinished( bool ) ),
             this, SLOT( closeSrThird() ) );   
    
    QWebElementCollection c = mViewFrame->findAllElements( "select" );
    QString changeJS;
    QWebElement done;
    QWebElement e;
    
    for ( int i = 0; i < c.count(); ++i )
    {
        if ( c.at(i).attribute( "id" ).contains( "s_1_1_96" ) )
        {
            changeJS = c.at(i).attribute( "onchange" );
            
            QWebElementCollection d = c.at(i).findAll( "*" );
            
            for ( int i = 0; i < d.count(); ++i )
            {
                d.at(i).removeAttribute("selected");
                
                if ( d.at( i ).attribute( "value" ) == mCloseDialog->closeReason() )
                {
                    e = d.at( i );
                }
            }

            e.setAttribute( "selected", "Yes" );
            mViewFrame->evaluateJavaScript( changeJS );
        }
    }
}

void UnityPage::closeSrThird()
{
    mSaveSr = true;
    
    disconnect( mViewFrame, 0, 0, 0 );
    
    connect( mViewFrame, SIGNAL( loadFinished( bool ) ), 
             this, SLOT( actionDone() ) );
    
    mCloseSR = false;
    
    delete mCloseDialog;
    actionDone();
}
