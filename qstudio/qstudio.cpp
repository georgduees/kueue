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

#include "qstudio.h"

#include <QDebug>
#include <QObject>
#include <QtXml>
#include <QDesktopServices>
#include <QFile>
#include <QDir>
#include <QDateTime>

QStudio::QStudio( const QString& server, const QString& user, const QString& key, bool debug )
{
    qDebug() << "[QSTUDIO] Constructing";

    mServer = server;
    mUser = user;
    mApiKey = key;
    mDebug = debug;
    
    mNAM = new QNetworkAccessManager( this );
    
    connect( mNAM, SIGNAL( authenticationRequired( QNetworkReply*, QAuthenticator* ) ), 
             this, SLOT( authenticate( QNetworkReply*, QAuthenticator* ) ) );
}

QStudio::~QStudio()
{
    qDebug() << "[QSTUDIO] Destroying";
}

void QStudio::log( const QString& f, const QString& x )
{
    if ( mDebug )
    {
        QFile file( QDesktopServices::storageLocation( QDesktopServices::DataLocation ) + "/studio-xml.log" );
        
        if ( !file.open( QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text ) )
        {
            qDebug() << "Logfile not open";
            return;
        }
        
        QString d = "[" + QDateTime::currentDateTime().toString( "MM/dd hh:mm:ss" ) + "]";
        QTextStream out(&file);
        
        out << "\n\n";
        out << "-----" + d + "-----------------------------------------------------------------------------------------";
        out << "\n\n";
        out << f;
        out << "\n\n";
        out << x;
        
        file.close();
    }
}

QString QStudio::getRequest( const QString& req )
{
    QEventLoop loop;
    QString result;
    
    QNetworkRequest request( QUrl( "http://" + mServer + "/api/v2" + req ) );
    
    qDebug() << request.url();
    QNetworkReply *reply = mNAM->get( request );
    reply->ignoreSslErrors(); 
    qDebug() << request.url();
    QObject::connect( reply, SIGNAL( finished() ), 
                      &loop, SLOT( quit() ) );
     
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
             this, SLOT( networkError( QNetworkReply::NetworkError ) ) );
                                    
    loop.exec();
    
    result = reply->readAll();
    
    return result;
}

QString QStudio::putRequest( const QString& req, const QByteArray& data )
{
    QEventLoop loop;
    QString result;
    
    QNetworkRequest request( QUrl( "http://" + mServer + "/api/v2" + req ) );
    QNetworkReply *reply = mNAM->put( request, data );
    qDebug() << request.url(); 
    QObject::connect( reply, SIGNAL( finished() ), 
                      &loop, SLOT( quit() ) );
     
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
             this, SLOT( networkError( QNetworkReply::NetworkError ) ) );
                                    
    loop.exec();
    
    result = reply->readAll();
    
    return result;
}

QString QStudio::postRequest( const QString& req, const QByteArray& data )
{
    QEventLoop loop;
    QString result;
    QByteArray empty;
    
    QNetworkRequest request( QUrl( "http://" + mServer + "/api/v2" + req ) );
    QNetworkReply *reply = mNAM->post( request, empty );
    qDebug() << request.url(); 
    QObject::connect( reply, SIGNAL( finished() ), 
                      &loop, SLOT( quit() ) );
     
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
             this, SLOT( networkError( QNetworkReply::NetworkError ) ) );
                                    
    loop.exec();
    
    result = reply->readAll();
    
    return result;
}

QString QStudio::deleteRequest( const QString& req )
{
    QEventLoop loop;
    QString result;
    
    QNetworkRequest request( QUrl( "http://" + mServer + "/api/v2" + req ) );
    QNetworkReply *reply = mNAM->get( request );
     
    QObject::connect( reply, SIGNAL( finished() ), 
                      &loop, SLOT( quit() ) );
     
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
             this, SLOT( networkError( QNetworkReply::NetworkError ) ) );
                                    
    loop.exec();
    
    result = reply->readAll();
    
    return result;
}

