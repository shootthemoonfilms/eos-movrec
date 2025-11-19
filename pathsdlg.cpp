/***************************************************************************
 *   Copyright (C) 2015, 2021 by insaner                                   *
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

#include "debug.h"

#include "pathsdlg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QToolButton>
#include <QFileDialog>



GPathsDlg::GPathsDlg(QWidget* parent)
 : QDialog(parent)
{
	setModal(true);
	setSizeGripEnabled(false);
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSizeConstraint(QLayout::SetFixedSize);
	
	
	QHBoxLayout* layout_VidName = new QHBoxLayout();
	layout_VidName->addWidget(new QLabel(tr("Video Filename:")), 0);
	VidNameLabel = new QLabel(VidName);
	VidNameLabel->setStyleSheet("font-weight: bold");
	//VidNameLabel->setStyleSheet("color: #555");
	layout_VidName->addWidget(VidNameLabel, 1);
	
	VidNameBtn = new QToolButton(this);
	VidNameBtn->setText(tr("..."));
	VidNameBtn->setEnabled(true);
	VidNameBtn->setToolTip(tr("select Vid name"));
	layout_VidName->addWidget(VidNameBtn, 0);
	layout->addLayout(layout_VidName, 0);
	
	
	QHBoxLayout* layout_ImgName = new QHBoxLayout();
	layout_ImgName->addWidget(new QLabel(tr("Image Filename:")), 0);
	ImgNameLabel = new QLabel(ImgName);
	ImgNameLabel->setStyleSheet("font-weight: bold");
	layout_ImgName->addWidget(ImgNameLabel, 1);
	
	ImgNameBtn = new QToolButton(this);
	ImgNameBtn->setText(tr("..."));
	ImgNameBtn->setEnabled(true);
	ImgNameBtn->setToolTip(tr("select Img name"));
	layout_ImgName->addWidget(ImgNameBtn, 0);
	layout->addLayout(layout_ImgName, 0);
	
	
	QFrame* line = new QFrame(this);
	line->setFrameShape(QFrame::HLine); // Horizontal line
	line->setFrameShadow(QFrame::Sunken);
	line->setLineWidth(1);

	QHBoxLayout* layout_line = new QHBoxLayout();
	layout_line->addWidget(line);
	layout->addLayout(layout_line, 0);
	

	QHBoxLayout* layout_FName_inc = new QHBoxLayout();
	layout_FName_inc->addWidget(new QLabel(tr("File names automatically increment numerically (appended with _000, _001... before the extension).")), 0);
	layout->addLayout(layout_FName_inc, 0);
	
	
	QHBoxLayout* layout_btn = new QHBoxLayout();
	layout_btn->addStretch(10);
	QPushButton* okBtn = new QPushButton(this);
	okBtn->setText(tr("&OK"));
	okBtn->setDefault(true);
	okBtn->setAutoDefault(true);
	layout_btn->addWidget(okBtn, 0);
	layout_btn->addSpacing(10);
	QPushButton* cancelBtn = new QPushButton(this);
	cancelBtn->setText(tr("&Cancel"));
	layout_btn->addWidget(cancelBtn, 0);
	layout_btn->addStretch(10);
	layout->addLayout(layout_btn, 0);
	
	connect(ImgNameBtn, SIGNAL(clicked()), this, SLOT(slotSelImgName()));
	connect(VidNameBtn, SIGNAL(clicked()), this, SLOT(slotSelVidName()));

	connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
}


void GPathsDlg::setFilePaths(QString newVidName, QString newImgName)
{
	if (DEBUG) fprintf(stderr, "GPathsDlg::setFilePaths: [%s, %s]\n",
		newVidName.toLocal8Bit().constData(),
		newImgName.toLocal8Bit().constData() );
	VidName = newVidName;
	ImgName = newImgName;
	VidNameLabel->setText(VidName);
	ImgNameLabel->setText(ImgName);
}
	
void GPathsDlg::slotSelImgName()
{
	QString dir = QFileDialog::getSaveFileName(this, tr("Image file output name"), ImgName,
		// tr("JPG Image  .jpg (*.jpg)"), 0);
		tr("JPG Image  .jpg (*.jpg)"), 0, QFileDialog::DontConfirmOverwrite);
	if (!dir.isNull() && !dir.isEmpty())
	{
		ImgName = dir;
		ImgNameLabel->setText(ImgName);
	}
}

// QString QFileDialog::getSaveFileName(QWidget * parent = 0, const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString(), QString * selectedFilter = 0, Options options = 0)
void GPathsDlg::slotSelVidName()
{
	QString dir = QFileDialog::getSaveFileName(this, tr("Video file output name"), VidName,
		// tr("Videos  .avi .mpg .mp4 (*.avi *.mpg *.mp4)"), 0);
		tr("Videos  .avi .mpg .mp4 (*.avi *.mpg *.mp4)"), 0, QFileDialog::DontConfirmOverwrite);
	// 	QFileDialog::ShowDirsOnly);
	if (!dir.isNull() && !dir.isEmpty())
	{
		VidName = dir;
		VidNameLabel->setText(VidName);
	}
}


GPathsDlg::~GPathsDlg()
{
}


