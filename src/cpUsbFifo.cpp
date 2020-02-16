#include "stdafx.h"
#include "cpUsbFifo.h"

usbfifo::usbfifo(void)
{
}
usbfifo::~usbfifo(void)
{
}

void usbfifo::write(BYTE* buf, int length, int& save_real)
{
	bool isfull =false;
	if ((length < 1) || (true == full))
	{
		save_real = 0;
		isfull=true;
	}
	if (!isfull)
	{
			if((lengthsave+length)<FIFLODEEPLENGTH)
			{
				save_real = length;
				empty = false;
				full = false;
			}
			else  // full
			{
				// save only part data
				save_real = FIFLODEEPLENGTH - lengthsave-1;
				empty = false;
				full = true;
			}
			// save now
			if ((pwrite + save_real)>FIFLODEEPLENGTH)
			{
				int actsavelength1,actsavelength2;
				actsavelength1= FIFLODEEPLENGTH-pwrite;
				actsavelength2  = save_real-actsavelength1;
				memcpy(ram+pwrite,buf,actsavelength1);
				memcpy(ram,&buf[actsavelength1],actsavelength2);
				pwrite = actsavelength2;
			}
			else
			{
				memcpy(ram+pwrite,buf,save_real);
				pwrite = pwrite + save_real;
			}
			lengthsave = lengthsave + save_real;
	}
}
void usbfifo::read(BYTE* buf, int length,LONG& reback)
{
	int real_pop;
	bool isempty =false;
	if ((length <1 ) || (true == empty))
	{
		reback =0;
		isempty =true;
	}
	if (!isempty)
	{
	
		if (lengthsave>length) // pop
		{
			real_pop = length;
			empty = false;
			full = false;
		}
		else  // not empty and not enough data
		{
			real_pop = lengthsave;
			empty = true;
			full = false;
		}

		// Pop now
		if ((pread + real_pop)>FIFLODEEPLENGTH)
		{
			int actsavelength1,actsavelength2;
			actsavelength1= FIFLODEEPLENGTH-pread;
			actsavelength2  = real_pop-actsavelength1;
			memcpy(buf,ram+pread,actsavelength1);
			memcpy(buf+actsavelength1,ram,actsavelength2);
			pread = actsavelength2;
		}
		else
		{
			memcpy(buf,ram+pread,real_pop);
			pread = pread + real_pop;
		}
		lengthsave = lengthsave - real_pop;
		reback = real_pop;
		//qDebug("******lengthsave is %ld  reback is %d pread is %ld*********",lengthsave,real_pop,pread);
	}
}
void usbfifo::initialfifo()
{
	pwrite=0;
	pread=0;
	full=false;
	empty =true;
	lengthsave= 0;
}