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
#include <math.h>

#if AF_DEBUG_LOG
#include <QImage>
#include <stdio.h>

static QImage array_to_image(int** array, int w, int h);
#endif

GAutoFocus::GAutoFocus()
{
	Noise = 0.0;
	stop = false;
	focus_step = 8;
	NextFocus = focus_step;
	change_count = 0;
	NoiseCounts = 4;		// must calibrate
}

GAutoFocus::~GAutoFocus()
{
	/*int infs = finfos.count();
	for (int i = 0; i < infs; i++)
	{
		delete finfos[i];
	}
	finfos.clear();*/
}

int GAutoFocus::lastDispersion()
{
	int last_index = finfos.count() - 1;
	if (last_index < 0)
		return 0.0;
	return finfos[last_index].dispersion;
}

int GAutoFocus::lastPosition()
{
	int last_index = finfos.count() - 1;
	if (last_index < 0)
		return 0.0;
	return finfos[last_index].focusPosition;
}

void GAutoFocus::NextIter(int **image_arr, int w, int h, int* cookie)
{
	if (stop)
		return;

	int i;
	int last_index = finfos.count() - 1;
	focusingInfo finf;
	int** gauss_image = gauss_filter(image_arr, w, h);
	int** sobel_image = sobel_filter(gauss_image, w, h);
#if AF_DEBUG_LOG
	QString name1 = QString("afimg_%1.png").arg(*cookie, 3, 10, QLatin1Char('0'));
	QString name2 = QString("afimg_%1_gauss.png").arg(*cookie, 3, 10, QLatin1Char('0'));
	QString name3 = QString("afimg_%1_g+s.png").arg(*cookie, 3, 10, QLatin1Char('0'));
	char name4[128];
	sprintf(name4, "afimg_%03d.txt", *cookie);
	array_to_image(image_arr, w, h).save(name1);
	array_to_image(gauss_image, w, h).save(name2);
	array_to_image(sobel_image, w, h).save(name3);
	FILE* info = fopen(name4, "wt");
#endif
	finf.dispersion = dispersion(sobel_image, w, h, 2);	// this value of dispersion is a result of previous NextFocus
	// delete gauss image
	for (i = 0; i < h; i++)
		free(gauss_image[i]);
	free(gauss_image);
	// delete sobel image
	for (i = 0; i < h; i++)
		free(sobel_image[i]);
	free(sobel_image);
	if (last_index >= NoiseCounts)
	{
		finf.focusPosition = finfos[last_index].focusPosition + NextFocus;
		finf.focusDir = NextFocus;
	}
	else
	{
		finf.focusPosition = 0;
		finf.focusDir = 0;
	}
	finfos.append(finf);
	last_index++;

	if (last_index == NoiseCounts - 1)
	{
		// калибровка шумов - прогрев автофокуса )))
		//Noise = (maxdispersion() - mindispersion())/2;
		int avg = 0;
		int s = 0;
		for (i = 0; i < NoiseCounts; i++)
			avg += finfos[i].dispersion;
		avg /= NoiseCounts;
		for (i = 0; i < NoiseCounts; i++)
			s += (finfos[i].dispersion - avg)*
				 (finfos[i].dispersion - avg);
		Noise = (int)sqrt((double)s/(double)NoiseCounts);
		Noise *= 6;		// правило трех сигм
		if (Noise == 0)
			Noise = 3;
	}

	if (last_index >= NoiseCounts)
	{
		//if (abs(finf->dispersion - finfos[last_index - 1].dispersion) < Noise)
		//	finf->dispersion = finfos[last_index - 1].dispersion;
		// если дисперсия дисперсий почти не меняется -> остановиться
		if (finfos.count() > 10 + NoiseCounts)
		{
			int avg = 0;
			int e = 0;
			const int n = 6;
			for (i = 0; i < n; i++)
				avg += finfos[last_index - i].dispersion;
			avg /= n;
			for (i = 0; i < n; i++)
				e += (finfos[last_index - i].dispersion - avg)*
					 (finfos[last_index - i].dispersion - avg);
			e = (int)sqrt((double)e/(double)n);
			if (8*e < Noise)
			{
#if AF_DEBUG_LOG
				fprintf(info, "AF stoped: e <= Noise(%d): e = %d\n", Noise, e*8);
				fflush(info);
#endif
				stop = true;
			}
		}

		int max_disp_pos = 0;
		int max_disp = maxdispersion(&max_disp_pos);
		//int min_disp = mindispersion();
		if (finf.dispersion > max_disp && finfos.count() > 50 + NoiseCounts)
		{
#if AF_DEBUG_LOG
				fprintf(info, "AF stoped: to many iterations\n");
				fflush(info);
#endif
			stop = true;
		}

		if (finf.focusDir != finfos[last_index - 1].focusDir && abs(finfos[last_index - 1].focusDir) > 0)
			change_count++;
		if (change_count > 2)
			focus_step = 1;
		if (change_count > 4)
		{
#if AF_DEBUG_LOG
				fprintf(info, "AF stoped: change_count > 4\n");
				fflush(info);
#endif
			stop = true;
		}

#if AF_DEBUG_LOG
				fprintf(info, "prev disp = %d\n", finfos[last_index - 1].dispersion);
				fprintf(info, "max_disp = %d\n", max_disp);
				fflush(info);
#endif
		if (stop)
		{
			NextFocus = max_disp_pos - finf.focusPosition;
		}
		else
		{
			if (finf.dispersion >= finfos[last_index - 1].dispersion ||
				abs(finf.dispersion - finfos[last_index - 1].dispersion)*2/3 <= Noise)
			{
				NextFocus = finf.focusDir > 0 ? focus_step : -focus_step;
			}
			else
			{
				NextFocus = finf.focusDir > 0 ? -focus_step : focus_step;
			}
		}
	}
#if AF_DEBUG_LOG
	fprintf(info, "i = %d\ndisp = %d, NextFocus = %d\n", *cookie, finf.dispersion, NextFocus);
	if (info)
		fclose(info);
#endif
}

