/****************************************************************************
**
** Copyright (C) 2005-2007 Ralf Habacker. All rights reserved.
**
** This file is part of the KDE installer for windows
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License version 2 as published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public License
** along with this library; see the file COPYING.LIB.  If not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA 02110-1301, USA.
**
****************************************************************************/

#include <QtDebug>
#include <QDir>
#include <QTreeWidget>
#include <QFlags>

#include "installerengine.h"

#include "downloader.h"
#include "installer.h"
#include "installerprogress.h"
#include "package.h"
#include "packagelist.h"
#include "globalconfig.h"
#include "database.h"

InstallerEngine::InstallerEngine(DownloaderProgress *progressBar,InstallerProgress *instProgressBar)
{
	m_progressBar = progressBar;
	m_instProgressBar = instProgressBar; 
	m_database = &Database::getInstance();
}

void InstallerEngine::readGlobalConfig()
{
	m_downloader = new Downloader(/*blocking=*/ true,m_progressBar);
    m_globalConfig = new GlobalConfig(QString("http://82.149.170.66/kde-windows/installer/config.txt"),*m_downloader);
    createMainPackagelist();
}

void InstallerEngine::createMainPackagelist()
{
    // package list is build from packages defined in global configuration
    PackageList *packageList = new PackageList();
    packageList->setConfigFileName("packages-other.txt");
    packageList->setName("main");
    m_packageListList.append(packageList);

    Installer *installer = new Installer(packageList,m_instProgressBar );
    installer->setRoot(root());
    m_installerList.append(installer);
	m_database->readFromDirectory(root()+"/manifest");
	m_database->listPackages("Package");
    QList<Package*>::iterator p;
    for (p = m_globalConfig->packages()->begin(); p != m_globalConfig->packages()->end(); p++)
    {
        packageList->addPackage(*(*p));
    }
    
    // @TODO: cleanup
#if 1
	packageList->syncWithDatabase(*m_database);
#else
    if (packageList->hasConfig())
        packageList->syncWithFile();

    packageList->writeToFile();
#endif
}

/// download all packagelists, which are available on the configured sites
bool InstallerEngine::downloadPackageLists()
{
    QList<Site*>::iterator s;
    for (s = m_globalConfig->sites()->begin(); s != m_globalConfig->sites()->end(); s++)
    {
        qDebug() << "download package file list for site: " << (*s)->name();
        PackageList *packageList = new PackageList();
        packageList->setConfigFileName("packages-" + (*s)->name() + ".txt");
        packageList->setName((*s)->name());
        m_packageList = packageList;
        m_packageListList.append(packageList);
        Installer *installer = new Installer(packageList,m_instProgressBar );
        // packagelist needs to access Site::getDependencies() && Site::isExclude()
        packageList->setCurrentSite(*s);

        // FIXME:: hardcoded name, better to use an option in the config file ?
        if ((*s)->name() == "gnuwin32")
        {
            installer->setType(Installer::GNUWIN32);
            // FIXME: add additional option in config.txt for mirrors
            packageList->setBaseURL("http://heanet.dl.sourceforge.net/sourceforge/gnuwin32/");
        }
        else
            packageList->setBaseURL((*s)->url());
        installer->setRoot(root());
        m_installerList.append(installer);
        m_installer = installer;

		// FIXME: it is probably better to download package list every 
		//        time and to sync with local copy 

		// download package list
        qDebug() << (*s)->url();
#ifdef DEBUG
        QFileInfo tmpFile(installer->Root() + "/packages-"+(*s)->Name()+".html");
        if (!tmpFile.exists())
            m_downloader->start((*s)->URL(), ba));

        // load and parse
        if (!packageList->readHTMLFromFile(tmpFile.absoluteFilePath(),(*s)->Type() == Site::ApacheModIndex ? PackageList::ApacheModIndex : PackageList::SourceForge ))
#else            
        QByteArray ba;
        m_downloader->start((*s)->url(), ba);
        if (!packageList->readHTMLFromByteArray(ba,(*s)->Type() == Site::ApacheModIndex ? PackageList::ApacheModIndex : PackageList::SourceForge ))
#endif
        {
            qDebug() << "error reading package list from download html file";
            continue;
        }
#if 1
		packageList->syncWithDatabase(*m_database);
#else
		if (packageList->hasConfig())
			packageList->syncWithFile();

		if (!packageList->writeToFile())
		{
			  qDebug() << "error writing package list to file";
			continue;
		}
#endif
	}
	return true;
}

