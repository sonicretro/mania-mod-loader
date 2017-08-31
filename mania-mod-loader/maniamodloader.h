// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MANIAMODLOADER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MANIAMODLOADER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MANIAMODLOADER_EXPORTS
#define MANIAMODLOADER_API __declspec(dllexport)
#else
#define MANIAMODLOADER_API __declspec(dllimport)
#endif

// This class is exported from the maniamodloader.dll
class MANIAMODLOADER_API Cmaniamodloader {
public:
	Cmaniamodloader(void);
	// TODO: add your methods here.
};

extern MANIAMODLOADER_API int nmaniamodloader;

MANIAMODLOADER_API int fnmaniamodloader(void);
