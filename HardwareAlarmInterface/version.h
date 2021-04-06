#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "18";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2008";
	static const double UBUNTU_VERSION_STYLE = 8.06;

	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";

	//Standard Version Type
	static const long MAJOR = 1;
	static const long MINOR = 3;
	static const long BUILD = 359;
	static const long REVISION = 1956;

	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 381;
	#define RC_FILEVERSION 1,3,359,1956
	#define RC_FILEVERSION_STRING "1, 3, 359, 1956\0"
	static const char FULLVERSION_STRING[] = "1.3.359.1956";

	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 56;


#endif //VERSION_H