#ifdef USE_GUI


int typeToColumn(Package::Type type)
{
    switch (type)
    {
        case Package::BIN : return 3;
        case Package::LIB : return 4;
        case Package::DOC : return 5;
        case Package::SRC : return 6;
        default: return 2;
    }	
}

Package::Type columnToType(int column)
{
    switch (column)
    {
        case 3: return Package::BIN;
        case 4: return Package::LIB; 
        case 5: return Package::DOC; 
        case 6: return Package::SRC; 
        case 2: return Package::ALL;
        default : return Package::NONE;
    }	
}

enum iconType {_install, _autoinstall,_keepinstalled, _update, _remove, _nothing, _disable}; 

void setIcon(QTreeWidgetItem &item, Package::Type type, iconType action)
{
    static QIcon *ai;
    static QIcon *ii;
    static QIcon *ki;
    static QIcon *ni;
    static QIcon *id;
    static QIcon *dl;
    static QIcon *up;
 
    if (!ii) 
    {
        ai = new QIcon(":/images/autoinstall.xpm");
        ii = new QIcon(":/images/install.xpm");
        ki = new QIcon(":/images/keepinstalled.xpm");
        ni = new QIcon(":/images/noinst.xpm");
        id = new QIcon(":/images/install_disabled.xpm");
        dl = new QIcon(":/images/del.xpm");
        up = new QIcon(":/images/update.xpm");
    }

    int column = typeToColumn(type);
	switch(action)
	{
        case _autoinstall: item.setIcon(column,*ai); return;
        case _install: item.setIcon(column,*ii); return;
        case _keepinstalled: item.setIcon(column,*ki); return;
        case _remove:  item.setIcon(column,*dl); return;
        case _update:  item.setIcon(column,*up); return;
        case _nothing: item.setIcon(column,*ni); return;
        case _disable: item.setIcon(column,*id); return;
    }
    // FIXME: does not work, don't know how to set the icon size 
    // item.icon(column).setIconSize(QSize(22,22));
}

enum actionType { _initial, _next, _deps, _sync}; 
enum stateType { _Install, _Update, _Remove, _Nothing}; 

static void setState(QTreeWidgetItem &item, const Package *pkg, int column, actionType action, int syncColumn=0)
{
    Package::Type type = columnToType(column);

    if(!pkg)
        return;
    if (type != Package::ALL && !pkg->hasType(type)) 
    {
        setIcon(item,type,_disable);
        return; 
    }

    switch (action)
    {
        case _initial: 
            if (pkg->isInstalled(type))
            {
                setIcon(item,type,_keepinstalled );
                item.setData(column,Qt::UserRole,_Nothing);
            }
            else
            {
                setIcon(item,type,_nothing);
                item.setData(column,Qt::UserRole,_Nothing);
            }
            break;
        
        // enter next state depending on current state 
        case _next: 
        {
            stateType state = (stateType)item.data(column,Qt::UserRole).toInt();

            switch(state)
            {
                case _Nothing:  
                    if (pkg->isInstalled(type))
                    {
                        setIcon(item,type,_remove);
                        item.setData(column,Qt::UserRole,_Remove);
                    }
                    else
                    {
                        setIcon(item,type,_install);
                        item.setData(column,Qt::UserRole,_Install);
                    }
                    break;

                case _Remove:  
                    if (pkg->isInstalled(type))
                    {
                        setIcon(item,type,_keepinstalled );
                        item.setData(column,Qt::UserRole,_Nothing);
                    }
                    else
                    {
                        item.setData(column,Qt::UserRole,_Nothing);
                        setIcon(item,type,_nothing);  
                    }
                    break;

                case _Install:  
                    item.setData(column,Qt::UserRole,_Nothing);
                    setIcon(item,type,_nothing);  
                    break;
            }
            break;
        }
        // handle dependeny selecting 
        case _deps: 
        {
            if (pkg->isInstalled(type))
            {
            }
            else
            {
                // FIXME: should be _autoinstall, but then the main package is using this icon too 
                setIcon(item,type,_install);
                item.setData(column,Qt::UserRole,_Install);
            }
            break;
        }
        case _sync: 
            stateType state = (stateType)item.data(syncColumn,Qt::UserRole).toInt();
            switch(state) 
            {
                case _Install: 
                    if (!pkg->isInstalled(type))
                    {
                        setIcon(item,type,_install);
                        item.setData(column,Qt::UserRole,_Install);
                    }
                    break;
                case _Nothing: 
                    if (!pkg->isInstalled(type))
                    {
                        setIcon(item,type,_nothing);
                        item.setData(column,Qt::UserRole,_Nothing);
                    }
                    break;
                case _Remove:
                    break;
            }                       
	}        
}

