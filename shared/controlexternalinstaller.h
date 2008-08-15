/****************************************************************************
**
** Copyright (C) 2006-2007 Ralf Habacker. All rights reserved.
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
#ifndef CONTROLEXTERNALINSTALLER_H
#define CONTROLEXTERNALINSTALLER_H

#include <QtDebug>
#include <windows.h>
class QProcess;
class ControlExternalInstallerPrivate;

/**    
   \brief The ControlExternalInstaller class provides support for controlling external installers by selecting buttons and set text in edit fields
   */
class ControlExternalInstaller {
    public:
        ControlExternalInstaller();
        ~ControlExternalInstaller();
        bool connect(int pid);
        bool connect(HANDLE handle);
        bool connect(const QProcess &proc);
        bool disconnect();
        
        bool pressButtonWithText(const QString &text);

        friend QDebug &operator<<(QDebug &,const ControlExternalInstaller &);
    protected:
        bool updateWindowItems();

    private:
        ControlExternalInstallerPrivate *d;
        DWORD m_processId;
};

#endif