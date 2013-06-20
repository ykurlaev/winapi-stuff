#include "avifile.h"
#include <string>
#include <stdexcept>

using namespace YQ;

using std::string;
using std::runtime_error;

AviFile::AviFile(wchar_t *name, HDC hDc)
: pFile(NULL), pStream(NULL), position(0), memDc(NULL), bitmap(NULL)
{
	memset(&aviInfo, 0, sizeof(aviInfo));
	int e = AVIFileOpenW(&pFile, name, OF_READ, NULL);
	if(e)
	{
		string s;
		switch(e) // see MSDN for AVIFileOpen
		{
			case AVIERR_BADFORMAT:
				s = "Corrupt file or an unrecognized format";
				break;
			case AVIERR_MEMORY:
				s = "Not enough memory";
				break;
			case AVIERR_FILEREAD:
				s = "A disk error occurred while reading the file";
				break;
			case AVIERR_FILEOPEN:
				s = "A disk error occurred while opening the file";
				break;
			case REGDB_E_CLASSNOTREG:
				s = "The type of file does not have a handler to process it";
				break;
			default:
				s = "Unknown error";
				break;
		}
		throw runtime_error(s);
	}
	AVIFileInfo(pFile, &aviInfo, sizeof(aviInfo));
	e = AVIFileGetStream(pFile, &pStream, streamtypeVIDEO, 0);
	if(e)
	{
		string s;
		switch(e)
		{
			case AVIERR_NODATA:
				s = "The file does not contain video stream";
				break;
			case AVIERR_MEMORY:
				s = "Not enough memory";
				break;
			default:
				s = "Unknown error";
				break;
		}
		throw runtime_error(s);
	}
	first = AVIStreamStart(pStream);
	pGetFrame = AVIStreamGetFrameOpen(pStream, NULL);
	memDc = CreateCompatibleDC(hDc);
	bitmap = CreateCompatibleBitmap(hDc, aviInfo.dwWidth, aviInfo.dwHeight);
	SelectObject(memDc, bitmap);
}

unsigned AviFile::getMsPerFrame()
{
	return static_cast<int>((static_cast<float>(aviInfo.dwScale) /
							 static_cast<float>(aviInfo.dwRate)) * 1000.);
}

unsigned AviFile::getLength()
{
	return aviInfo.dwLength;
}

void AviFile::seek(unsigned frame)
{
	position = frame;
}

unsigned AviFile::seekNext()
{
	seek((position + 1) % aviInfo.dwLength);
	return position;
}

void AviFile::renderFrame(HDC hDc, RECT rect)
{
	char *data = static_cast<char *>(AVIStreamGetFrame(pGetFrame, position - first));
	BITMAPINFO *bitmapInfo = reinterpret_cast<BITMAPINFO *>(data);
	SetDIBitsToDevice(memDc, 0, 0, aviInfo.dwWidth, aviInfo.dwHeight, 0, 0, 0, aviInfo.dwHeight,
					  data + bitmapInfo->bmiHeader.biSize + bitmapInfo->bmiHeader.biClrUsed * 4,
					  bitmapInfo, DIB_RGB_COLORS);
	StretchBlt(hDc, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top),
			   memDc, 0, 0, aviInfo.dwWidth, aviInfo.dwHeight, SRCCOPY);
}

AviFile::~AviFile()
{
	AVIStreamGetFrameClose(pGetFrame);
	AVIFileRelease(pFile);
	DeleteDC(memDc);
	DeleteObject(bitmap);
}