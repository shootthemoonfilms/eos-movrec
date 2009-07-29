/***************************************************************************
 *   Copyright (C) 2008-2009 by Чернов А.А.                                *
 *   valexlin@gmail.com                                                    *
 *   --                                                                    *
 *   Copyright (C) 2009 by Uterr                                           *
 *     Old AF method                                                       *
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

#include <QList>

class GAutoFocus
{
private:
	int NextFocus;
	int Noise;
	int focus_step;
	int change_count;
	int NoiseCounts;
	struct focusingInfo
	{
		int dispersion;
		int focusPosition;
		int focusDir;
	};
	QList <GAutoFocus::focusingInfo> finfos;
public:
	bool stop;
	GAutoFocus();
	~GAutoFocus();
	int lastDispersion();
	int lastPosition();
	int noise()
	{
		return Noise;
	}
	void NextIter(int **image_arr, int w, int h, int* cookie);
	int getNextFocus()
	{
		if ((int)finfos.size() <= NoiseCounts)
			return 0;
		return NextFocus;
	}
private:
	int abs(int i)
	{
		return i > 0 ? i : -i;
	}
	int maxdispersion(int* focus_pos = 0);
	int mindispersion();

	int** sobel_filter(int** src_image, int w, int h);
	int** gauss_filter(int** src_image, int w, int h);
	int dispersion(int** array, int w, int h, int n);
	int average(int** array, int w, int h, int n);
	//QImage array_to_image(int** array, int w, int h);
};