bool isMarkedForInstall(QTreeWidgetItem &item,Package::Type type)
{
    int column = typeToColumn(type);
    stateType state = (stateType)item.data(column,Qt::UserRole).toInt();
    return state == _Install;
}

bool isMarkedForRemoval(QTreeWidgetItem &item,Package::Type type)
{
    int column = typeToColumn(type);
    stateType state = (stateType)item.data(column,Qt::UserRole).toInt();
    return state == _Remove;
}

void InstallerEngine::setPageSelectorWidgetData(QTreeWidget *tree)
{
    QStringList labels;
    QList<QTreeWidgetItem *> items;
    // QTreeWidgetItem *item;

    labels
    << "Package"
    << "Version"
    << "all"
    << "bin"
    << "lib"
    << "doc"
    << "src"
    << "Notes";

	tree->setColumnCount(8);
    tree->setHeaderLabels(labels);
    // see http://lists.trolltech.com/qt-interest/2006-06/thread00441-0.html
    // and Task Tracker Entry 106731
    //tree->setAlignment(Center);

    // adding top level items
    QList<QTreeWidgetItem *> categoryList;

    qDebug() << "adding categories size:" << m_globalConfig->sites()->size();

    QList <PackageList *>::ConstIterator k = m_packageListList.constBegin();
    for ( ; k != m_packageListList.constEnd(); ++k)
    {
        QTreeWidgetItem *category = new QTreeWidgetItem((QTreeWidget*)0, QStringList((*k)->Name()));
        categoryList.append(category);

        // adding sub items
        QList<Package*>::ConstIterator i = (*k)->packageList().constBegin();
        for ( ; i != (*k)->packageList().constEnd(); ++i)
        {
            QStringList data;
            data << (*i)->name()
                 << (*i)->version()
                 << QString();
            QTreeWidgetItem *item = new QTreeWidgetItem(category, data);
            setState(*item,*i,2,_initial);
            setState(*item,*i,3,_initial);
            setState(*item,*i,4,_initial);
            setState(*item,*i,5,_initial);
            setState(*item,*i,6,_initial);
            item->setText(7, (*i)->notes());
        }
    }
    tree->insertTopLevelItems(0,categoryList);
    tree->expandAll();
	tree->sortItems(0,Qt::AscendingOrder);
}
extern QTreeWidget *tree;

