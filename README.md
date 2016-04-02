# browser-dumpwd
Dump browser passwords(chrome, firefox) with sqlite3 lib. 

# tested
Windows 7, Windows 10 x64 and x86
Chrome, Firefox

#example
> me.c file


```

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "chrome.h"
#include "firefox.h"

int main(int argc, char** argv)
{
	dump_chrome_passwords();
	dump_firefox_passwords();
	return EXIT_SUCCESS;
}

```

# contact
0xdr@protonmail.ch
> Dr. R
