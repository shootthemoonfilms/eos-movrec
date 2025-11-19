/***************************************************************************
 *   Copyright (C) 2015 by insaner                                         *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _pathsdlg_h
#define _pathsdlg_h

#include <QDialog>
#include <QWidget>

#include <qstring.h>

class QString;
class QLabel;
class QToolButton;


class GPathsDlg: public QDialog
{
	Q_OBJECT
 //   Q_INTERFACES(QGraphicsItem)
public:
	// explicit GPathsDlg(QWidget* parent);
	GPathsDlg(QWidget* parent);
	~GPathsDlg();
	void setFilePaths(QString newVidName, QString newImgName);
 	QString getVidName() {return VidName;};
 	QString getImgName() {return ImgName;};
private slots:
	void slotSelVidName();
	void slotSelImgName();
	
private:
	QString VidName;
	QString ImgName;
	QToolButton* VidNameBtn;
	QToolButton* ImgNameBtn;
	QLabel* VidNameLabel;
	QLabel* ImgNameLabel;
	
};

#endif // _pathsdlg_h
