#ifndef YQ_AVIFILE_H
#define YQ_AVIFILE_H

#include <windows.h>
#include <vfw.h>

#import "GetAVIInfo.dll" no_namespace named_guids raw_interfaces_only

namespace YQ
{

class AviFile
{
	public:
		AviFile();
		void open(const wchar_t *name, HDC hDc = NULL);
		unsigned getMsPerFrame();
		unsigned getLength();
		void seek(unsigned frame);
		unsigned seekNext();
		void renderFrame(HDC hDc, RECT rect);
		void close();
		~AviFile();
	private:
		bool m_opened;
		IGetAVIInfo *m_pGetAviInfo;
		AVIStreamHeader m_videoHeader;
		BITMAPINFOHEADER m_bitmapHeaderSrc;
		BITMAPINFOHEADER m_bitmapHeaderDst;
		char *m_bitmapBuffer;
		HIC m_hDecompressor;
		HANDLE m_hFile;
		HDC m_hMemDc;
		HBITMAP m_hBitmap;
		unsigned m_position;
		long m_width;
		long m_height;
};

}

#endif //YQ_AVIFILE_H