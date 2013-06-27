#include "avifile.h"
#include <string>
#include <stdexcept>

using namespace YQ;

using std::runtime_error;

AviFile::AviFile()
: m_opened(false), m_pGetAviInfo(NULL), m_bitmapBuffer(NULL),
  m_hDecompressor(NULL), m_hFile(NULL), m_hMemDc(NULL), m_hBitmap(NULL), m_position(0)
{
	memset(&m_videoHeader, 0, sizeof(m_videoHeader));
	memset(&m_bitmapHeaderSrc, 0, sizeof(m_bitmapHeaderSrc));
	memset(&m_bitmapHeaderDst, 0, sizeof(m_bitmapHeaderDst));
	m_bitmapHeaderDst.biSize = sizeof(m_bitmapHeaderDst);
	m_bitmapHeaderDst.biPlanes = 1;
	m_bitmapHeaderDst.biBitCount = 32;
	m_bitmapHeaderDst.biCompression = BI_RGB;
}

void AviFile::open(const wchar_t *name, HDC hDc)
{
	if(m_opened)
	{
		close();
	}
	m_opened = true;
	if(!m_pGetAviInfo)
	{
		if(CoCreateInstance(CLSID_GetAVIInfo, NULL, CLSCTX_INPROC_SERVER, IID_IGetAVIInfo,
							 reinterpret_cast<void **>(&m_pGetAviInfo)))
		{
			throw runtime_error("Can't create GetAVIInfo instance");
		}
	}
	BSTR bstrName = SysAllocString(name);
	if(!bstrName)
	{
		throw runtime_error("Can't allocate string");
	}
	HRESULT hRes = m_pGetAviInfo->FastLoad(bstrName);
	SysFreeString(bstrName);
	if(hRes)
	{
		throw runtime_error("Can't load AVI file");
	}
	long videoHeaderSize, bitmapHeaderSize;
	if(m_pGetAviInfo->GetVideoStreamInfo(0, reinterpret_cast<BYTE *>(&m_videoHeader),
										 sizeof(m_videoHeader), &videoHeaderSize,
										 &bitmapHeaderSize, NULL))
	{
		throw runtime_error("Can't get video stream info");
	}
	if(m_pGetAviInfo->GetVideoFormatInfo(0, reinterpret_cast<BYTE *>(&m_bitmapHeaderSrc),
										 sizeof(m_bitmapHeaderSrc), &bitmapHeaderSize))
	{
		throw runtime_error("Can't get video format info");
	}
	m_bitmapHeaderDst.biWidth = m_bitmapHeaderSrc.biWidth;
	m_bitmapHeaderDst.biHeight = m_bitmapHeaderSrc.biHeight;
	m_hDecompressor = ICDecompressOpen(ICTYPE_VIDEO, m_videoHeader.fccHandler, &m_bitmapHeaderSrc, 0);
	if(!m_hDecompressor)
	{
		throw runtime_error("Can't open decompressor");
	}
	for(; m_bitmapHeaderDst.biBitCount > 0; m_bitmapHeaderDst.biBitCount -= 8)
	{
		if(ICDecompressBegin(m_hDecompressor, &m_bitmapHeaderSrc, &m_bitmapHeaderDst) != ICERR_BADFORMAT)
		{
			break;
		}
	}
	if(m_bitmapHeaderDst.biBitCount == 0)
	{
		m_bitmapHeaderDst.biBitCount = 32;
		throw runtime_error("Decompressor format error");
	}
	m_bitmapHeaderDst.biSizeImage = ((m_bitmapHeaderDst.biWidth * m_bitmapHeaderDst.biBitCount + 31) / 32)
									* 4 * m_bitmapHeaderDst.biHeight;
	m_bitmapBuffer = new char[m_bitmapHeaderDst.biSizeImage];
	if(!m_bitmapBuffer)
	{
		throw runtime_error("Can't allocate memory");
	}
	m_hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL,
						 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(m_hFile == INVALID_HANDLE_VALUE)
	{
		throw runtime_error("Can't open file for reading");
	}
	m_hMemDc = CreateCompatibleDC(hDc);
	if(!m_hMemDc)
	{
		throw runtime_error("Can't create draw context");
	}
	m_width = m_bitmapHeaderDst.biWidth;
	m_height = m_bitmapHeaderDst.biHeight;
	m_hBitmap = CreateCompatibleBitmap(hDc, m_width, m_height);
	if(!m_hBitmap)
	{
		throw runtime_error("Can't create bitmap");
	}
	HGDIOBJ hGdiObj = SelectObject(m_hMemDc, m_hBitmap);
	if(!hGdiObj || hGdiObj == HGDI_ERROR)
	{
		throw runtime_error("Can't select bitmap");
	}
}

