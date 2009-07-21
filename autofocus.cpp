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

#include "autofocus.h"

#include <stdlib.h>

GAutoFocus::GAutoFocus()
{
	Noise = 0.0;
	stop = false;
	focus_step = 8;
	NextFocus = focus_step;
	change_count = 0;
	NoiseCounts = 10;		// must calibrate
}

GAutoFocus::~GAutoFocus()
{
	int infs = finfos.size();
	for (int i = 0; i < infs; i++)
	{
		delete finfos[i];
	}
	finfos.clear();
}

int GAutoFocus::lastDispersion()
{
	int last_index = (int)finfos.size() - 1;
	if (last_index < 0)
		return 0.0;
	return finfos[last_index]->dispersion;
}

int GAutoFocus::lastPosition()
{
	int last_index = (int)finfos.size() - 1;
	if (last_index < 0)
		return 0.0;
	return finfos[last_index]->focusPosition;
}

void GAutoFocus::NextIter(int **image_arr, int w, int h)
{
	if (stop)
		return;

	int last_index = (int)finfos.size() - 1;
	focusingInfo* finf = new focusingInfo;
	int** sobel_image = sobel_trans(image_arr, w, h);
	finf->dispersion = dispersion(sobel_image, w, h);	// this value of dispersion is a result of previous NextFocus
	delete_image(sobel_image, w, h);
	if (last_index >= NoiseCounts)
	{
		finf->focusPosition = finfos[last_index]->focusPosition + NextFocus;
		finf->focusDir = NextFocus;
	}
	else
	{
		finf->focusPosition = 0;
		finf->focusDir = 0;
	}
	finfos.push_back(finf);
	last_index++;

	if (last_index == NoiseCounts)
	{
		// калибровка шумов - прогрев автофокуса )))
		Noise = (maxdispersion() - mindispersion())/2;
	}

	if (last_index > NoiseCounts)
	{
		if (abs(finf->dispersion - finfos[last_index - 1]->dispersion) < Noise)
			finf->dispersion = finfos[last_index - 1]->dispersion;
		if (finfos.size() > 5 + NoiseCounts)
			if (abs((finf->dispersion - finfos[last_index - 4]->dispersion)) <= Noise)
				stop = true;

		int max_disp = maxdispersion();
		//int min_disp = mindispersion();
		if (finf->dispersion > max_disp && finfos.size() > 50 + NoiseCounts)
			stop = true;

		if (finf->focusDir != finfos[last_index - 1]->focusDir && abs(finfos[last_index - 1]->focusDir) > 0)
			change_count++;
		if (change_count > 1)
			focus_step = 1;
		if (change_count > 3)
			stop = true;

		if (finf->dispersion >= finfos[last_index - 1]->dispersion ||
			abs(finf->dispersion - max_disp) < 3*Noise)
		{
			NextFocus = finf->focusDir > 0 ? focus_step : -focus_step;
		}
		else
		{
			NextFocus = finf->focusDir > 0 ? -focus_step : focus_step;
		}
	}
	if (stop)
		NextFocus = 0;
}

int GAutoFocus::maxdispersion()
{
	int ret = finfos[0]->dispersion;
	int infs = finfos.size();
	for(int i = 0; i < infs; i++)
	{
		if (ret < finfos[i]->dispersion)
			ret = finfos[i]->dispersion;
	}
	return ret;
}

int GAutoFocus::mindispersion()
{
	int ret = finfos[0]->dispersion;
	int infs = finfos.size();
	for(int i = 0; i < infs; i++)
	{
		if (ret > finfos[i]->dispersion)
			ret = finfos[i]->dispersion;
	}
	return ret;
}

int GAutoFocus::delete_image(int** array, int /*w*/, int h)
{
	int i;
	for (i = 0; i < h; i++)
		free(array[i]);
	free(array);
	return 0;
}

int** GAutoFocus::sobel_trans(int** src_image, int w, int h)
{
	static const int Gx[3][3] =
		{
			{-1, 0, 1},
			{-2, 0, 2},
			{-1, 0, 1}
		};
	static const int Gy[3][3] =
		{
			{1, 2, 1},
			{0, 0, 0},
			{-1, -2, -1}
		};
	int x, y;
	// allocate destination array
	int** dst_vals = (int**)malloc(h*sizeof(int*));
	if (!dst_vals)
	{
		return 0;
	}
	for (y = 0; y < h; y++)
	{
		dst_vals[y] = (int*)malloc(w*sizeof(int));
	}
	int i, j;
	int sum_x, sum_y;
	int sum;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			if (x == 0 || x == w - 1 || y == 0 || y == h - 1)
				sum = 0;
			else
			{
				sum_x = 0;
				sum_y = 0;
				for (i = 0; i < 3; i++)
				{
					for (j = 0; j < 3; j++)
					{
						sum_x += src_image[y + i - 1][x + j - 1]*Gx[i][j];
						sum_y += src_image[y + i - 1][x + j - 1]*Gy[i][j];
					}
				}
				sum = abs(sum_x) + abs(sum_y);
				if (sum > 255)
					sum = 255;
				else if (sum < 0)
					sum = 0;
			}
			dst_vals[y][x] = 255 - sum;
		}
	}
	return dst_vals;
}

int GAutoFocus::dispersion(int** array, int w, int h)
{
	int i, j;
	int res = 0;
	int avg = average(array, w, h);
	for (i = 1; i < h - 1; i++)
	{
		for (j = 1; j < w - 1; j++)
			res += (array[i][j] - avg)*(array[i][j] - avg);
	}
	res = res/((w - 2)*(h - 2));
	return res;
}

int GAutoFocus::average(int** array, int w, int h)
{
	int i, j;
	int res = 0;
	for (i = 1; i < h - 1; i++)
	{
		for (j = 1; j < w - 1; j++)
			res += array[i][j];
	}
	res = res/((w - 2)*(h - 2));
	return res;
}