int GAutoFocus::maxdispersion(int* focus_pos)
{
	int infs = finfos.count();
	if (infs < 1)
		return 0;
	int ret = finfos[0].dispersion;
	for(int i = 0; i < infs; i++)
	{
		if (ret < finfos[i].dispersion)
		{
			ret = finfos[i].dispersion;
			if (focus_pos)
				*focus_pos = finfos[i].focusPosition;
		}
	}
	return ret;
}

int GAutoFocus::mindispersion()
{
	int infs = finfos.count();
	if (infs < 1)
		return 0;
	int ret = finfos[0].dispersion;
	for(int i = 0; i < infs; i++)
	{
		if (ret > finfos[i].dispersion)
			ret = finfos[i].dispersion;
	}
	return ret;
}

int** GAutoFocus::sobel_filter(int** src_image, int w, int h)
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
	for (y = 1; y < h - 1; y++)
	{
		for (x = 1; x < w - 1; x++)
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
			dst_vals[y][x] = 255 - sum;
		}
	}
	for (i = 0; i < w; i++)
	{
		dst_vals[0][i] = dst_vals[1][i];
		dst_vals[h - 1][i] = dst_vals[h - 2][i];
	}
	for (i = 0; i < h; i++)
	{
		dst_vals[i][0] = dst_vals[i][1];
		dst_vals[i][w - 1] = dst_vals[i][w - 2];
	}
	return dst_vals;
}

/*int** GAutoFocus::gauss_filter(int** src_image, int w, int h)
{
	static const int M[5][5] =
	{
		{1,  4,  6,  4, 1},
		{4, 16, 24, 16, 4},
		{6, 24, 36, 24, 6},
		{4, 16, 24, 16, 4},
		{1,  4,  6,  4, 1}
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
	int sum;
	for (y = 2; y < h - 2; y++)
	{
		for (x = 2; x < w - 2; x++)
		{
			sum = 0;
			for (i = 0; i < 5; i++)
			{
				for (j = 0; j < 5; j++)
				{
					sum += src_image[y + i - 2][x + j - 2]*M[i][j];
				}
			}
			sum /= 256;
			if (sum > 255)
				sum = 255;
			else if (sum < 0)
				sum = 0;
			dst_vals[y][x] = sum;
		}
	}
	for (i = 0; i < w; i++)
	{
		dst_vals[0][i] = dst_vals[2][i];
		dst_vals[1][i] = dst_vals[2][i];
		dst_vals[h - 2][i] = dst_vals[h - 3][i];
		dst_vals[h - 1][i] = dst_vals[h - 3][i];
	}
	for (i = 0; i < h; i++)
	{
		dst_vals[i][0] = dst_vals[i][2];
		dst_vals[i][1] = dst_vals[i][2];
		dst_vals[i][w - 2] = dst_vals[i][w - 3];
		dst_vals[i][w - 1] = dst_vals[i][w - 3];
	}
	return dst_vals;
}*/

int** GAutoFocus::gauss_filter(int** src_image, int w, int h)
{
	static const int M[3][3] =
	{
		{1, 2, 1},
		{2, 4, 2},
		{1, 2, 1}
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
	int sum;
	for (y = 1; y < h - 1; y++)
	{
		for (x = 1; x < w - 1; x++)
		{
			sum = 0;
			for (i = 0; i < 3; i++)
			{
				for (j = 0; j < 3; j++)
				{
					sum += src_image[y + i - 1][x + j - 1]*M[i][j];
				}
			}
			sum /= 16;
			if (sum > 255)
				sum = 255;
			else if (sum < 0)
				sum = 0;
			dst_vals[y][x] = sum;
		}
	}
	for (i = 0; i < w; i++)
	{
		dst_vals[0][i] = dst_vals[1][i];
		dst_vals[h - 1][i] = dst_vals[h - 2][i];
	}
	for (i = 0; i < h; i++)
	{
		dst_vals[i][0] = dst_vals[i][1];
		dst_vals[i][w - 1] = dst_vals[i][w - 2];
	}
	return dst_vals;
}

int GAutoFocus::dispersion(int** array, int w, int h, int n)
{
	int i, j;
	int res = 0;
	int avg = average(array, w, h, n);
	for (i = n; i < h - n; i++)
	{
		for (j = n; j < w - n; j++)
			res += (array[i][j] - avg)*(array[i][j] - avg);
	}
	res = res/((w - 2*n)*(h - 2*n));
	return res;
}

int GAutoFocus::average(int** array, int w, int h, int n)
{
	int i, j;
	int res = 0;
	for (i = n; i < h - n; i++)
	{
		for (j = n; j < w - n; j++)
			res += array[i][j];
	}
	res = res/((w - 2*n)*(h - 2*n));
	return res;
}

#if AF_DEBUG_LOG
QImage array_to_image(int** array, int w, int h)
{
	QImage img(w, h, QImage::Format_RGB32);
	int i, j;
	for (i = 0; i < h; i++)
		for (j = 0; j < w; j++)
			img.setPixel(j, i, qRgb(array[i][j], array[i][j], array[i][j]));
	return img;
}
#endif