unsigned AviFile::getMsPerFrame()
{
	if(!m_opened)
	{
		return 0;
	}
	return static_cast<int>((static_cast<float>(m_videoHeader.dwScale) /
							 static_cast<float>(m_videoHeader.dwRate)) * 1000.);
}

unsigned AviFile::getLength()
{
	if(!m_opened)
	{
		return 0;
	}
	return m_videoHeader.dwLength;
}

void AviFile::seek(unsigned frame)
{
	if(!m_opened || frame >= m_videoHeader.dwLength)
	{
		return;
	}
	m_position = frame;
	unsigned long flags;
	do
	{
		long hi_offset;
		unsigned long low_offset, size;
		m_pGetAviInfo->GetVideoFrameInfo2(0, m_position, &hi_offset, &low_offset, &size, &flags);
		m_position -= 1;
	}
	while(!(flags & AVIIF_KEYFRAME));
	m_position += 1;
	for(; m_position < frame; m_position++)
	{
		RECT r;
		memset(&r, 0, sizeof(r));
		renderFrame(NULL, r);
	}
}

unsigned AviFile::seekNext()
{
	if(!m_opened || m_position == m_videoHeader.dwLength - 1)
	{
		return 0;
	}
	m_position += 1;
	return m_position;
}

void AviFile::renderFrame(HDC hDc, RECT rect)
{
	if(!m_opened)
	{
		return;
	}
	long hi_offset;
	unsigned long low_offset, size, flags;
	m_pGetAviInfo->GetVideoFrameInfo2(0, m_position, &hi_offset, &low_offset, &size, &flags);
	LARGE_INTEGER offset;
	offset.HighPart = hi_offset;
	offset.LowPart = low_offset;
	SetFilePointerEx(m_hFile, offset, NULL, FILE_BEGIN);
	char *buffer = new char[size];
	unsigned long read;
	ReadFile(m_hFile, buffer, size, &read, NULL);
	ICDecompress(m_hDecompressor, (flags & AVIIF_KEYFRAME) ? 0 : ICDECOMPRESS_NOTKEYFRAME,
				 &m_bitmapHeaderSrc, buffer, &m_bitmapHeaderDst, m_bitmapBuffer);
	if(hDc)
	{
		SetDIBitsToDevice(m_hMemDc, 0, 0, m_width, m_height, 0, 0, 0, m_height,
						  m_bitmapBuffer, reinterpret_cast<BITMAPINFO *>(&m_bitmapHeaderDst),
						  DIB_RGB_COLORS);
		StretchBlt(hDc, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top),
				   m_hMemDc, 0, 0, m_width, m_height, SRCCOPY);
	}
	delete[] buffer;
}

void AviFile::close()
{
	if(m_hMemDc)
	{
		DeleteDC(m_hMemDc);
	}
	if(m_hBitmap)
	{
		DeleteObject(m_hBitmap);
	}
	if(m_hFile)
	{
		CloseHandle(m_hFile);
	}
	if(m_hDecompressor)
	{
		ICDecompressEnd(m_hDecompressor);
		ICClose(m_hDecompressor);
	}
	if(m_bitmapBuffer)
	{
		delete[] m_bitmapBuffer;
	}
}

AviFile::~AviFile()
{
	if(m_opened)
	{
		close();
	}
	if(m_pGetAviInfo)
	{
		m_pGetAviInfo->Release();
	}
}