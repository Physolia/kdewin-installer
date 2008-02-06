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

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QUrl>
#include <QTreeWidget>

#include "packagelist.h"
#include "globalconfig.h"
#include "database.h"
#include "downloader.h"
#include "misc.h"
#include "hintfile.h"

PackageList::PackageList()
        : QObject()
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif
    m_root = ".";
    m_configFile = "/packages.txt";
    m_curSite = 0;
    m_parserConfigFileFound = 0;
}

PackageList::~PackageList()
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif
    qDeleteAll(m_packageList);
}

bool PackageList::hasConfig()
{
    return QFile::exists(m_root + m_configFile);
}

void PackageList::addPackage(const Package &package)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__ << package.toString();
#endif

    m_packageList.append(new Package(package));
}

Package *PackageList::getPackage(const QString &name, const QByteArray &version)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif
    QString pkgName = name;
    QString pkgVersion = version;
    // name may have version info included, split it
    if (version.isEmpty())
    {
        QString _pkgName;
        QString _pkgVersion;
        if (PackageInfo::fromString(name, _pkgName, _pkgVersion) && !_pkgVersion.isEmpty())
        {
            pkgName = _pkgName;
            pkgVersion = _pkgVersion;
        }
    }
    QList<Package*>::iterator it = m_packageList.begin();
    for ( ; it != m_packageList.end(); ++it)
    {
        Package *p = *it;
        if (p->name() == pkgName) {
            if(!pkgVersion.isEmpty() && p->version() != pkgVersion)
                continue;
            return p;
        }
    }
    return NULL;
}

void PackageList::dumpPackages(const QString &title)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif

    qDebug() << title;
    QList<Package*>::iterator it;
    for (it = m_packageList.begin(); it != m_packageList.end(); ++it)
        qDebug() << (*it)->toString(true," - ");
}

QStringList PackageList::listPackages()
{
    QStringList list;
    QList<Package*>::iterator it;
    for (it = m_packageList.begin(); it != m_packageList.end(); ++it)
        list << (*it)->toString(true," - ");
    return list;
}

bool PackageList::writeToFile(const QString &_fileName)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif

    if (m_packageList.count() == 0)
        return false;

    QString fileName = _fileName.isEmpty() ? m_root + m_configFile : _fileName;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "could not open '"  << fileName << "' for writing (" << file.errorString() << ")";
        return false;
    }

    // this needs to be enhanced to fit current Package content
    // also maybe move this into class Package -> Package::write(QTextStream &out)
    QTextStream out(&file);
    out << "# package list" << "\n";
    out << "@format 1.0" << "\n";
    QList<Package*>::iterator it;
    for (it = m_packageList.begin(); it != m_packageList.end(); ++it)
        (*it)->write(out);
    return true;
}

bool PackageList::readFromFile(const QString &_fileName)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif

    QString fileName = _fileName.isEmpty() ? m_root + m_configFile : _fileName;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly| QIODevice::Text))
        return false;

    m_packageList.clear();
    QTextStream in(&file);

    QString comment = in.readLine();
    QString format = in.readLine();

    while (!in.atEnd())
    {
        Package pkg;
        if (pkg.read(in))
        {
            if ( pkg.isInstalled(Package::BIN) || m_curSite && !m_curSite->isExclude(pkg.name()))
                addPackage(pkg);
        }
    }
    emit configLoaded();
    return true;
}

bool PackageList::append(PackageList &src)
{
    QList<Package*>::iterator it;
    for (it = src.m_packageList.begin(); it != src.m_packageList.end(); ++it)
    {
        Package *pkg = (*it);
        addPackage(*pkg);
    }
    return true;
}


bool PackageList::syncWithFile(const QString &_fileName)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif

    QString fileName = _fileName.isEmpty() ? m_root + m_configFile : _fileName;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly| QIODevice::Text))
        return false;

    QTextStream in(&file);

    QString comment = in.readLine();
    QString format = in.readLine();

    Package *apkg;
    while (!in.atEnd())
    {
        Package pkg;
        if (pkg.read(in))
        {
            apkg = getPackage(pkg.name());
            if (apkg)
            {
                if (pkg.isInstalled(Package::BIN))
                    apkg->setInstalled(Package::BIN);
                if (pkg.isInstalled(Package::LIB))
                    apkg->setInstalled(Package::LIB);
                if (pkg.isInstalled(Package::DOC))
                    apkg->setInstalled(Package::DOC);
                if (pkg.isInstalled(Package::SRC))
                    apkg->setInstalled(Package::SRC);
            }
            else
                addPackage(pkg);
        }
    }
    emit configLoaded();
    return true;
}

