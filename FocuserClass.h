/***************************************************************************
 *   Copyright (C) 2008-2009 by Uterr                                      *
 *   --                                                                    *
 *     Some fixes by Чернов А.А. <valexlin@gmail.com>                      *
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

#include <vector>
using namespace std;

class FocusingClass
{
	private:
		int NextFocus;
		double Noise;
		int focus_step;
		int change_count;
		int NoiseCounts;
	public:

	typedef struct focusingInfo
	{
		double dispersion;
		int focusPosition;
		int focusDir;
	};

	vector <FocusingClass::focusingInfo*> finfos;



	FocusingClass()
	{
		NextFocus=1;
		Noise = 0.0;
		stop=false;
		focus_step = 8;
		change_count = 0;
		NoiseCounts = 20;		// must calibrate
	}
	~FocusingClass()
	{
		int infs=finfos.size();
		for(int i=0;i<infs;i++)
		{
			delete finfos[i];
		}
		finfos.clear();
	}

	double lastDispersion()
	{
		int last_index = (int)finfos.size() - 1;
		if (last_index < 0)
			return 0.0;
		return finfos[last_index]->dispersion;
	}

	double lastPosition()
	{
		int last_index = (int)finfos.size() - 1;
		if (last_index < 0)
			return 0.0;
		return finfos[last_index]->focusPosition;
	}

	bool stop;
	double dabs(double i)
	{
		return i>0?i:-i;
	}
	int abs(int i)
	{
		return i>0?i:-i;
	}
	void NextIter(double **Luminance,int x_dim,int y_dim)
	{
		if (stop)
			return;

		int last_index = (int)finfos.size() - 1;
		focusingInfo* finf = new focusingInfo;
		finf->dispersion = Dispersion(Luminance, x_dim, y_dim);			// this value of dispersion is a result of previous NextFocus
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
			Noise = maxdispersion() - mindispersion();
		}

		if (last_index > NoiseCounts)
		{
			if (dabs(finf->dispersion - finfos[last_index - 1]->dispersion) < Noise)
				finf->dispersion = finfos[last_index - 1]->dispersion;
			if (finfos.size() > 70 + NoiseCounts)
				if (dabs((finf->dispersion - finfos[last_index-8]->dispersion)) <= 1.1*Noise)
				// это условие надо заменить на другое
				// дисперсия от дисперсий в диапазоне от last_index-8 до last_index должна быть в пределах погрешности (шумов)
					stop = true;

			double max_disp = maxdispersion();
			double min_disp = mindispersion();
			if (finf->dispersion > max_disp - Noise && finfos.size() > 50 + NoiseCounts)
				stop = true;

			if (finf->focusDir != finfos[last_index - 1]->focusDir && abs(finfos[last_index - 1]->focusDir) > 0)
				change_count++;
			if (change_count > 3)
				focus_step = 1;
			if (change_count > 12)
				stop = true;

			if (finf->dispersion >= finfos[last_index - 1]->dispersion ||
				dabs(finf->dispersion - max_disp) < 2.0*Noise)
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

	double maxdispersion()
	{
		double ret=finfos[0]->dispersion;
		int infs=finfos.size();
		for(int i=0;i<infs;i++)
		{
			if(ret<finfos[i]->dispersion)
				ret=finfos[i]->dispersion;
		}
		return ret;
	}

	double noise()
	{
		return Noise;
	}

	double mindispersion()
	{
		double ret=finfos[0]->dispersion;
		int infs=finfos.size();
		for(int i=0;i<infs;i++)
		{
			if(ret>finfos[i]->dispersion)
			ret=finfos[i]->dispersion;
		}
		return ret;
	}

	int getNextFocus()
	{
		if (finfos.size() <= NoiseCounts)
			return 0;
		return NextFocus;
	}

	double Dispersion(double**lum_array,int x_dim,int y_dim)
	{
		return M1(lum_array,x_dim,y_dim);
	}

	double M1(double**lum_array,int x_dim,int y_dim)
	{
		double m_=M(lum_array,x_dim,y_dim);
		double ret=0;

		for(int i=0;i<x_dim;i++)
		{
			for(int j=0;j<y_dim;j++)
			{
				ret+=(lum_array[i][j]-m_)*(lum_array[i][j]-m_);
			}
		}

		return ret/(x_dim*y_dim);
	}

	double M(double**lum_array,int x_dim,int y_dim)
	{
		double ret=0;

		for(int i=0;i<x_dim;i++)
		{
			for(int j=0;j<y_dim;j++)
			{
				ret+=lum_array[i][j];
			}
		}

		return ret/(x_dim*y_dim);
	}
};
