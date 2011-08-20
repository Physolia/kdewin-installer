/****************************************************************************
**
** Copyright (C) 2011 Constantin Tudorica <tudalex@gmail.com>
** All rights reserved.
**
** This file is part of the KDE installer for windows
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "mainwindow.h"
#include <attica/provider.h>
#include <attica/providermanager.h>
#include <attica/person.h>
#include <attica/itemjob.h>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QCommandLinkButton>
#include "categorybutton.h"
#include "softwareitem.h"
#include <attica/category.h>
#include <attica/content.h>
#include <QListWidget>
//#include <QDebug>
#include <attica/downloaditem.h>
//#include <QWebView>
#include "installerenginesocial.h"
#include "debug.h"
#include "QtGui/QMessageBox"
#include "softwaredetails.h"
#include "settingspage.h"
#include <QtGui>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    this->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    this->setMinimumWidth(600);


    QHBoxLayout *hbox = new QHBoxLayout;
    m_categories = new QVBoxLayout;
    hbox->addLayout(m_categories);
    m_SoftwareList = new QListWidget;

    hbox->addWidget(m_SoftwareList);
    QWidget *center = new QWidget;
    center->setLayout(hbox);
    this->setCentralWidget(center);

    //Setting up the install engine
    m_installengine = new InstallerEngineSocial();

    connect(m_SoftwareList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(softwareSelected(QListWidgetItem*)));
    connect(&m_manager, SIGNAL(defaultProvidersLoaded()), SLOT(providersChanged()));
    // tell it to get the default Providers
    m_manager.addProviderFromXml(QString::fromLatin1("<provider>"
                                 "<id>opendesktop</id>"
                                 "<location>http://api.opendesktop.org/v1/</location>"
                                 "<name>openDesktop.org</name>"
                                 "<icon></icon>"
                                 "<termsofuse>https://opendesktop.org/terms/</termsofuse>"
                                 "<register>https://opendesktop.org/usermanager/new.php</register>"
                                 "<services>"
                                 "  <person ocsversion=\"1.3\" />"
                                 "  <friend ocsversion=\"1.3\" />"
                                 "  <message ocsversion=\"1.3\" />"
                                 "  <activity ocsversion=\"1.3\" />"
                                 "  <content ocsversion=\"1.3\" />"
                                 "  <fan ocsversion=\"1.3\" />"
                                 "  <knowledgebase ocsversion=\"1.3\" />"
                                 "  <event ocsversion=\"1.3\" />"
                                 "</services>"
                                 "</provider>"));
    m_manager.addProviderFromXml(QString::fromLatin1("<provider>"
                                 "<id>opendesktop</id>"
                                 "<location>http://attica.tudalex.com/api/</location>"
                                 "<name>Kde Windows Test</name>"
                                 "<icon></icon>"
                                 "<termsofuse>https://opendesktop.org/terms/</termsofuse>"
                                 "<register>https://opendesktop.org/usermanager/new.php</register>"
                                 "<services>"
                                 "  <person ocsversion=\"1.3\" />"
                                 "  <friend ocsversion=\"1.3\" />"
                                 "  <message ocsversion=\"1.3\" />"
                                 "  <activity ocsversion=\"1.3\" />"
                                 "  <content ocsversion=\"1.3\" />"
                                 "  <fan ocsversion=\"1.3\" />"
                                 "  <knowledgebase ocsversion=\"1.3\" />"
                                 "  <event ocsversion=\"1.3\" />"
                                 "</services>"
                                 "</provider>"));
    this->menuBar()->addAction("Settings",this,SLOT(displaySettings()));


}
void MainWindow::displaySettings()
{
    qDebug()<<"should display settings page";
    SettingsPage *settings_window = new SettingsPage();
    connect(settings_window,SIGNAL(setRoot(QString)), m_installengine,SLOT(changeRoot(QString)));

    settings_window->setCurrentRoot(Settings::instance().installDir());
    settings_window->show();
}


void MainWindow::providersChanged()
{
    qDebug()<<"provider has changed";
    if (!m_manager.providers().isEmpty()) {
        qDebug()<<"provider list is not empty";
        m_provider = m_manager.providerByUrl(QUrl(QLatin1String("http://attica.tudalex.com/api/")));
      //  m_provider = m_manager.providerByUrl(QUrl(QString::fromAscii("http://api.opendesktop.org/v1/")));
        if (!m_provider.isValid()) {
            qDebug()<<"provider is not valid";
            return;
        }
        Attica::ListJob<Attica::Category>* job = m_provider.requestCategories();
        connect(job, SIGNAL(finished(Attica::BaseJob*)), SLOT(onContentRecieved(Attica::BaseJob*)));
        job->start();
    }

}

void MainWindow::onContentRecieved(Attica::BaseJob *job)
{
    qDebug()<<"Job finished";
    Attica::ListJob<Attica::Category> *personJob = static_cast< Attica::ListJob<Attica::Category> * >( job );
    if (personJob->metadata().error() == Attica::Metadata::NoError)
    {
        int i = 0;
        Attica::Category::List l(personJob->itemList());
        for (QList <Attica::Category>::iterator it = l.begin(); it!=l.end(); ++ it)
        {
            Attica::Category *cat = new Attica::Category(*it);
            CategoryButton *category_button = new CategoryButton(this,cat);
            category_button->category=cat;

            connect(category_button,SIGNAL(clicked()),this,SLOT(category_selected()));
            m_categories->addWidget(category_button);
            if (i>10) //FIXME: should change this
                break;
            ++i;
        }

    }
    else
        qDebug()<<"job didn't go well";
}

void MainWindow::category_selected()
{
    CategoryButton *test =static_cast < CategoryButton *>(QObject::sender());

    QList <Attica::Category> list;
    list.append(*(test->category));
    Attica::ListJob<Attica::Content>* job = m_provider.searchContents(list);

    connect(job, SIGNAL(finished(Attica::BaseJob*)), SLOT(onContentListRecieved(Attica::BaseJob*)));
    job->start();
}

void MainWindow::onContentListRecieved(Attica::BaseJob *job)
{
    m_SoftwareList->clear();
    Attica::ListJob<Attica::Content> *ContentListJob = static_cast< Attica::ListJob<Attica::Content> * >( job );
    if (ContentListJob->metadata().error() == Attica::Metadata::NoError)
    {
        QString clist;
        Attica::Content::List l(ContentListJob->itemList());
        for (Attica::Content::List::iterator it = l.begin(); it!=l.end(); ++ it)
        {
            Attica::Content *temp_content = new Attica::Content(static_cast < Attica::Content> (*it));
            m_SoftwareList->addItem( new SoftwareItem(temp_content ));
        }

    }

}
void MainWindow::softwareSelected(QListWidgetItem* item)
{
    SoftwareItem *sitem = static_cast < SoftwareItem *> (item);
    qDebug()<<"reached here";
    Attica::Content *content = sitem->getContent();
    qDebug()<<"got content";
    if (content == NULL)
            qDebug()<<"content is NULL";
    else
    {
        qDebug()<<"should now install software named:"<<content->name()<<endl;
        qDebug()<<"software has "<<content->downloadUrlDescriptions().count()<<" instalation methods";
        int n= content->downloadUrlDescriptions().count();
        for (int i = 0; i < n ; ++i)
        {
            Attica::DownloadDescription down = content->downloadUrlDescription(i+1);
            qDebug()<<"Download "<<down.name();

            qDebug()<<"type is "<<down.type();

            qDebug()<<" and a package "<<down.packageName();
            qDebug()<<" from repo "<<down.repository();

            qDebug()<<" a link "<<down.link();
            qDebug()<<endl;

        }
    }


    QString packageName = content->description();
    SoftwareDetails *details_page = new SoftwareDetails();
    connect(details_page,SIGNAL(installpackage(QString)),m_installengine,SLOT(installpackage(QString)));
    connect(details_page,SIGNAL(uninstallpackage(QString)),m_installengine,SLOT(uninstallpackage(QString)));
    details_page->setContent(content);
    details_page->setProvider(m_provider);
    if (m_installengine->isPackageInstalled(content->downloadUrlDescription(1).packageName()))
        details_page->install_status_changed();
    details_page->show();




}