void InstallerEngine::itemClickedPackageSelectorPage(QTreeWidgetItem *item, int column)
{
    if (column < 2)
        return;

    Package *pkg = getPackageByName(item->text(0));

    if (column == 2 && pkg)
    {
       setState(*item,pkg,2,_next);
       setState(*item,pkg,3,_sync,2);
       setState(*item,pkg,4,_sync,2);
       setState(*item,pkg,5,_sync,2);
       setState(*item,pkg,6,_sync,2);
    }
    else
       setState(*item,pkg,column,_next);

    // select depending packages in case all or bin is selected
    //pkg->dump();
    if (column == 2 || column == 3 && pkg) 
    {
        const QStringList &deps = pkg->deps();

        qDebug() << deps.join(" ");    
        for (int i = 0; i < deps.size(); ++i)
        {  
            QString dep = deps.at(i);
            QList<QTreeWidgetItem *> items = tree->findItems(deps.at(i),Qt::MatchFixedString | Qt::MatchRecursive);
            qDebug() << items.size();
            for (int j = 0; j < items.size(); ++j) 
            {
           	    qDebug() << items.at(j);
                QTreeWidgetItem * depItem = static_cast<QTreeWidgetItem*>(items[j]);
                /// the dependency is only for bin package and one way to switch on
                Package *depPkg = getPackageByName(dep);

                setState(*depItem,depPkg,3,_deps);
            }
        }    
    }

}

bool InstallerEngine::downloadPackages(QTreeWidget *tree, const QString &category)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = static_cast<QTreeWidgetItem*>(tree->topLevelItem(i));
#ifdef DEBUG
        qDebug() << __FUNCTION__ << " " << item->text(0);
#endif
        if (category.isEmpty() || item->text(0) == category)
        {
            PackageList *packageList = getPackageListByName(item->text(0));
            if (!packageList)
            {
                qDebug() << __FUNCTION__ << " packagelist for " << item->text(0) << " not found";
                continue;
            }
            for (int j = 0; j < item->childCount(); j++)
            {
                QTreeWidgetItem *child = static_cast<QTreeWidgetItem*>(item->child(j));
//              qDebug("%s %s %d",child->text(0).toAscii().data(),child->text(1).toAscii().data(),child->checkState(2));
				bool all = child->checkState(2) == Qt::Checked;
				Package *pkg = packageList->getPackage(child->text(0));
				if (!pkg)
					continue;
				if (all | isMarkedForInstall(*child,Package::BIN))
			        pkg->downloadItem(m_downloader, Package::BIN);
				if (all | isMarkedForInstall(*child,Package::LIB))
			        pkg->downloadItem(m_downloader, Package::LIB);
				if (all | isMarkedForInstall(*child,Package::DOC))
			        pkg->downloadItem(m_downloader, Package::DOC);
				if (all | isMarkedForInstall(*child,Package::SRC))
			        pkg->downloadItem(m_downloader, Package::SRC);
            }
        }
    }
    return true;
}

bool InstallerEngine::removePackages(QTreeWidget *tree, const QString &category)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = static_cast<QTreeWidgetItem*>(tree->topLevelItem(i));
        if (category.isEmpty() || item->text(0) == category)
        {
            PackageList *packageList = getPackageListByName(item->text(0));
            if (!packageList)
            {
                qDebug() << __FUNCTION__ << " packagelist for " << item->text(0) << " not found";
                continue;
            }
            for (int j = 0; j < item->childCount(); j++)
            {
                QTreeWidgetItem *child = static_cast<QTreeWidgetItem*>(item->child(j));
				bool all = false; //child->checkState(2) == Qt::Checked;
				Package *pkg = packageList->getPackage(child->text(0));
				if (!pkg)
					continue;
				if (all | isMarkedForRemoval(*child,Package::BIN))
			        pkg->removeItem(m_installer, Package::BIN);
				if (all | isMarkedForRemoval(*child,Package::LIB))
			        pkg->removeItem(m_installer, Package::LIB);
				if (all | isMarkedForRemoval(*child,Package::DOC))
			        pkg->removeItem(m_installer, Package::DOC);
				if (all | isMarkedForRemoval(*child,Package::SRC))
			        pkg->removeItem(m_installer, Package::SRC);
			}
        }
    }
    return true;
}

bool InstallerEngine::installPackages(QTreeWidget *tree,const QString &category)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = static_cast<QTreeWidgetItem*>(tree->topLevelItem(i));
#ifdef DEBUG
        qDebug() << __FUNCTION__ << " " << item->text(0);
