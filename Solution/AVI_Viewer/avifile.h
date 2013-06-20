#ifndef YQ_AVIFILE_H
#define YQ_AVIFILE_H

#include <windows.h>
#include <vfw.h>

namespace YQ
{

class AviFile
{
	public:
		AviFile(wchar_t *name, HDC hDc = NULL);
		unsigned getMsPerFrame();
		unsigned getLength();
		void seek(unsigned frame);
		unsigned seekNext();
		void renderFrame(HDC hDc, RECT rect);
		~AviFile();
	private:
		PAVIFILE pFile;
		AVIFILEINFOW aviInfo;
		PAVISTREAM pStream;
		PGETFRAME pGetFrame;
		unsigned first;
		unsigned position;
		HDC memDc;
		HBITMAP bitmap;
};

}

#endif //YQ_AVIFILE_H