bool PackageList::syncWithDatabase(Database &database)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif

    QList<Package*> newPackages;
    QList<Package*>::ConstIterator it = m_packageList.constBegin();
    for ( ; it != m_packageList.constEnd(); ++it)
    {
        Package *apkg = (*it);
        Package *pkg = database.getPackage(apkg->name(), apkg->version().toAscii());
        if (!pkg)
        {
            pkg = database.getPackage(apkg->name());
            if (!pkg || pkg->handled())
                continue;
            // if this installed package is already in the package list
            // may be it comes later in the list, don't add it twice
            Package *p = getPackage(pkg->name(),pkg->version().toAscii());
            if (p)
                continue;
            newPackages += pkg;
            pkg->setHandled(true);
            continue;
        }
        if (pkg->isInstalled(Package::BIN))
            apkg->setInstalled(Package::BIN);
        if (pkg->isInstalled(Package::LIB))
            apkg->setInstalled(Package::LIB);
        if (pkg->isInstalled(Package::DOC))
            apkg->setInstalled(Package::DOC);
        if (pkg->isInstalled(Package::SRC))
            apkg->setInstalled(Package::SRC);
        pkg->setHandled(true);
    }
    it = newPackages.constBegin();
    for ( ; it != newPackages.constEnd(); ++it)
        m_packageList += *it;

    return true;
}

inline bool isPackageFileName(const QString &fileName)
{
    return ( fileName.endsWith(".zip") || fileName.endsWith(".tbz") || fileName.endsWith(".tar.bz2") );
}

inline bool isPackageFileName(const QByteArray &fileName)
{
    return ( fileName.endsWith(".zip") || fileName.endsWith(".tbz") || fileName.endsWith(".tar.bz2") );
}

class FileType {
    public:
        FileType(const QString &v, const QString &f) { version = v; fileName = f; }
    QString version;
    QString fileName;
};

// filter List to get latest versions
QStringList filterFileName(QStringList & files)
{
    QStringList filteredFiles;

    QMap<QString,FileType*> packages;
    foreach(const QString &fileName, files)
    {
        if (isPackageFileName(fileName) )
        {
            QString pkgName;
            QString pkgVersion;
            QString pkgType;
            QString pkgFormat;
            if (!PackageInfo::fromFileName(fileName,pkgName,pkgVersion,pkgType,pkgFormat))
                continue;
            QString key = pkgName+"-"+pkgType;
            if (packages.contains(key))
            {
                qDebug() << "compare" << fileName << "with" << key << packages.value(key)->version << pkgVersion << (packages.value(key)->version < pkgVersion);
                if (packages.value(key)->version < pkgVersion)
                {
                    qDebug() << "using" << fileName << "as higher version";
                    packages[key]->version = pkgVersion;
                    packages[key]->fileName = fileName;
                }
            }
            else
            {
                qDebug() << "added" << fileName;
                packages[key] = new FileType(pkgVersion,fileName);
            }
        }
    }
    foreach(FileType *p,packages)
        filteredFiles << p->fileName;
    qDeleteAll(packages);

    return filteredFiles;
}

