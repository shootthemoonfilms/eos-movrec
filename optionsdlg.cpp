/***************************************************************************
 *   Copyright (C) 2008-2009 by Чернов А.А.                                *
 *   valexlin@gmail.com                                                    *
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

#include "optionsdlg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>

GOptionsDlg::GOptionsDlg(QWidget* parent)
 : QDialog(parent)
{
	setModal(true);
	setSizeGripEnabled(false);
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSizeConstraint(QLayout::SetFixedSize);
	QHBoxLayout* layout_line1 = new QHBoxLayout();
	layout_line1->addWidget(new QLabel(tr("Buffer size (for writing)")), 0);
	BufferBox = new QSpinBox(this);
	BufferBox->setSuffix(tr("MB"));
	BufferBox->setRange(1, 512);
	BufferBox->setValue(2);
	layout_line1->addWidget(BufferBox, 0);
	layout->addLayout(layout_line1, 0);

	QHBoxLayout* layout_line2 = new QHBoxLayout();
	layout_line2->addWidget(new QLabel(tr("Camera AF mode (only on 50D and later!)")), 0);
	AFModeBox = new QComboBox(this);
	AFModeBox->addItem(tr("Quick"));
	AFModeBox->addItem(tr("Live"));
	AFModeBox->addItem(tr("Live Face"));
	AFModeBox->setCurrentIndex(1);
	layout_line2->addWidget(AFModeBox, 0);
	layout->addLayout(layout_line2, 0);

	StabFPSBox = new QCheckBox(tr("Use stabilized FPS"), this);
	layout->addWidget(StabFPSBox, 0);

	ShowWhiteBox = new QCheckBox(tr("Show zoom white box"), this);
	layout->addWidget(ShowWhiteBox, 0);

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

	connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
}

void GOptionsDlg::setOptions(int buff_sz, int afmode, bool s, bool w)
{
	BufferBox->setValue(buff_sz/1048576);
	AFModeBox->setCurrentIndex(afmode);
	StabFPSBox->setChecked(s);
	ShowWhiteBox->setChecked(w);
}

int GOptionsDlg::bufferSize()
{
	return 1048576*BufferBox->value();
}

int GOptionsDlg::afMode()
{
	return AFModeBox->currentIndex();
}

bool GOptionsDlg::useStabFPS()
{
	return StabFPSBox->isChecked();
}

bool GOptionsDlg::showWhiteBox()
{
	return ShowWhiteBox->isChecked();
}