void QStudio::authenticate( QNetworkReply* reply, QAuthenticator* auth )
{
    auth->setUser( mUser );
    auth->setPassword( mApiKey );
}

void QStudio::networkError( QNetworkReply::NetworkError error )
{
    qDebug() << "NETWORK ERROR" << error;
}

QList<TemplateSet> QStudio::getTemplates()
{
    QString xml = getRequest( "/user/template_sets" );
    QList<TemplateSet> list;
    
    QDomDocument doc;
    doc.setContent( xml );
    
    QDomElement docelement = doc.documentElement();
    
    QDomNode n = docelement.firstChild();
    
    while ( !n.isNull() )
    {
        QDomElement e = n.toElement();
        TemplateSet ts;
        
        if ( e.tagName() == "template_set" )
        {
            QDomNode no = n.firstChild();
        
            while ( !no.isNull() )
            {
                QDomElement te = no.toElement();
                  
                if ( te.tagName() == "template" )
                {
                    Template t;
                    QDomNode tn = no.firstChild();
                
                    while ( !tn.isNull() )
                    {
                        QDomElement ele = tn.toElement();
                        
                        if ( ele.tagName() == "appliance_id" )
                        {
                            t.id = ele.text().toInt();
                        }            
                        else if ( ele.tagName() == "name" )
                        {
                            t.name = ele.text();
                        }            
                        else if ( ele.tagName() == "description" )
                        {
                            t.description = ele.text();
                        }            
                        else if ( ele.tagName() == "basesystem" )
                        {
                            t.basesystem = ele.text();
                        }            
                        
                        tn = tn.nextSibling();
                    }

                    ts.templates.append( t );
                }
                else if ( te.tagName() == "name" )
                {
                    ts.name = te.text();
                }
                else if ( te.tagName() == "description" )
                {
                    ts.description = te.text();
                }
            
                no = no.nextSibling();
            }

            n = n.nextSibling();
        }

        list.append( ts );
    }
    
    log( "getTemplates", xml );
    
    return list;
}

Appliance QStudio::cloneAppliance( int id, const QString& name, const QString& arch )
{
    QByteArray empty;
    QString xml = postRequest( "/user/appliances?clone_from=" + QString::number( id ) + "&name=" + name + "&arch=" + arch, empty );
    
    Appliance app;
    
    QDomDocument doc;
    doc.setContent( xml );
    
    QDomElement docelement = doc.documentElement();
    
    QDomNode n = docelement.firstChild();
    
    while ( !n.isNull() )
    {
        QDomElement e = n.toElement();
        
        if ( e.tagName() == "id" )
        {
            app.id = e.text().toInt();
        }
        else if ( e.tagName() == "name" )
        {
            app.name = e.text();
        }
        else if ( e.tagName() == "arch" )
        {
            app.arch = e.text();
        }
        else if ( e.tagName() == "type" )
        {
            app.type = e.text();
        }
        else if ( e.tagName() == "last_edited" )
        {
            app.last_edited = e.text();
        }
        else if ( e.tagName() == "estimated_raw_size" )
        {
            app.estimated_raw_size = e.text();
        }
        else if ( e.tagName() == "estimated_compressed_size" )
        {
            app.estimated_compressed_size = e.text();
        }
        else if ( e.tagName() == "edit_url" )
        {
            app.edit_url = e.text();
        }
        else if ( e.tagName() == "basesystem" )
        {
            app.basesystem = e.text();
        }
        else if ( e.tagName() == "uuid" )
        {
            app.uuid = e.text();
        }
        else if ( e.tagName() == "parent" )
        {
            QDomNode tn = n.firstChildElement( "id" );
            app.parent = n.toElement().text().toInt();
        }

        n = n.nextSibling();
    }
    
    log( "cloneAppliance - ID: " + QString::number( id ) + " - NAME: " + name + " - ARCH: " + arch, xml );
    
    return app;
}