bool PackageList::readInternal(QIODevice *ioDev, PackageList::Type type, bool append)
{
    if (!append)
        m_packageList.clear();

    m_parserConfigFileFound = false;
    QStringList files;

    switch (type)
    {
    
    case PackageList::Ftp:
        // -rw-r--r--    1 10004    10004      385455 Feb 04 23:00 amarok-mingw-1.80.20080121-lib.tar.bz2
        while (!ioDev->atEnd())
        {
            QString line = ioDev->readLine().replace("\n","");
            QStringList parts = line.split(" ",QString::SkipEmptyParts);
            int size = parts.size();
            if (size != 9)
                continue;
            // size could be used for download estimation 
            //QString size = parts[4];
            QString file = parts[8];
            if (!isPackageFileName(file))
                continue;
            files << file;
        }
        files = filterFileName(files);
        addPackagesFromFileNames(files);
        break;

    case PackageList::SourceForge:
        while (!ioDev->atEnd())
        {
            QByteArray line = ioDev->readLine();
            if (line.contains("<td><a href=\"/project/showfiles.php?group_id="))
            {
                Package pkg;
                int a = line.indexOf("\">") + 2;
                int b = line.indexOf("</a>");
                QByteArray name = line.mid(a,b-a);
                if(m_curSite && m_curSite->isExclude(name)) {
                    ioDev->readLine();
                    continue;
                }
                if (m_curSite)
                    pkg.addDeps(m_curSite->getDependencies(name));
                pkg.setName(name);
                line = ioDev->readLine();
                a = line.indexOf("\">") + 2;
                b = line.indexOf("</a>");
                QByteArray version = line.mid(a,b-a);
                pkg.setVersion(version);
                // available types could not be determined on this web page
                // so assume all types are available
                Package::PackageItem item;
                item.set(m_baseURL,name+"-"+version+"-bin.zip",Package::BIN,false);
                pkg.add(item);
                item.set(m_baseURL,name+"-"+version+"-lib.zip",Package::LIB,false);
                pkg.add(item);
                item.set(m_baseURL,name+"-"+version+"-src.zip",Package::SRC,false);
                pkg.add(item);
                item.set(m_baseURL,name+"-"+version+"-doc.zip",Package::DOC,false);
                pkg.add(item);
                addPackage(pkg);
            }
        }
        break;

    // for example http://www.mirrorservice.org/sites/download.sourceforge.net/pub/sourceforge/k/kd/kde-cygwin/
    case PackageList::SourceForgeMirror:
    case PackageList::ApacheModIndex:
    case PackageList::Default:
        const char *startKey1 = "<a href=\"";
        const char *startKey2 = "<A HREF=\"";
        const char *endKey = "\">";
        QByteArray data = ioDev->readAll();
        int startKeyLength = strlen(startKey1);
        int endKeyLength = strlen(endKey);
        QUrl baseURL(m_baseURL);
        int a;
        int b,i;
        for (a = 0 ; a < data.size(); a = b + endKeyLength)
        {
            if ((i = data.indexOf(startKey1,a)) == -1 && (i = data.indexOf(startKey2,a)) == -1)
                break;
            a = i + startKeyLength;
            b = data.indexOf(endKey,a);
            QUrl url = QUrl(data.mid(a,b-a));
            if (!url.isValid())
                continue;
            QString path = url.path();
            if (path.isEmpty())
                continue;
            if (!isPackageFileName(path))
                continue;
            int c = path.lastIndexOf('/');
            if (c != -1)
                 files << path.mid(c+1);
            else
                 files << path;
        }
        files = filterFileName(files);
        addPackagesFromFileNames(files);
        break;
    }
    emit configLoaded();
    m_parserConfigFileFound = false;
    return true;
}

bool PackageList::addPackageFromHintFile(const QString &fileName)
{
    // if a config file is present, don't scan hint files, we assume that they
    // are merged into the config file
    if (m_parserConfigFileFound)
        return false;

    QStringList hintFile = QString(fileName).split(".");
    QString pkgName = hintFile[0];
    // download hint file
    QByteArray ba;
    if (!Downloader::instance()->start(m_baseURL.toString() + '/' + fileName, ba))
    {
        qCritical() << "could not download" << m_baseURL.toString() + '/' + fileName;
        return false;
    }
    HintFile hf;
    HintFileType hd;
    hf.parse(ba,hd);
    Package *pkg = getPackage(pkgName);
    if(!pkg) {
        Package p;
        p.setName(pkgName);
        p.setNotes(hd.shortDesc);
        p.setLongNotes(hd.longDesc);
        p.addCategories(hd.categories);
        p.addDeps(hd.requires.split(" "));
        addPackage(p);
    } else {
        pkg->setNotes(hd.shortDesc);
        pkg->setLongNotes(hd.longDesc);
        pkg->addCategories(hd.categories);
        pkg->addDeps(hd.requires.split(" "));
    }
    return true;
}

