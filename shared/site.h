/****************************************************************************
**
** Copyright (C) 2006 Ralf Habacker. All rights reserved.
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

#ifndef SITE_H
#define SITE_H

#include <QString>

/**
	holds a site definition 
*/ 
class Site {

	public:
		QString Name() {return m_name;}
		QString URL()  {return m_url;}
		QString type() {return m_type;}
		void setName(const QString &name) {m_name = name;}
		void setURL(const QString &url)  {m_url = url; }
		void setType(const QString &type) {m_type = type; }
	private:
		QString m_name;
		QString m_url;
		QString m_type;
};

#endif 