RPM QStudio::uploadRPM( const QString& basesystem, const QString& filename )
{
    RPM rpm;
    
    QByteArray f;
    
    QFile file( filename );
    if ( file.open(QIODevice::ReadOnly) )
    {
        f = file.readAll();
    }
    else
    {
        return rpm;
    }

    QString boundary = "----------------------------81c4238448f2";
                        
    QByteArray filearray;
    filearray += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
    filearray += boundary + "\r";
    filearray += "Content-Disposition: form-data; name=\"file\"; filename=\"" + QFileInfo( filename ).fileName() + "\"\r";
    filearray += "Content-Type: application/octet-stream\r\n";
    filearray += f;
    filearray += "--" + boundary + "--\r\n";    
       
    QString xml = postRequest( "/user/rpms?base_system=" + basesystem, filearray );
    
    QDomDocument doc;
    doc.setContent( xml );
    
    QDomElement docelement = doc.documentElement();
    
    QDomNode n = docelement.firstChild();
    
    while ( !n.isNull() )
    {
        QDomElement e = n.toElement();
        
        if ( e.tagName() == "id" )
        {
            rpm.id = e.text().toInt();
        }
        else if ( e.tagName() == "filename" )
        {
            rpm.filename = e.text();
        }
        else if ( e.tagName() == "size" )
        {
            rpm.size = e.text();
        }
        else if ( e.tagName() == "archive" )
        {
            if ( e.text() == "true" )
            {
                rpm.archive = true;
            }
            else
            {
                rpm.archive = false;
            }
        }
        else if ( e.tagName() == "base_system" )
        {
            rpm.basesystem = e.text();
        }
        
        n = n.nextSibling();
    }
    
    log( "uploadRPM - BASESYSTEM: " + basesystem + " - FILENAME: " + filename, xml );
    log( "uploadRPM - BASESYSTEM: " + basesystem + " - FILENAME: " + filename, filearray );
    
    return rpm;
}

int QStudio::startApplianceBuild( int id )
{
    QByteArray empty;
    QString xml = postRequest( "/user/running_builds?appliance_id=" + QString::number( id ) + "&image_type=vmx", empty );
    
    log( "startApplianceBuild - ID: " + QString::number( id ), xml );
    
    QDomDocument doc;
    doc.setContent( xml );
    QDomElement docelement = doc.documentElement();
    QDomNode n = docelement.firstChildElement( "id" );

    return n.toElement().text().toInt();
}

bool QStudio::addUserRepository( int id )
{
    QByteArray empty;
    QString xml = postRequest( "/user/appliances/" + QString::number( id )  + "/cmd/add_user_repository", empty );
    
    log( "addUserRepository - ID: " + QString::number( id ), xml );
}

BuildStatus QStudio::getBuildStatus( int id )
{
	qDebug() << "getBuildStatus";
    QString xml = getRequest( "/user/running_builds/" + QString::number( id ) );
    
    BuildStatus bs;
    
    QDomDocument doc;
    doc.setContent( xml );
    QDomElement docelement = doc.documentElement();
    
    QDomNode n = docelement.firstChild();
    
    while ( !n.isNull() )
    {
        QDomElement e = n.toElement();
        
        if ( e.tagName() == "id" )
        {
            bs.id = e.text().toInt();
        }
        else if ( e.tagName() == "state" )
        {
           bs.state = e.text();
        }
        else if ( e.tagName() == "percent" )
        {
           bs.percent = e.text().toInt();
        }
        else if ( e.tagName() == "time_elapsed" )
        {
           bs.time_elapsed = e.text().toInt();
        }
        else if ( e.tagName() == "message" )
        {
           bs.state = e.text();
        }
        
        n = n.nextSibling();
    }
    
    log( "getBuildStatus - ID: " + QString::number( id ), xml );
    
    qDebug() << bs.state << bs.percent;
    return bs;
}

#include "qstudio.moc"
