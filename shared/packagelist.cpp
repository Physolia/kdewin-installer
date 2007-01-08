/****************************************************************************
**
** Copyright (C) 2005-2006 Ralf Habacker. All rights reserved.
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

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QUrl>
#include <QTreeWidget>

#include "packagelist.h"
#include "downloader.h"
#include "installer.h"

QStringList filterPackageFiles(const QStringList &list,const QString &mode)
{
    QStringList result; 
    for (int j = 0; j < list.size(); ++j) {
        QUrl url(list.at(j));
        QFileInfo fileInfo(url.path());
        QString fileName = fileInfo.fileName();

        // only download package not already downloaded and only bin and lib packages
	        if (mode == "URL" && QFile::exists(fileName))
            qDebug() << fileName << " - already downloaded";
        //		else if(fileName.contains("src") ) 
        //	    qDebug() << fileName << " - ignored";
        else {
        if (mode == "URL")
            qDebug() << fileName << " - downloading";
            else
            qDebug() << fileName << " - installing";
        if (mode == "URL")
            result << list.at(j);
        else
            result << fileName;
        }
    }
    return result;
}

PackageList::PackageList(Downloader *_downloader)
	: QObject()
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
    downloader = _downloader ? _downloader : new Downloader;
	m_packageList = new QList<Package>;
	root = ".";
	m_configFile = "/packages.txt";
	
}
		
PackageList::~PackageList()
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	delete m_packageList;
}

bool PackageList::hasConfig()
{
	return QFile::exists(root + m_configFile);
}

void PackageList::addPackage(Package const &package)
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	m_packageList->append(package);
}

Package *PackageList::getPackage(QString const &pkgName)
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	QList<Package>::iterator i;
	for (i = m_packageList->begin(); i != m_packageList->end(); ++i)
		if (i->Name() == pkgName) 
			return &*i;
	return 0;
}

void PackageList::listPackages(const QString &title) 
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	qDebug() << title;
	QList<Package>::iterator i;
	for (i = m_packageList->begin(); i != m_packageList->end(); ++i)
		qDebug(i->toString(true," - ").toLatin1());
}

bool PackageList::writeToFile(const QString &_fileName)
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	if (m_packageList->count() == 0)
		return false;

	QString fileName = _fileName.isEmpty() ? root + m_configFile : _fileName;
	QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug() << "could not open '"  << fileName << "' for writing";
		return false;
	}

	QTextStream out(&file);
	out << "# package list" << "\n";
	QList<Package>::iterator i;
	for (i = m_packageList->begin(); i != m_packageList->end(); ++i)
		out << i->Name() << "\t" << i->Version() << "\n";
	return true;
}

bool PackageList::readFromFile(const QString &_fileName)
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	QString fileName = _fileName.isEmpty() ? root + m_configFile : _fileName;
	QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly| QIODevice::Text))
      return false;

  m_packageList->clear();

	Package pkg;
  while (!file.atEnd()) {
	  QByteArray line = file.readLine();
		if (line.startsWith("#"))
			continue;
    int i = line.lastIndexOf('\t');
    pkg.setName(line.mid(0,i));
    pkg.setVersion(line.mid(i+1,line.size()-i-2));
		addPackage(pkg);
	}
	emit loadedConfig();
	return true;
}

bool PackageList::readHTMLInternal(QIODevice *ioDev, Site::SiteType type)
{
  m_packageList->clear();
	
	Package pkg; 

	switch (type) {
		case Site::SourceForge: 
			while (!ioDev->atEnd()) {
				QByteArray line = ioDev->readLine();
				if (line.contains("<td><a href=\"/project/showfiles.php?group_id=23617")) {
					int a = line.indexOf("\">") + 2;
					int b = line.indexOf("</a>");
					QByteArray value = line.mid(a,b-a);
					if (line.indexOf("release_id") > -1) {
						pkg.setVersion(value);
						addPackage(pkg);
					}
				else
					pkg.setName(value);
				}
			}
			break; 

		case Site::ApacheModIndex: 
			char *lineKey = "alt=\"[   ]\"> <a href=\"";
			char *fileKeyStart = "<a href=\"";
			char *fileKeyEnd = "\">";
			while (!ioDev->atEnd()) {
				QByteArray line = ioDev->readLine();
#ifdef DEBUG
				qDebug() << "2"  << line << " " << lineKey; 
#endif
				if (line.contains(lineKey)) {
					int a = line.indexOf(fileKeyStart) + strlen(fileKeyStart);
					int b = line.indexOf(fileKeyEnd,a);
					QByteArray name = line.mid(a,b-a);
#ifdef DEBUG
					qDebug() << "3"  << name;
#endif
					// desktop-translations-10.1-41.3.noarch.rpm
					// kde3-i18n-vi-3.5.5-67.9.noarch.rpm
                    // aspell-0.50.3-3.zip 

                    // first remove ending
                    int idx = name.lastIndexOf('.');
                    if(idx != -1)
                        name = name.left(idx);

                    // now split a little bit :)
					QList<QByteArray> parts;
					QList<QByteArray> tempparts = name.split('-');
                    // and once more on '.'
                    for (int i = 0; i < tempparts.size(); ++i) {
                        parts << tempparts[i].split('.');
                        parts << "-";
                    }
                    // not a correct named file
                    if(parts.size() <= 1)
                        continue;

                    parts.removeLast();

#ifdef _DEBUG
     				qDebug() << parts.size();
					for (int i = 0; i < parts.size(); ++i) 
     					qDebug() << parts.at(i);
#endif

                    int iVersionLow = 0, iVersionHigh = parts.size() - 1;
                    for(; iVersionHigh > 0; iVersionHigh --) {
                        if(parts[iVersionHigh][0] >= '0' && parts[iVersionHigh][0] <= '9')
                            break;
                    }
                    if(iVersionHigh == 0)
                        continue;
                    iVersionHigh++;
                    for(; iVersionLow < iVersionHigh; iVersionLow++) {
                        bool ok;
                        parts[iVersionLow].toInt(&ok);
                        if(ok)
                            break;
                    }
                    if(iVersionLow == iVersionHigh)
                        continue;

                    QByteArray version, patchlevel;
                    name = "";
                    for(int i = 0; i < iVersionLow; i++) {
                        if(parts[i] == "-")
                            break;
                        if(name.size())
                            name += '.';
                        name += parts[i];
                    }
                    for(; iVersionLow < iVersionHigh; iVersionLow++) {
                        if(parts[iVersionLow] == "-")
                            break;
                        if(version.size())
                            version += '.';
                        version += parts[iVersionLow];
                    }
                    iVersionLow++;
                    for(; iVersionLow < iVersionHigh; iVersionLow++) {
                        if(parts[iVersionLow] == "-")
                            break;
                        if(patchlevel.size())
                            patchlevel += '.';
                        patchlevel += parts[iVersionLow];
                    }

					if(!patchlevel.isEmpty())
                        version += "-" + patchlevel;
                    pkg.setVersion(version);
					pkg.setName(name);
					addPackage(pkg);
				}
			}
			break; 
	}
	emit loadedConfig();
	return true;
}

bool PackageList::readHTMLFromByteArray(const QByteArray &_ba, Site::SiteType type)
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif

  QByteArray ba(_ba);
  QBuffer buf(&ba);

  if (!buf.open(QIODevice::ReadOnly| QIODevice::Text))
      return false;

  return readHTMLInternal(&buf, type);
}

bool PackageList::readHTMLFromFile(const QString &fileName, Site::SiteType type )
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	QFile pkglist(fileName);
	if (!pkglist.exists())
  	return false;

	pkglist.open(QIODevice::ReadOnly);

  return readHTMLInternal(&pkglist, type);
}

QStringList PackageList::getFilesForInstall(QString const &pkgName)
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	QStringList result;
	Package *pkg = getPackage(pkgName);
	if (!pkg) 
		return result;
	result << pkg->getFileName(Package::BIN);
	result << pkg->getFileName(Package::LIB);
#ifdef INCLUDE_DOC_AND_SRC_PACKAGES
	result << pkg->getFileName(Package::DOC);
	result << pkg->getFileName(Package::SRC);
#else
	qDebug("downloading of DOC and SRC disabled for now");
#endif
	return result;
}

QStringList PackageList::getFilesForDownload(QString const &pkgName)
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	QStringList result;
	Package *pkg = getPackage(pkgName);
	if (!pkg)
		return result;
	result << pkg->getURL(Package::BIN);
	result << pkg->getURL(Package::LIB);
#ifdef INCLUDE_DOC_AND_SRC_PACKAGES
	result << pkg->getURL(Package::DOC);
	result << pkg->getURL(Package::SRC);
#else
	qDebug("downloading of DOC and SRC disabled for now");
#endif
	return result;
}

bool PackageList::updatePackage(Package &apkg)
{
	Package *pkg = getPackage(apkg.Name());
	if (!pkg) {
		qDebug() << __FUNCTION__ << "package " << apkg.Name() << " not found";
		return false;
	}
	pkg->addInstalledTypes(apkg);
	return true;
}

bool PackageList::downloadPackage(const QString &pkgName)
{
	QStringList files = getFilesForDownload(pkgName);
	files = filterPackageFiles(files,"URL");
	bool ret = true;
	for (int j = 0; j < files.size(); ++j) {
		if (!downloader->start(files.at(j)))
			ret = false;
	}
	return true;
}

bool PackageList::installPackage(const QString &pkgName)
{
	QStringList files = getFilesForInstall(pkgName);
	files = filterPackageFiles(files,"PATH");
	bool ret = true;
	for (int j = 0; j < files.size(); ++j) {
		if (!installer->install(files.at(j)))
			ret = false;
	}
	return true;
}

int PackageList::size()
{
#ifdef DEBUG
	qDebug() << __FUNCTION__;
#endif
	return m_packageList->size();
}

#include "packagelist.moc"