#endif
        if (category.isEmpty() || item->text(0) == category)
        {
            PackageList *packageList = getPackageListByName(item->text(0));
            if (!packageList)
            {
                qDebug() << __FUNCTION__ << " packagelist for " << item->text(0) << " not found";
                continue;
            }
            for (int j = 0; j < item->childCount(); j++)
            {
                QTreeWidgetItem *child = static_cast<QTreeWidgetItem*>(item->child(j));
//                qDebug("%s %s %d",child->text(0).toAscii().data(),child->text(1).toAscii().data(),child->checkState(2));
				bool all = child->checkState(2) == Qt::Checked;
				Package *pkg = packageList->getPackage(child->text(0));
				if (!pkg)
					continue;
				if (all | isMarkedForInstall(*child,Package::BIN))
					pkg->installItem(m_installer, Package::BIN);
				if (all | isMarkedForInstall(*child,Package::LIB))
					pkg->installItem(m_installer, Package::LIB);
				if (all | isMarkedForInstall(*child,Package::DOC))
					pkg->installItem(m_installer, Package::DOC);
				if (all | isMarkedForInstall(*child,Package::SRC))
					pkg->installItem(m_installer, Package::SRC);

				// if package does not have manifest files create one
            }
			packageList->writeToFile();
        }
    }
    return true;
}

#else

void InstallerEngine::listPackages(const QString &title)
{
    QList <PackageList *>::iterator k;
    for (k = m_packageListList.begin(); k != m_packageListList.end(); ++k)
    {
       qDebug() << (*k)->Name();
       (*k)->listPackages(title);
   }
}

bool InstallerEngine::downloadPackages(const QStringList &packages, const QString &category)
{
    QList<Site*>::ConstIterator s = m_globalConfig->sites()->constBegin();
    for ( ; s != m_globalConfig->sites()->constEnd(); s++)
    {
       qDebug() << (*s)->name();
       PackageList *packageList = getPackageListByName((*s)->name());
       if (!packageList)
       {
           qDebug() << __FUNCTION__ << " packagelist for " << (*s)->name() << " not found";
           continue;
       }
       foreach(QString package, packages)
       {
           if (!packageList->downloadPackage(package))
               qDebug() << "could not download package" << package;
       }
   }
   return true;
}

bool InstallerEngine::installPackages(const QStringList &packages,const QString &category)
{
    QList<Site*>::ConstIterator s = m_globalConfig->sites()->constBegin();
    for ( ; s != m_globalConfig->sites()->constEnd(); s++)
    {
       qDebug() << (*s)->name();
       PackageList *packageList = getPackageListByName((*s)->name());
       if (!packageList)
       {
           qDebug() << __FUNCTION__ << " packagelist for " << (*s)->name() << " not found";
           continue;
       }
       foreach(QString package, packages)
       {
           if (!packageList->installPackage(package))
               qDebug() << "could not download package" << package;
       }
       packageList->writeToFile();
   }
   return true;
}
#endif

PackageList *InstallerEngine::getPackageListByName(const QString &name)
{
    QList <PackageList *>::iterator k;
    for (k = m_packageListList.begin(); k != m_packageListList.end(); ++k)
    {
        if ((*k)->Name() == name)
            return (*k);
    }
    return 0;
}

Package *InstallerEngine::getPackageByName(const QString &name)
{
	  Package *pkg;
    QList <PackageList *>::iterator k;
    for (k = m_packageListList.begin(); k != m_packageListList.end(); ++k)
    {
       pkg = (*k)->getPackage(name);
       if (pkg)
           return pkg;
    }
    return 0;
}

void InstallerEngine::setRoot(QString root)
{
    Settings::getInstance().setInstallDir(root);
}

const QString InstallerEngine::root()
{
    QString root = Settings::getInstance().installDir();
    if (root.isEmpty())
    {
        root = QDir::currentPath();
        Settings::getInstance().setInstallDir(root);
    }
    return QDir::convertSeparators(root);
}

void InstallerEngine::stop()
{
    m_downloader->cancel();
}