bool PackageList::addPackagesFromFileNames(const QStringList &files, bool ignoreConfigTxt)
{
    if (!ignoreConfigTxt && files.contains("config.txt"))
    {
        // fetch config file
        QFileInfo cfi(Settings::instance().downloadDir()+"/config-temp.txt");
        bool ret = Downloader::instance()->start(m_baseURL.toString() + "/config.txt",cfi.absoluteFilePath());
        if (ret)
        {
            GlobalConfig g;
            g.setBaseURL(m_baseURL);
            QStringList configFile = QStringList() << cfi.absoluteFilePath();
            if (!g.parse(configFile))
                return false;

            // add package from globalconfig
            QList<Package*>::iterator p;
            for (p = g.packages()->begin(); p != g.packages()->end(); p++)
            {
                Package *pkg = *p;
                addPackage(*pkg);
            }
            // sites are not supported here
            m_parserConfigFileFound = true;
            // config.txt overrides all other definitions
            return true;
        }
    }

    foreach(const QString &fileName, files)
    {
#ifdef ENABLE_HINTFILE_SUPPORT
        // @TODO using hint files results into duplicated package entries
        if (fileName.endsWith(".hint"))
        {
            addPackageFromHintFile(fileName);
        } else
#endif
        if (fileName.endsWith(".zip") || fileName.endsWith(".tbz") || fileName.endsWith(".tar.bz2") ) {
            QString pkgName;
            QString pkgVersion;
            QString pkgType;
            QString pkgFormat;
            if (!PackageInfo::fromFileName(fileName,pkgName,pkgVersion,pkgType,pkgFormat))
                continue;
            Package *pkg = getPackage(pkgName, pkgVersion.toAscii());
            if(!pkg) {
                Package p;
                p.setVersion(pkgVersion);
                p.setName(pkgName);
                Package::PackageItem item;
                item.set(m_baseURL.toString() + '/' + fileName, "", pkgType.toAscii());
                p.add(item);
                if(m_curSite)
                    p.setNotes(m_curSite->packageNote(pkgName));
                if(m_curSite)
                    p.setLongNotes(m_curSite->packageLongNotes(pkgName));
                if (m_curSite)
                {
#ifdef VERSIONED_DEPENDENCIES
                    p.addDeps(m_curSite->getDependencies(pkgName+"-"+pkgVersion));
#endif
                    p.addDeps(m_curSite->getDependencies(pkgName));
                }
                addPackage(p);
            } else {
                Package::PackageItem item;
                item.set(m_baseURL.toString() + '/' + fileName, "", pkgType.toAscii());
                pkg->add(item);
                if (m_curSite)
                {
#ifdef VERSIONED_DEPENDENCIES
                    pkg->addDeps(m_curSite->getDependencies(pkgName+"-"+pkgVersion));
#endif
                    pkg->addDeps(m_curSite->getDependencies(pkgName));
                }
            }
        }
        else {
            qDebug() << __FUNCTION__ << "unsupported package format" << fileName;
        }
    }
    return true;
}

bool PackageList::readFromByteArray(const QByteArray &_ba, PackageList::Type type, bool append)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif

    QByteArray ba(_ba);
    QBuffer buf(&ba);

    if (!buf.open(QIODevice::ReadOnly| QIODevice::Text))
        return false;

    return readInternal(&buf, type, append);
}

bool PackageList::readFromFile(const QString &fileName, PackageList::Type type, bool append)
{
#ifdef DEBUG
    qDebug() << __FUNCTION__;
#endif

    QFile pkglist(fileName);
    if (!pkglist.exists())
        return false;

    pkglist.open(QIODevice::ReadOnly);

    return readInternal(&pkglist, type, append);
}

bool PackageList::setInstalledPackage(const Package &apkg)
{
    Package *pkg = getPackage(apkg.name());
    if (!pkg)
    {
        qDebug() << __FUNCTION__ << "package " << apkg.name() << " not found";
        return false;
    }
    pkg->setInstalled(apkg);
    return true;
}

void PackageList::clear()
{
    m_packageList.clear();
    m_curSite = 0;
}

QDebug & operator<<(QDebug &out, PackageList &c)
{
    out << "PackageList( "
        << "m_name:" << c.m_name
        << "m_root:" << c.m_root
        << "m_configFile:" << c.m_configFile
        << "m_baseURL:" << c.m_baseURL
        << "packages: (size:" << c.m_packageList.size();
    QList<Package*>::ConstIterator it = c.m_packageList.constBegin();
    for ( ; it != c.m_packageList.constEnd(); ++it)
        out << *(*it);
    out << ") )";
    return out;
}

#include "packagelist.moc"
