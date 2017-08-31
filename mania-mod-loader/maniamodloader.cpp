// maniamodloader.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "maniamodloader.h"


// This is an example of an exported variable
MANIAMODLOADER_API int nmaniamodloader=0;

// This is an example of an exported function.
MANIAMODLOADER_API int fnmaniamodloader(void)
{
    return 42;
}

// This is the constructor of a class that has been exported.
// see maniamodloader.h for the class definition
Cmaniamodloader::Cmaniamodloader()
{
    return;
}
